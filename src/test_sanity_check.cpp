#include <eventprocessor/core/event.h>
#include <eventprocessor/core/event_processor_concurrent_in_place.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <new>
#include <thread>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
template <typename Derived>
class CountingEvent : public IEvent {
public:
    ~CountingEvent() override { ++destroyed; }

    void Process() noexcept override { ++processed; }

    static std::size_t Created() { return created; }
    static std::size_t Destroyed() { return destroyed; }
    static std::size_t Processed() { return processed; }

protected:
    CountingEvent() { ++created; }

private:
    inline static thread_local std::size_t created = 0;
    inline static thread_local std::size_t destroyed = 0;
    inline static thread_local std::size_t processed = 0;
};

class SimpleEvent final : public CountingEvent<SimpleEvent> {
    [[maybe_unused]] uint64_t data2_{};
};
static_assert(sizeof(SimpleEvent) == 16);

class ComplexEvent final : public CountingEvent<ComplexEvent> {
    [[maybe_unused]] std::unique_ptr<uint64_t> data_ = std::make_unique<uint64_t>(42);
};
static_assert(sizeof(ComplexEvent) == 16);

static inline auto Now() {
    // TODO: add barriers to prohibit reordering see google bench
    return std::chrono::high_resolution_clock::now();
}

static inline auto DiffNs(auto start, auto end) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

class StatsEvent final : public CountingEvent<StatsEvent> {
public:
    explicit StatsEvent(std::chrono::high_resolution_clock::time_point start_time = Now()) : start_time_(start_time) {}

    ~StatsEvent() override {
        auto end_time = Now();
        auto duration_ns = DiffNs(start_time_, end_time);
        durations.push_back(duration_ns);
    }

    static void ReserveDurations(size_t count) { durations.reserve(count); }

    struct Stats {
        size_t size{};
        uint64_t p50{};
        uint64_t p90{};
        uint64_t p95{};
        uint64_t p99{};
        uint64_t avg{};
    };
    static Stats GetStats() {
        // TODO: use some library
        if (durations.empty()) {
            return {};
        }

        auto sorted = durations;
        std::ranges::sort(sorted);

        auto percentile = [&sorted](double p) {
            auto idx = static_cast<size_t>(p * sorted.size());
            if (idx >= sorted.size()) {
                idx = sorted.size() - 1;
            }
            return sorted[idx];
        };

        int64_t sum = 0;
        for (auto d : sorted) {
            sum += d;
        }
        double avg = static_cast<double>(sum) / sorted.size();

        return {
            .size = durations.size(),
            .p50 = static_cast<uint64_t>(percentile(0.5)),
            .p90 = static_cast<uint64_t>(percentile(0.9)),
            .p95 = static_cast<uint64_t>(percentile(0.95)),
            .p99 = static_cast<uint64_t>(percentile(0.99)),
            .avg = static_cast<uint64_t>(avg),
        };
    }

private:
    std::chrono::high_resolution_clock::time_point start_time_;
    inline static thread_local std::vector<int64_t> durations;
};
static_assert(sizeof(StatsEvent) == 16);

