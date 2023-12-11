// Copyright (c) 2019 Forsvarets forskningsinstitutt (FFI). All rights reserved.
#pragma once

#include <functional>
#include <mutex>
#include <shared_mutex>

namespace flow
{
/// \brief Exactly the same as Mutexed, except with a `shared_mutex` that
/// allows multiple concurrent reads.
/// It is better suited for scenarios were read operations are frequent and expensive.
/// \see Mutexed
/// \note Note that performing expensive operations while holding a lock is often a bad sign.
/// There may be better ways to solve the problem than using a SharedMutexed.
template<typename T>
class SharedMutexed : public T
{
public:
  /// \brief Construct a new SharedMutexed<T>
  /// \tparam Args any types applicable to the constructor of T
  /// \param args any arguments applicable to the constructor of T
  template<typename... Args>
  explicit SharedMutexed(Args&& ... args)
    : T(std::forward<Args>(args)...)
  {}

  /// \brief Assign a new value to the mutexed object, interpreted as a T.
  /// \note NOT threadsafe! Must be used in a manner like this:
  /// SharedMutexed<std::string> mutexed("hello");
  /// {
  ///   std::scoped_lock lock{mutexed};
  ///   mutexed = "hello";
  /// }
  /// \tparam Args any types applicable to the assignment of T
  /// \param args any arguments applicable to the assignment of T
  /// \see store, write
  /// \return
  template<typename... Args>
  SharedMutexed& operator=(Args&& ... args)
  {
    T& t = *this;
    t = {std::forward<Args>(args)...};
    return *this;
  }

  /// Intentionally slice object from type 'SharedMutexed<T>' to 'T'.
  [[nodiscard]] T slice() const
  { return *this; }

  /// Intentionally slice object from type 'SharedMutexed<T>' to 'T'.
  [[nodiscard]] T& slice()
  { return *this; }

  /// \brief Thread safe assignment of a new value to SharedMutexed<T>
  /// \tparam Args any types applicable to the creation of T
  /// \param args any arguments applicable to the creation of T
  template<typename... Args>
  void store(Args&& ... args)
  {
    T& t = *this;
    std::scoped_lock lock{*this};
    t = {std::forward<Args>(args)...};
  }

  /// Thread safe slicing of object from type 'SharedMutexed<T>' to 'T'.
  /// Will create a copy, so this method is not applicable to non-copyable 'T's.
  /// Concurrent load operations are possible
  /// \return
  [[nodiscard]] T load() const
  {
    std::shared_lock lock{*this};
    return slice();
  }

  /// \brief Provides shared access to the data by giving a `const T&` to the reader function.
  /// A shared lock on the mutex is acquired prior to calling the supplied reader function,
  /// so multiple simultaneous calls to read is possible.
  /// \tparam Invokable any invokable type that is invokable with `const T&` as argument
  /// \param reader the invocable (a function or lambda, typically)
  /// \return whatever the Invokable returns
  template<typename Invokable>
  auto read(const Invokable& reader) const
  {
    static_assert(
      std::is_invocable_v<decltype(reader), T const&>,
      "The provided `reader` is not invokable with `const T&` as argument"
    );

    std::shared_lock lock{*this};
    return std::invoke(reader, slice());
  }

  /// \brief Provides unique access to the data by giving a `T&` to the writer function.
  /// A unique lock on the mutex is acquired prior to calling the supplied writer function,
  /// so simultaneous calls to read or write is not possible.
  /// \tparam Invokable any invokable type that is invokable with `T&` as argument
  /// \param writer the invocable (a function or lambda, typically)
  /// \return whatever the Invokable returns
  template<typename Invokable>
  auto write(const Invokable& writer)
  {
    static_assert(
      std::is_invocable_v<decltype(writer), T&>,
      "The provided `writer` is not invokable with `T&` as argument"
    );

    std::scoped_lock lock{*this};
    return std::invoke(writer, slice());
  }

  void lock() const ///< C++ named requirement: Mutex
  { this->mu_.lock(); }

  void unlock() const ///< C++ named requirement: Mutex
  { this->mu_.unlock(); };

  bool try_lock() const noexcept ///< C++ named requirement: Mutex
  { return this->mu_.try_lock(); }

  void lock_shared() const ///< C++ named requirement: SharedMutex
  { this->mu_.lock_shared(); }

  void unlock_shared() const ///< C++ named requirement: SharedMutex
  { this->mu_.unlock_shared(); };

  bool try_lock_shared() const noexcept ///< C++ named requirement: SharedMutex
  { return this->mu_.try_lock_shared(); }

private:
  mutable std::shared_mutex mu_;
};
}
