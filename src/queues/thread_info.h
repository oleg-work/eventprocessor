#pragma once

#include <atomic>
#include <cstddef>

namespace detail {

class ThreadInfo {
 public:
  static ThreadInfo& Instance() noexcept {
    static ThreadInfo instance;
    return instance;
  }

  size_t GetIndex() noexcept {
    static thread_local const size_t kIndex =
        next_thread_index_.fetch_add(1, std::memory_order_relaxed);
    return kIndex;
  }

 private:
  ThreadInfo() noexcept = default;

 private:
  std::atomic<size_t> next_thread_index_{0};
};

}  // namespace detail
