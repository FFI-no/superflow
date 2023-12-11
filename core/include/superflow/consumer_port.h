// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/port.h"

#include <variant>

namespace flow
{
namespace detail
{
template<typename T>
class Consumer
{
public:
  using Ptr = std::shared_ptr<Consumer>;
  using Type = T;

  virtual ~Consumer() = default;

  /// \brief Function to be called by ProducerPort in order to send data to the ConsumerPort
  /// \param data The data sent by ProducerPort
  /// \param port Pointer to the ProducerPort sending data
  virtual void receive(const T& data, const Port::Ptr& port) = 0;
};

template<typename Base, typename Variant>
class ConsumerVariant
  : protected virtual Consumer<Base>
  , public Consumer<Variant>
{
public:
  static_assert(
    std::is_convertible_v<const Variant&, const Base&> || std::is_constructible_v<Base, const Variant&>,
    "Cannot create a ConsumerVariant with the Variant and Base types specified in your ConsumerPort because const Variant& is not convertible to const Base&."
  );

  void receive(const Variant& data, const Port::Ptr& port) final
  {
    using BaseConsumer = Consumer<Base>;
    if constexpr (std::is_convertible_v<const Variant&, const Base&>)
    {
      static_cast<BaseConsumer&>(*this).receive(static_cast<const Base&>(data), port);
    }
    else
    {
      // Since const Variant& is not convertible to const Base&, Base must be
      // constructible from const Variant&. This is less efficient than casting refs.

      static_cast<BaseConsumer&>(*this).receive(static_cast<Base>(data), port);
    }
  }
};
}

/// \brief  Interface for input ports able to connect with ProducerPort.
/// \tparam T The base type (e.g. "output type") of data of the consumer.
/// \tparam Variants... Optional upstream variant types of T which are also to be accepted by
/// the consumer. Each such `Variants` must be const ref convertible to `T`, that is
/// `const Variant&` must [be convertible](https://en.cppreference.com/w/cpp/types/is_convertible)
/// to `const T&`. This is also useful for allowing upcasts from a derived class
/// to a parent baseclass.
/// \see ProducerPort
/// \see BufferedConsumerPort
/// \see CallbackConsumerPort
/// \see MultiConsumerPort
template<typename T, typename... Variants>
class ConsumerPort :
  public Port,
  public virtual detail::Consumer<T>,
  public detail::ConsumerVariant<T, Variants>...
{
public:
  using Ptr = std::shared_ptr<ConsumerPort>;
};

/// \brief Partial template specialization for the case when `T` is a [`std::variant`](https://en.cppreference.com/w/cpp/utility/variant).
/// For further details, \see ConsumerPort.
template<typename... Variants>
class ConsumerPort<std::variant<Variants...>> :
  public ConsumerPort<std::variant<Variants...>, Variants...>
{};
}
