// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <ciso646>
#include <iterator>
#include <optional>

namespace flow
{
/// \brief Interface for classes that will continuosly produce data.
/// Defines a convenient extract stream operator for those classes,
/// facilitating statements like `for(T item; stream >> item;){}`
/// \tparam T
template<typename T>
class DataStream
{
public:
  virtual ~DataStream() = default;

  /// \brief Request the next item from the stream.
  /// \param item The element that will contain the newly retreived data.
  /// \return Should return true if item contains valid data.
  virtual std::optional<T> getNext() = 0;

  /// \brief Tells whether the stream is valid, i.e. is alive and produces valid data.
  /// \return Should return true if the stream is valid.
  virtual operator bool() const = 0;

  class Iterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    explicit Iterator(DataStream& stream) //NOLINT
        : Iterator{stream, !stream}
    {}

    Iterator(DataStream& stream, const bool is_end)
        : stream_{stream}
        , is_end_{is_end}
    {
      ++*this;
    }

    Iterator& operator++()
    {
      if (!is_end_)
      {
        t_ = stream_.getNext();
        is_end_ = not t_.has_value();
      }

      return *this;
    }

    Iterator operator++(int)
    {
      auto retval = *this;

      ++(*this);

      return retval;
    }

    bool operator==(const Iterator& other) const
    {
      if (is_end_ || !stream_)
      { return other.is_end_ || !other.stream_; }
      else
      { return !other.is_end_ && other.stream_; }
    }

    bool operator!=(const Iterator& other) const
    {
      return !(*this == other);
    }

    constexpr const T& operator*() const & { return *t_; }

    T& operator*() & { return *t_; }

    T&& operator*() && { return std::move(*t_); }

  private:
    DataStream& stream_;
    bool is_end_;
    std::optional<T> t_;
  };

  Iterator begin()
  {
    return Iterator{*this};
  }

  Iterator end()
  {
    return {*this, true};
  }
};

/// \brief Extract operator for DataStream
/// \tparam T the type of data in the stream.
/// \param stream the stream.
/// \param item element containing the received data
/// \return the stream.
template<typename T>
inline DataStream<T>& operator>>(DataStream<T>& stream, T& item)
{
  if (auto data = stream.getNext())
  { item = std::move(*data); }

  return stream;
}
}
