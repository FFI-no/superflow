// Copyright (c) 2019 Forsvarets forskningsinstitutt (FFI). All rights reserved.
#pragma once

#include <functional>
#include <mutex>

namespace flow
{
/// \brief A wrapper class for protecting an object with a mutex.
/// \note Note that since Mutexed is derived from T, T cannot be pointer or refrence.
///
/// \code{.cpp}
/// Mutexed<std::string> mutexed("hello");
/// {
///   std::scoped_lock lock{mutexed};
///   mutexed = "hello";
/// }
///
/// mutexed.store("bye");
/// std::string bye = mutexed.load();
///
/// mutexed.read([&bye](auto const& str){ bye = str; }
/// mutexed.write([](auto& str){ str = "hello"; }
/// \endcode
/// \tparam T The type of object you want to protect.
/// \see http://herbsutter.com/2013/01/01/video-you-dont-know-const-and-mutable/
/// \see https://stackoverflow.com/questions/4127333/should-mutexes-be-mutable/4128689#4128689
template<typename T>
class Mutexed : public T
{
public:
  /// \brief Construct a new Mutexed<T>
  using T::T;

  /// \brief Assign a new value to the mutexed object, interpreted as a T.
  /// \note NOT threadsafe! Must be used in a manner like this:
  /// Mutexed<std::string> mutexed("hello");
  /// {
  ///   std::scoped_lock lock{mutexed};
  ///   mutexed = "hello";
  /// }
  /// \see store, write
  /// \return
  template<typename... Args>
  Mutexed& operator=(Args&& ... args)
  {
    T::operator=(std::forward<Args>(args)...);
    return *this;
  }

  /// Intentionally slice object from type 'Mutexed<T>' to 'T'.
  [[nodiscard]] T slice() const
  { return *this; }

  /// Intentionally slice object from type 'Mutexed<T>' to 'T'.
  [[nodiscard]] T& slice()
  { return *this; }

  /// \brief Thread safe assignment of a new value to Mutexed<T>
  /// \tparam Args any types applicable to the creation of T
  /// \param args any arguments applicable to the creation of T
  template<typename... Args>
  void store(Args&& ... args)
  {
    std::scoped_lock lock{*this};
    T::operator=(std::forward<Args>(args)...);
  }

  /// Thread safe slicing of object from type 'Mutexed<T>' to 'T'.
  /// Will create a copy, so this method is not applicable to non-copyable 'T's
  /// \return
  [[nodiscard]] T load() const
  {
    std::scoped_lock lock{*this};
    return slice();
  }

  /// \brief Provides unique access to the data by giving a `const T&` to the reader function.
  /// A unique lock on the mutex is acquired prior to calling the supplied reader function,
  /// so multiple simultaneous calls to read is not possible.
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

    std::scoped_lock lock{*this};
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

private:
  mutable std::mutex mu_;
};
}
