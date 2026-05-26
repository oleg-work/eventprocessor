#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

// NOTE: not sure that's optimal interface for required functionality, but
//       there are few details about how this class is being used
//       therefore I tried to stick as much as I could to interface that was
//       provided.

// Assumptions about implementation:
// - I made most of the functions virtual because of the name of the class,
//   and examples provided;
// - I also added extra function `ProcessEvents`, so I don't have to add
//   `thread` dependency;

class IEventProcessor {
 public:
  using Integer = int64_t;

  //////////////////////////////////////////////////////////////////////////
  /// ---
  struct ReservedEvent {
    ReservedEvent() = default;

    ReservedEvent(const Integer sequence_number, void* const event)
        : sequence_number_(sequence_number),
          event_(event) {
    }

    ReservedEvent(const ReservedEvent&) = delete;
    ReservedEvent& operator=(const ReservedEvent&) = delete;

    ReservedEvent(ReservedEvent&&) = default;
    ReservedEvent& operator=(ReservedEvent&&) = default;

    ~ReservedEvent() = default;

    Integer GetSequenceNumber() const {
      return sequence_number_;
    }
    void* GetEvent() const {
      return event_;
    }
    bool IsValid() const {
      return event_ != nullptr;
    }

   private:
    Integer sequence_number_{};
    void* event_{};  // doesn't own this memory
  };

  //////////////////////////////////////////////////////////////////////////
  /// ---
  struct ReservedEvents {
    ReservedEvents(const Integer sequence_number, void* const event,
                   const size_t count, const size_t event_size)
        : sequence_number_(sequence_number),
          events_(event),
          count_(count),
          event_size_(event_size) {
    }

    ReservedEvents(const ReservedEvents&) = delete;
    ReservedEvents& operator=(const ReservedEvents&) = delete;

    ReservedEvents(ReservedEvents&&) = default;
    ReservedEvents& operator=(ReservedEvents&&) = default;

    ~ReservedEvents() = default;

    template <class TEvent, class... Args>
    void Emplace(const size_t index, Args&&... args) {
      auto event = static_cast<TEvent*>(GetEvent(index));
      if (event) {
        std::construct_at(event, std::forward<Args>(args)...);
      }
    }

    Integer GetSequenceNumber() const {
      return sequence_number_;
    }

    void* GetEvent(const size_t index) const {
      return reinterpret_cast<std::byte*>(events_) + (index * event_size_);
    }

    size_t Count() const {
      return count_;
    }

    bool IsValid() const {
      return events_ != nullptr;
    }

   private:
    Integer sequence_number_{};
    void* events_{};  // doesn't own this memory
    size_t count_{};
    size_t event_size_{};
  };

  using ReserveResult = std::pair<size_t, void* const>;

  template <typename T>
  ReserveResult ReserveEvent() {
    return ReserveEvent(sizeof(T));
  }
  virtual ReserveResult ReserveEvent(size_t type_size) = 0;

  //////////////////////////////////////////////////////////////////////////
  /// ---
  template <class T, class... Args>
  ReservedEvent Reserve(Args&&... args) {
    const auto reservation = ReserveEvent<T>();

    if (!reservation.second) {
      return ReservedEvent();
    }

    std::construct_at(static_cast<T*>(reservation.second),
                      std::forward<Args>(args)...);

    return ReservedEvent(reservation.first, reservation.second);
  }

  //////////////////////////////////////////////////////////////////////////
  /// ---
  // TODO: static vector?
  virtual std::vector<ReservedEvents> ReserveRange(const size_t size) = 0;

  //////////////////////////////////////////////////////////////////////////
  /// ---
  virtual void Commit(const Integer sequence_number) = 0;

  //////////////////////////////////////////////////////////////////////////
  /// ---
  virtual void Commit(const Integer sequence_number, const size_t count) = 0;

  // returns events processed
  virtual size_t ProcessEvents() = 0;

  virtual ~IEventProcessor() = default;
};
