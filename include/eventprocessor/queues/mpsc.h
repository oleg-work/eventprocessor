#pragma once

#include "spsc.h"
#include "thread_info.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <new>

// Queue is sharded between threads, each thread has its own SPSC queue.
// Consumer thread polls all queues for items.
template <size_t SlotByteSize, size_t ThreadCapacity,
          size_t ThreadQueueCapacity>
class MpscQueue {
 public:
  using Slot = Slot<SlotByteSize>;
  using SlotsView = SlotsView<SlotByteSize>;

  // - Called by multiple producer threads concurrently [0, ThreadCapacity),
  //   otherwise UB
  // - Callers may not request another producer view
  //   before committing or abandoning the prior one, otherwise UB
  [[nodiscard]] SlotsViewSize ProduceViewSlots(SlotsView out_buffer) noexcept {
    return GetThisThreadState().queue->ProduceViewSlots(out_buffer);
  }

  // - Called by multiple producer threads concurrently [0, ThreadCapacity),
  //   otherwise UB
  // - `size` must be <= number of slots returned by the
  //   immediately preceding ProduceViewSlots call, otherwise UB
  void ProduceCommitSlots(SlotsViewSize size) noexcept {
    GetThisThreadState().queue->ProduceCommitSlots(size);
  }

  // - Called by single consumer thread, otherwise UB
  // - Callers may not request another consumer view
  //   before committing or abandoning the prior one, otherwise UB
  //    - in case of abandoning view it's assumed that slots
  //      remain valid
  [[nodiscard]] SlotsViewSize ConsumeViewSlotsRoundRobin(
      SlotsView out_buffer) noexcept {
    for (size_t i = 0; i < ThreadCapacity; ++i) {
      auto& state = GetCurrThreadState();

      auto size = state.queue->ConsumeViewSlots(out_buffer);
      if (size > 0) {
        return size;
      }
      SetNextThreadState();
    }
    return 0;
  }

  // - Called by single consumer thread, otherwise UB
  // - `size must be <= number of slots returned by the
  //   immediately preceding ConsumeViewSlots call, otherwise UB
  //    - in case of `size` < number of slots remaining slots
  //      remain valid
  void ConsumeCommitSlots(SlotsViewSize size) noexcept {
    GetCurrThreadState().queue->ConsumeCommitSlots(size);
    SetNextThreadState();
  }

 private:
  auto& GetThisThreadState() noexcept {
    // FIX: this logic isn't completly safe, because it depends
    // on the fact that only producer threads working with this
    // class call `GetIndex`. It was done to simplify overall logic.
    auto& thread_info = detail::ThreadInfo::Instance();
    const size_t index = thread_info.GetIndex();

    if (index >= ThreadCapacity) {
      std::unreachable();
    }

    return threads_state_[index];
  }

  auto& GetCurrThreadState() noexcept {
    return threads_state_[consumer_index_];
  }

  void SetNextThreadState() noexcept {
    consumer_index_ = ToIndex(consumer_index_ + 1);
  }

  size_t ToIndex(size_t i) const noexcept {
    static_assert(ThreadCapacity > 0);
    return i % ThreadCapacity;
  }

 private:
  struct ThreadState {
    using SpscQueue = SpscQueue<SlotByteSize, ThreadQueueCapacity>;
    // queues can be big
    alignas(std::hardware_destructive_interference_size)
        const std::unique_ptr<SpscQueue> queue = std::make_unique<SpscQueue>();
  };
  const std::array<ThreadState, ThreadCapacity> threads_state_;
  size_t consumer_index_{};
};