int main() {
    static constexpr size_t kEventsPerProducerThread = 10'000'000;
    static constexpr size_t kThreadCapacity = 16;
    static constexpr size_t kExpectedProcessedEvents = kEventsPerProducerThread * kThreadCapacity;
    static constexpr std::memory_order kRelaxed = std::memory_order_relaxed;

    std::unique_ptr<IEventProcessor> event_processor = std::make_unique<EventProcessorConcurrentInPlace>();

    // 16 producers threads (kThreadCapacity) + 1 consumer main
    std::atomic<size_t> participants{};
    std::atomic<size_t> producers_done{};

    struct ConsumerThreadStats {
        size_t processed_events{};
        // during execution
        size_t during_failed_polls{};
        size_t during_success_polls{};
        // after we have collected exected events,
        // but before all producers are done
        size_t after_failed_polls{};
        size_t after_success_polls{};
    } consumer_stats;

    struct alignas(std::hardware_destructive_interference_size) ProducerThreadStats {
        size_t processed_events{};
        // during execution
        size_t during_failed_polls{};
        size_t during_success_polls{};
    };

    struct alignas(std::hardware_destructive_interference_size) ProducerEventStats {
        struct Stats {
            size_t events_created{};
            size_t events_destroyed{};
            size_t events_processed{};
        };
        Stats simple;
        Stats complex;
        Stats stats;
    };

    std::array<ProducerEventStats, kThreadCapacity> producer_event_stats;
    std::array<ProducerThreadStats, kThreadCapacity> producer_threads_stats;
    std::array<std::thread, kThreadCapacity> producer_threads;
    for (size_t i = 0; i < kThreadCapacity; ++i) {
        auto& producer = producer_threads[i];
        auto& producer_stats = producer_threads_stats[i];
        auto& event_stats = producer_event_stats[i];

        auto producer_task = [&event_processor, &producer_stats, &event_stats, &producers_done, &participants] {
            // start when all producers and consumer are ready
            participants.fetch_add(1, kRelaxed);
            while (participants.load(kRelaxed) != kThreadCapacity + 1) {
                // TODO: spin-wait
            }
            auto event_submitter = [&event_processor, &producer_stats]<typename Event>() -> size_t {
                IEventProcessor::ReservedEvent reserved_event;
                if constexpr (std::is_same_v<Event, StatsEvent>) {
                    auto now = std::chrono::high_resolution_clock::now();
                    reserved_event = event_processor->Reserve<Event>(now);
                } else {
                    reserved_event = event_processor->Reserve<Event>();
                }
                if (!reserved_event.IsValid()) {
                    ++producer_stats.during_failed_polls;
                    return 0;
                }
                event_processor->Commit(reserved_event.GetSequenceNumber());
                ++producer_stats.during_success_polls;
                ++producer_stats.processed_events;
                return 1;
            };
            auto multi_event_submitter = [&event_processor, &producer_stats](size_t current_i) -> size_t {
                auto range = std::min(size_t{101}, kEventsPerProducerThread - current_i);
                auto reserved_events_collection = event_processor->ReserveRange(range);

                if (reserved_events_collection.empty()) {
                    ++producer_stats.during_failed_polls;
                    return 0;
                }

                ++producer_stats.during_success_polls;
                size_t total_reserved = 0;
                for (auto& reserved_events : reserved_events_collection) {
                    for (size_t i = 0; i < reserved_events.Count(); ++i) {
                        reserved_events.Emplace<SimpleEvent>(i);
                    }
                    event_processor->Commit(reserved_events.GetSequenceNumber(), reserved_events.Count());
                    total_reserved += reserved_events.Count();
                }
                producer_stats.processed_events += total_reserved;
                return total_reserved;
            };
            for (size_t i = 0, j = 0; i < kEventsPerProducerThread; j = (j + 1) % 4) {
                size_t events{};
                switch (j) {
                    case 0:
                        events += event_submitter.template operator()<SimpleEvent>();
                        break;
                    case 1:
                        events += event_submitter.template operator()<ComplexEvent>();
                        break;
                    case 2:
                        events += multi_event_submitter(i);
                        break;
                    case 3:
                        events += event_submitter.template operator()<StatsEvent>();
                        break;
                    default:
                        std::unreachable();
                }
                if (events == 0) {
                    // TODO: spin-wait
                }
                i += events;
            }
            event_stats.simple.events_created = CountingEvent<SimpleEvent>::Created();
            event_stats.simple.events_destroyed = CountingEvent<SimpleEvent>::Destroyed();
            event_stats.simple.events_processed = CountingEvent<SimpleEvent>::Processed();
            event_stats.complex.events_created = CountingEvent<ComplexEvent>::Created();
            event_stats.complex.events_destroyed = CountingEvent<ComplexEvent>::Destroyed();
            event_stats.complex.events_processed = CountingEvent<ComplexEvent>::Processed();
            event_stats.stats.events_created = CountingEvent<StatsEvent>::Created();
            event_stats.stats.events_destroyed = CountingEvent<StatsEvent>::Destroyed();
            event_stats.stats.events_processed = CountingEvent<StatsEvent>::Processed();

            producers_done.fetch_add(1, kRelaxed);
        };
        producer = std::thread(producer_task);
    }

    std::chrono::high_resolution_clock::time_point start_time_point{};
    std::chrono::high_resolution_clock::time_point end_time_point{};

    StatsEvent::ReserveDurations(kExpectedProcessedEvents);
    auto consumer_task =
        [&event_processor, &consumer_stats, &producers_done, &participants, &start_time_point, &end_time_point] {
            while (participants.load(kRelaxed) != kThreadCapacity) {
                // TODO: spin-wait
            }
            start_time_point = Now();
            participants.fetch_add(1, kRelaxed);

            auto& cs = consumer_stats;
            while (cs.processed_events < kExpectedProcessedEvents) {
                auto processed = event_processor->ProcessEvents();
                if (processed == 0) {
                    ++cs.during_failed_polls;
                } else {
                    ++cs.during_success_polls;
                }
                cs.processed_events += processed;
            }

            end_time_point = Now();

            while (producers_done.load(kRelaxed) != kThreadCapacity) {
                auto processed = event_processor->ProcessEvents();
                if (processed == 0) {
                    ++cs.after_failed_polls;
                } else {
                    ++cs.after_success_polls;
                }
                cs.processed_events += processed;
            };
        };
    consumer_task();

    for (auto& producer : producer_threads) {
        producer.join();
    }

    // Printing results

    std::cout << "Consumer:\n"
              << "\tProcessedEvents: " << consumer_stats.processed_events << "\n"
              << "\tDuringFailedPolls: " << consumer_stats.during_failed_polls << "\n"
              << "\tDuringSuccessPolls: " << consumer_stats.during_success_polls << "\n"
              << "\tAfterFailedPolls: " << consumer_stats.after_failed_polls << "\n"
              << "\tAfterSuccessPolls: " << consumer_stats.after_success_polls << "\n"
              << "\tSimpleEvent:\n"
              << "\tc " << CountingEvent<SimpleEvent>::Created()              //
              << " | d " << CountingEvent<SimpleEvent>::Destroyed()           //
              << " | p " << CountingEvent<SimpleEvent>::Processed() << "\n"   //
              << "\tComplexEvent:\n"                                          //
              << "\tc " << CountingEvent<ComplexEvent>::Created()             //
              << " | d " << CountingEvent<ComplexEvent>::Destroyed()          //
              << " | p " << CountingEvent<ComplexEvent>::Processed() << "\n"  //
              << "\tStatsEvent:\n"                                            //
              << "\tc " << CountingEvent<StatsEvent>::Created()               //
              << " | d " << CountingEvent<StatsEvent>::Destroyed()            //
              << " | p " << CountingEvent<StatsEvent>::Processed() << "\n"    //
              << "\tTotalEvents:\n"                                           //
              << "\tc "
              << CountingEvent<SimpleEvent>::Created() + CountingEvent<ComplexEvent>::Created() +
                     CountingEvent<StatsEvent>::Created()  //
              << " | d "
              << CountingEvent<SimpleEvent>::Destroyed() + CountingEvent<ComplexEvent>::Destroyed() +
                     CountingEvent<StatsEvent>::Destroyed()  //
              << " | p "
              << CountingEvent<SimpleEvent>::Processed() + CountingEvent<ComplexEvent>::Processed() +
                     CountingEvent<StatsEvent>::Processed()  //
              << "\n";

    for (size_t i = 0; i < kThreadCapacity; ++i) {
        auto& ps = producer_threads_stats[i];
        auto& pes = producer_event_stats[i];
        std::cout << "Producer: " << i << "\n"
                  << "\tProcessedEvents: " << ps.processed_events << "\n"
                  << "\tDuringFailedPolls: " << ps.during_failed_polls << "\n"
                  << "\tDuringSuccessPolls: " << ps.during_success_polls << "\n"
                  << "\tSimpleEvent:\n"
                  << "\tc " << pes.simple.events_created                                                          //
                  << " | d " << pes.simple.events_destroyed                                                       //
                  << " | p " << pes.simple.events_processed << "\n"                                               //
                  << "\tComplexEvent:\n"                                                                          //
                  << "\tc " << pes.complex.events_created                                                         //
                  << " | d " << pes.complex.events_destroyed                                                      //
                  << " | p " << pes.complex.events_processed << "\n"                                              //
                  << "\tStatsEvent:\n"                                                                            //
                  << "\tc " << pes.stats.events_created                                                           //
                  << " | d " << pes.stats.events_destroyed                                                        //
                  << " | p " << pes.stats.events_processed << "\n"                                                //
                  << "\tTotalEvents:\n"                                                                           //
                  << "\tc " << pes.simple.events_created + pes.complex.events_created + pes.stats.events_created  //
                  << " | d "
                  << pes.simple.events_destroyed + pes.complex.events_destroyed + pes.stats.events_destroyed  //
                  << " | p " << pes.simple.events_processed + pes.complex.events_processed + pes.stats.events_processed
                  << "\n\n";  //
    }

    std::cout << "General: \n"
              << "\tTotalProcessedEvents/kExpectedProcessedEvents: " << consumer_stats.processed_events << "/"
              << kExpectedProcessedEvents << "\n"
              << "\tConsumer 1: \t" << consumer_stats.processed_events / consumer_stats.during_success_polls
              << " events/poll; "
              << "\tfail rate: "
              << consumer_stats.during_failed_polls * 100.0 /
                     (consumer_stats.during_success_polls + consumer_stats.during_failed_polls)
              << "% \n";

    for (size_t i = 0; i < kThreadCapacity; ++i) {
        auto& ps = producer_threads_stats[i];
        std::cout << "\tProducer " << i << ": \t" << ps.processed_events / ps.during_success_polls << " events/poll; "
                  << "\tfail rate: "
                  << ps.during_failed_polls * 100.0 / (ps.during_success_polls + ps.during_failed_polls) << "% \n";
    }

    std::cout << "\tElapsed time: " << DiffNs(start_time_point, end_time_point) / 1'000'000 << " ms\n"
              << "\tThroughput: "
              << (consumer_stats.processed_events * 1000.0) / DiffNs(start_time_point, end_time_point) << "M events/s\n"
              << "\tPerEvent: "
              << DiffNs(start_time_point, end_time_point) / static_cast<double>(consumer_stats.processed_events)
              << " ns/event\n";
    auto event_stats = StatsEvent::GetStats();
    std::cout << "\tStatsEvent processing durations:\n"
              << "\tCount: " << event_stats.size << "\n"
              << "\tP50: " << event_stats.p50 << " ns | " << (event_stats.p50 / 1000.0) << " us\n"
              << "\tP90: " << event_stats.p90 << " ns | " << (event_stats.p90 / 1000.0) << " us\n"
              << "\tP95: " << event_stats.p95 << " ns | " << (event_stats.p95 / 1000.0) << " us\n"
              << "\tP99: " << event_stats.p99 << " ns | " << (event_stats.p99 / 1000.0) << " us\n"
              << "\tAvg: " << event_stats.avg << " ns | " << (event_stats.avg / 1000.0) << " us\n\n";

    return 0;
}
