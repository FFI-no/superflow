// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/port.h"
#include "superflow/responder_port.h"

#include <functional>
#include <variant>
#include <vector>

namespace flow
{
template<typename, typename...>
class RequesterPort;

/// \brief A port type that can request a response from a connected ResponderPort
/// \tparam ReturnValue The type of response required
/// \tparam Args Any arguments required by the slave to produce the response
/// \tparam Variants Optional variant types of `ReturnValue` that are also to be accepted by
/// the responder. `ReturnValue` must be convertible to each such `Variants`. That is, either `ReturnValue`
/// must [be convertible](https://en.cppreference.com/w/cpp/types/is_convertible) to `Variants`, or `Variants`
/// must [be constructible](https://en.cppreference.com/w/cpp/types/is_constructible) from `ReturnValue`. This is
/// also useful for allowing upcasting from a derived `ReturnValue` to a parent `Variant`.
/// \see ResponderPort
/// \see \ref ResponderPort "flow::ResponderPort<ReturnValue(Args...)>"
template<typename ReturnValue, typename... Args, typename... Variants>
class RequesterPort<ReturnValue(Args...), Variants...> :
    public Port,
    public std::enable_shared_from_this<Port>
{
public:
  using Ptr = std::shared_ptr<RequesterPort>;
  ~RequesterPort() override = default;

  void connect(const Port::Ptr& ptr) final;

  void disconnect() noexcept final;

  void disconnect(const Port::Ptr& ptr) noexcept final;

  PortStatus getStatus() const final;

  bool isConnected() const final;

  /// \brief Request a new response from the slave
  /// \param args Any arguments required by the slave to produce the response
  /// \return The response
  ReturnValue request(Args... args)
  {
    if (responder_)
    {
      ++num_transactions_;

      return responder_->respond(args..., nullptr);
    } else
    { throw std::runtime_error("RequesterPort has no connection"); }
  }

  /// \brief Overload for request
  /// \param args Any arguments required by the slave to produce the response
  /// \return The response
  ReturnValue operator()(Args... args)
  { return request(args...); }

private:
  using Responder = detail::Responder<ReturnValue, Args...>;
  using ResponderPtr = typename Responder::Ptr;

  size_t num_transactions_ = 0;

  Port::Ptr connection_;
  ResponderPtr responder_;

  template<typename Variant>
  class ResponderShim : public detail::Responder<ReturnValue, Args...>
  {
  public:
    static_assert(
      std::is_convertible_v<Variant, ReturnValue> || std::is_constructible_v<ReturnValue, Variant>,
      "Cannot create a RequesterPortPort<ReturnValue(Args...), Variant> with support for the given Variant-type. Variant is not convertible to ReturnValue."
    );

    using ResponderPtr = typename detail::Responder<Variant>::Ptr;

    explicit ResponderShim(const ResponderPtr& variant_responder)
      : variant_responder_{variant_responder}
    {}

    ReturnValue respond(Args... args, const ReturnValue*) final
    {
      return static_cast<ReturnValue>(variant_responder_->respond(args..., nullptr));
    }

  private:
    ResponderPtr variant_responder_;
  };

  template<typename Variant>
  [[nodiscard]] static bool getResponderPtr(
    const Port::Ptr& ptr,
    ResponderPtr& responder
  );
};

/// \brief Partial template specialization for the case when `ReturnValue` is a [`std::variant`](https://en.cppreference.com/w/cpp/utility/variant).
/// For further details, \see RequesterPort.
template<typename... Variants, typename... Args>
class RequesterPort<std::variant<Variants...>(Args...)> :
  public RequesterPort<std::variant<Variants...>(Args...), Variants...>
{};

// ----- Implementation -----
template<typename ReturnValue, typename... Args, typename... Variants>
void RequesterPort<ReturnValue(Args...), Variants...>::connect(const Port::Ptr& ptr)
{
  if (connection_ == ptr)
  {
    return;
  }

  ResponderPtr responder;
  getResponderPtr<ReturnValue>(ptr, responder) || (getResponderPtr<Variants>(ptr, responder) || ...);

  if (responder == nullptr)
  { throw std::invalid_argument{std::string("Type mismatch when connecting ports")}; }

  if (isConnected())
  { throw std::runtime_error{"The RequesterPort has already an active connection."}; }

  responder_ = responder;
  connection_ = ptr;
  ptr->connect(shared_from_this());
}

template<typename ReturnValue, typename... Args, typename... Variants>
void RequesterPort<ReturnValue(Args...), Variants...>::disconnect() noexcept
{
  if (connection_ == nullptr)
  {
    return;
  }

  responder_.reset();
  auto connection = std::move(connection_);
  connection->disconnect();
}

template<typename ReturnValue, typename... Args, typename... Variants>
void RequesterPort<ReturnValue(Args...), Variants...>::disconnect(const Port::Ptr& ptr) noexcept
{
  if (connection_ != ptr)
  {
    return;
  }

  disconnect();
}

template<typename ReturnValue, typename... Args, typename... Variants>
PortStatus RequesterPort<ReturnValue(Args...), Variants...>::getStatus() const
{
  return {
      isConnected() ? 1u : 0u,
      num_transactions_
  };
}

template<typename ReturnValue, typename... Args, typename... Variants>
bool RequesterPort<ReturnValue(Args...), Variants...>::isConnected() const
{
  return connection_ != nullptr;
}

template<typename ReturnValue, typename... Args, typename... Variants>
template<typename Variant>
bool RequesterPort<ReturnValue(Args...), Variants...>::getResponderPtr(
  const Port::Ptr& ptr,
  ResponderPtr& responder
)
{
  const auto variant_responder = std::dynamic_pointer_cast<detail::Responder<Variant, Args...>>(ptr);

  if (variant_responder == nullptr)
  {
    responder = nullptr;

    return false;
  }

  if constexpr (std::is_same_v<ReturnValue, Variant>)
  {
    responder = variant_responder;
  }
  else
  {
    responder = std::make_shared<ResponderShim<Variant>>(variant_responder);
  }

  return true;
}
}
