#pragma once

#include "event.h"
#include "event_processor.h"
#include "../queues/mpsc.h"

#include <algorithm>
#include <cassert>
#include <memory>

// spsc queue per thread, events are processed in place by consumer thread
class EventProcessorConcurrentInPlace final : public IEventProcessor {
 public:
  using IEventProcessor::ReservedEvent;

  ReserveResult ReserveEvent([[maybe_unused]] size_t type_size) override {
    assert(type_size <= kSlotByteSize);

    std::array<MpscQueue::Slot*, 1> out_buffer{};
    auto reserved_size = queue_.ProduceViewSlots(out_buffer);
    bool is_full = reserved_size == 0;
    if (is_full) {
      return {};
    }
    return {
        /*.sequence_number =*/kNotUsed,
        /*.memory =*/out_buffer[0]->storage.data(),
    };
  }

  void Commit(const Integer) override {
    queue_.ProduceCommitSlots(1);
  }

  std::vector<ReservedEvents> ReserveRange(const size_t size) override {
    // TODO: make constructor parameter
    static constexpr size_t kMaxBatchSize = 256;
    std::array<MpscQueue::Slot*, kMaxBatchSize> out_buffer{};
    SlotsView slots_view{out_buffer.data(), std::min(size, kMaxBatchSize)};

    auto reserved_size = queue_.ProduceViewSlots(slots_view);
    bool is_full = reserved_size == 0;
    if (is_full) {
      return {};
    }
    slots_view = slots_view.subspan(0, reserved_size);

    // 2 continuous memory areas are possible when ring buffer is
    // wrapped around, in that case we return 2 ReservedEvents
    std::vector<ReservedEvents> reserved_events;
    reserved_events.reserve(2);

    // If slots pointers are monotonically increasing, they are contiguous in
    // memory.
    if (slots_view.front() <= slots_view.back()) {
      reserved_events.emplace_back(kNotUsed, slots_view[0]->storage.data(),
                                   reserved_size, kSlotByteSize);
    } else {
      // Binary search for the first slot that wrapped around (address < front
      // address)
      auto wrap_it = std::ranges::partition_point(
          slots_view, [front = slots_view.front()](const auto* slot) {
            return slot >= front;
          });

      size_t first_area_size = std::distance(slots_view.begin(), wrap_it);
      size_t second_area_size = reserved_size - first_area_size;

      reserved_events.emplace_back(kNotUsed, slots_view[0]->storage.data(),
                                   first_area_size, kSlotByteSize);
      reserved_events.emplace_back(kNotUsed,
                                   slots_view[first_area_size]->storage.data(),
                                   second_area_size, kSlotByteSize);
    }

    return reserved_events;
  }

  void Commit(const Integer, const size_t count) override {
    queue_.ProduceCommitSlots(count);
  }

  size_t ProcessEvents() override {
    // TODO: make function parameter
    static constexpr size_t kMaxBatchSize = 256;
    std::array<MpscQueue::Slot*, kMaxBatchSize> out_buffer{};
    SlotsView slots_view{out_buffer.data(), out_buffer.size()};

    auto to_consume = queue_.ConsumeViewSlotsRoundRobin(slots_view);
    if (to_consume == 0) {
      return 0;
    }
    for (size_t i = 0; i < to_consume; ++i) {
      auto* event = std::launder(
          reinterpret_cast<IEvent*>(slots_view[i]->storage.data()));
      try {
        event->Process();
      } catch (...) {
        // FIX: don't catch ..., catch expected classes of exceptions
        //      what to do in case when exception is thrown?
      }
      // FIX: std::construct_at, std::destroy_at should be on the same level
      //      i.e. in event_processor class
      std::destroy_at(event);
    }
    queue_.ConsumeCommitSlots(to_consume);
    return to_consume;
  }

 private:
  // TODO: tune for the use case
  static constexpr size_t kSlotByteSize = 16;
  // hard limit on thread capacity according to original description, but
  // in theory can be made variable
  static constexpr size_t kThreadCapacity = 16;
  static constexpr size_t kThreadQueueCapacity = 1024;
  static constexpr size_t kNotUsed = 0;

 private:
  using MpscQueue =
      MpscQueue<kSlotByteSize, kThreadCapacity, kThreadQueueCapacity>;
  MpscQueue queue_;
};
