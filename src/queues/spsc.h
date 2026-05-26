#pragma once

#include <array>
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <new>
#include <span>

// TODO: for further optimizations https://rigtorp.se/ringbuffer/

// For types constructed inside:
//   sizeof(T) <= slot_size
//   alignof(T) <= slot_align
// Otherwise UB
template <size_t SlotByteSize>
struct Slot {
  std::array<std::byte, SlotByteSize> storage;
};

template <size_t SlotByteSize>
using SlotsView = std::span<Slot<SlotByteSize>*>;
using SlotsViewSize = size_t;

template <size_t SlotByteSize, size_t Capacity>
class SpscQueue {
 public:
  using Slot = Slot<SlotByteSize>;
  using SlotsView = SlotsView<SlotByteSize>;

  // Callers may not request another producer view
  // before committing or abandoning the prior one, otherwise UB
  [[nodiscard]] SlotsViewSize ProduceViewSlots(SlotsView out_buffer) noexcept {
    auto h = head_.load(std::memory_order::acquire);
    auto t = tail_.load(std::memory_order::relaxed);

    const size_t free_space = Capacity - Size(h, t);
    if (free_space == 0) {
      return 0;
    }

    const size_t to_grab = std::min(out_buffer.size(), free_space);
    for (size_t i = 0; i < to_grab; ++i) {
      out_buffer[i] = &buffer_[ToIndex(t + i)];
    }

    return to_grab;
  }

  // `size` must be <= number of slots returned by the
  // immediately preceding ProduceViewSlots call, otherwise UB
  void ProduceCommitSlots(SlotsViewSize size) noexcept {
    tail_.fetch_add(size, std::memory_order::release);
  }

  // Callers may not request another consumer view
  // before committing or abandoning the prior one, otherwise UB
  //  - in case of abandoning view it's assumed that slots
  //    remain valid
  [[nodiscard]] SlotsViewSize ConsumeViewSlots(SlotsView out_buffer) noexcept {
    auto h = head_.load(std::memory_order::relaxed);
    auto t = tail_.load(std::memory_order::acquire);

    const size_t used_space = Size(h, t);
    if (used_space == 0) {
      return 0;
    }

    const size_t to_grab = std::min(out_buffer.size(), used_space);
    for (size_t i = 0; i < to_grab; ++i) {
      out_buffer[i] = &buffer_[ToIndex(h + i)];
    }

    return to_grab;
  }

  // `size must be <= number of slots returned by the
  // immediately preceding ConsumeViewSlots call, otherwise UB
  //  - in case of `size` < number of slots remaining slots
  //    remain valid
  void ConsumeCommitSlots(SlotsViewSize size) noexcept {
    head_.fetch_add(size, std::memory_order::release);
  }

 private:
  static size_t Size(size_t h, size_t t) noexcept {
    return t - h;
  }

  static size_t ToIndex(size_t i) noexcept {
    constexpr bool kIsCapacityPowerOfTwo =
        Capacity > 0 && (Capacity & (Capacity - 1)) == 0;
    static_assert(
        kIsCapacityPowerOfTwo,
        "Capacity must be a power of two for efficient modulo operation.");
    return i % Capacity;
  }

 private:
  std::array<Slot, Capacity> buffer_;
  alignas(std::hardware_destructive_interference_size)
      std::atomic<size_t> head_ = 0;
  alignas(std::hardware_destructive_interference_size)
      std::atomic<size_t> tail_ = 0;
};
