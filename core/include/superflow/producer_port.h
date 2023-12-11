// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/consumer_port.h"
#include "superflow/port.h"

#include <map>
#include <memory>
#include <stdexcept>

namespace flow
{
/// \brief An output port able to connect with multiple ports extending Consumer.
/// \tparam T The type of data to be exchanged between the ports.
/// \tparam Variants... Optional downstream variant types of T which is also to be accepted by
/// the producer. Each such `Variants` must be const ref convertible to `T`, that is
/// `const Variant&` must [be convertible](https://en.cppreference.com/w/cpp/types/is_convertible) to
/// `const T&`. This is also useful for allowing upcasting from a derived class
/// to a parent baseclass.
///
/// Some examples:
/// ```cpp
/// const auto producer = std::make_shared<ProducerPort<int>>();
///
/// const auto int_consumer = std::make_shared<BufferedConsumerPort<int>>();
/// producer->connect(int_consumer); // OK, producer and consumer types match
///
/// const auto str_consumer = std::make_shared<BufferedConsumerPort<std::string>>();
/// producer->connect(str_consumer); // NOT OK, int is not std::string, will throw
///
/// const auto bool_consumer = std::make_shared<BufferedConsumerPort<bool>>();
/// producer->connect(bool_consumer); // NOT OK, int is not bool, will throw
///
/// // ---- Advanced usage below ----
///
/// const auto conv_producer = std::make_shared<ProducerPort<int, bool>>(); // int is implicitly convertible to bool, so this compiles fine
/// conv_producer->connect(bool_consumer); // OK, int is not bool, but bool is registered as a valid conversion for conv_producer
///
/// struct Base
/// {
///   virtual ~Base() = default;
/// };
///
/// struct Derived : public Base
/// {};
///
/// const auto base_consumer = std::make_shared<BufferedConsumerPort<Base>>();
/// const auto derived_producer = std::make_shared<ProducerPort<Derived, Base>>(); // Derived is derived from Base (duh), so this compiles fine
/// derived_producer->connect(base_consumer); // OK, Base is registered as a valid conversion for derived_producer
/// ```
template<typename T, typename... Variants>
class ProducerPort final :
    public Port,
    public std::enable_shared_from_this<Port>
{
public:
  using Ptr = std::shared_ptr<ProducerPort>;
  /// \brief Send data to all connected consumers.
  /// If the consumer reports to not accept data, the connection is removed.
  void send(const T&);

  /// \brief Connect port to a new consumer.
  /// \param ptr A pointer to the connecting port.
  /// \throws std::invalid_argument if ptr is incompatible.
  void connect(const Port::Ptr& ptr) override;

  /// \brief Disconnect all consumers.
  /// \note Not thread safe! Should not be called while some other thread is sending data.
  void disconnect() noexcept override;

  /// \brief Disconnect one consumers.
  /// \param ptr A pointer to the other port that will be disconnected
  /// \note Not thread safe! Should not be called while some other thread is sending data.
  void disconnect(const Port::Ptr& ptr) noexcept override;

  bool isConnected() const override;

  /// \brief Request the number of consumers currently connected.
  /// \return
  size_t numConnections() const;

  PortStatus getStatus() const override;

private:
  using Consumer = detail::Consumer<T>;
  using ConsumerPtr = typename Consumer::Ptr;
  using Connection = std::pair<Port::Ptr, ConsumerPtr>;

  template<typename Base>
  class ConsumerShim : public detail::Consumer<T>
  {
  public:
    static_assert(
      std::is_convertible_v<const T&, const Base&> || std::is_constructible_v<Base, const T&>,
      "Cannot create a ProducerPort<T, Base> with support for the given Base-type. const T& is not convertible to const Base&."
    );

    using ConsumerPtr = typename detail::Consumer<Base>::Ptr;

    explicit ConsumerShim(const ConsumerPtr& base_consumer)
      : base_consumer_{base_consumer}
    {}

    void receive(const T& data, const Port::Ptr& port) final
    {
      if constexpr (std::is_convertible_v<const T&, const Base&>)
      {
        base_consumer_->receive(static_cast<const Base&>(data), port);
      }
      else
      {
        // Since const T& is not convertible to const Base&, Base must be
        // constructible from const T&. This is less efficient than casting refs.

        base_consumer_->receive(static_cast<Base>(data), port);
      }
    }

  private:
    ConsumerPtr base_consumer_;
  };

  /// \brief Helper method for getting a pointer to a Consumer<T> that feeds
  /// into a Consumer<Base>, from a port. If `T` and `Base` is the same this is
  /// a simple cast, otherwise a ConsumerShim is inserted. `consumer` is set to
  /// the resulting consumer, or `nullptr` if no such shim is possible.
  /// \tparam Base
  /// \param ptr
  /// \param consumer The resulting consumer, or `nullptr` if no such consumer exists.
  /// \return `true` if a valid consumer was successfully found.
  template<typename Base>
  [[nodiscard]] static bool getConsumerPtr(
    const Port::Ptr& ptr,
    ConsumerPtr& consumer
  );

  size_t num_transactions_ = 0;
  std::map<Port::Ptr, ConsumerPtr> consumers_;

  bool hasConnection(const Port::Ptr& ptr) const;
};

// ----- Implementation -----
template<typename T, typename... Variants>
void ProducerPort<T, Variants...>::connect(const Port::Ptr& ptr)
{
  ConsumerPtr consumer;
  getConsumerPtr<T>(ptr, consumer) || (getConsumerPtr<Variants>(ptr, consumer) || ...);

  if (consumer == nullptr)
  { throw std::invalid_argument{std::string("Type mismatch when connecting ports")}; }

  if (hasConnection(ptr))
  {
    // already connected, do nothing
    return;
  }

  consumers_[ptr] = consumer;
  ptr->connect(shared_from_this());
}

template<typename T, typename... Variants>
void ProducerPort<T, Variants...>::disconnect() noexcept
{
  auto consumers = std::move(consumers_);

  for (auto& kv : consumers)
  {
    kv.first->disconnect();
  }
}

template<typename T, typename... Variants>
void ProducerPort<T, Variants...>::disconnect(const Port::Ptr& ptr) noexcept
{
  auto it = consumers_.find(ptr);

  if (it == consumers_.end())
  {
    return;
  }

  const auto port = it->first;
  consumers_.erase(it);

  port->disconnect(shared_from_this());
}

template<typename T, typename... Variants>
bool ProducerPort<T, Variants...>::isConnected() const
{
  return numConnections() > 0;
}

template<typename T, typename... Variants>
size_t ProducerPort<T, Variants...>::numConnections() const
{
  return consumers_.size();
}

template<typename T, typename... Variants>
PortStatus ProducerPort<T, Variants...>::getStatus() const
{
  return {
      numConnections(),
      num_transactions_
  };
}

template<typename T, typename... Variants>
void ProducerPort<T, Variants...>::send(const T& t)
{
  ++num_transactions_;

  for (const auto& kv : consumers_)
  {
    kv.second->receive(t, shared_from_this());
  }
}

template<typename T, typename... Variants>
bool ProducerPort<T, Variants...>::hasConnection(const Port::Ptr& ptr) const
{
  return consumers_.find(ptr) != consumers_.end();
}

template<typename T, typename... Variants>
template<typename Base>
bool ProducerPort<T, Variants...>::getConsumerPtr(
  const Port::Ptr& ptr,
  ConsumerPtr& consumer
)
{
  const auto base_consumer = std::dynamic_pointer_cast<detail::Consumer<Base>>(ptr);

  if (base_consumer == nullptr)
  {
    consumer = nullptr;

    return false;
  }

  if constexpr (std::is_same_v<T, Base>)
  {
    consumer = base_consumer;
  }
  else
  {
    consumer = std::make_shared<ConsumerShim<Base>>(base_consumer);
  }

  return true;
}
}
