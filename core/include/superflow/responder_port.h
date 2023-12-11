// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/connection_manager.h"
#include "superflow/policy.h"
#include "superflow/port.h"

#include <functional>
#include <variant>
#include <vector>

namespace flow
{
namespace detail
{
template<typename ReturnValue, typename... Args>
class Responder
{
public:
  using Ptr = std::shared_ptr<Responder>;

  virtual ~Responder() = default;

  virtual ReturnValue respond(Args... args, const ReturnValue*) = 0;
};

template<typename ReturnValue, typename Variant, typename... Args>
class ResponderVariant
  : protected virtual Responder<ReturnValue, Args...>
    , public Responder<Variant, Args...>
{
public:
  static_assert(
    std::is_convertible_v<ReturnValue, Variant> || std::is_constructible_v<Variant, ReturnValue>,
    "Cannot create a ResponderVariant with the Variant and ReturnValue types specified in your ResponderPort because ReturnValue is not convertible to Variant."
  );

  Variant respond(Args... args, const Variant*) override
  {
    const auto result = static_cast<Responder<ReturnValue, Args...>&>(*this).respond(args..., nullptr);

    return static_cast<Variant>(result);
  }
};
}

template<typename, typename...>
class ResponderPort;

/// \brief A port type that receives requests from a connected RequesterPort
/// and returns a response to that request
/// \tparam ReturnValue The type of response required
/// \tparam Args Any arguments required to produce the response
/// \tparam Variants Optional variant types of `ReturnValue` that are also to be accepted by
/// the responder. `ReturnValue` must be convertible to each such `Variants`. That is, either `ReturnValue`
/// must [be convertible](https://en.cppreference.com/w/cpp/types/is_convertible) to `Variants`, or `Variants`
/// must [be constructible](https://en.cppreference.com/w/cpp/types/is_constructible) from `ReturnValue`. This is
/// also useful for allowing upcasting from a derived `ReturnValue` to a parent `Variant`.
/// \see RequesterPort
/// \see \ref RequesterPort "flow::RequesterPort<ReturnValue(Args...)>"
///
/// Some examples:
/// ```cpp
/// const auto responder = std::make_shared<ResponderPort<double(int, double)>>(
///   [](const int exponent, const double base)
///   {
///     return 10*std::pow(base, static_cast<double>(exponent));
///   }
/// );
///
/// const auto requester = std::make_shared<RequesterPort<double(int, double)>>();
/// requester->connect(responder); // OK, requester and responder types match
///
/// const auto string_requester = std::make_shared<RequesterPort<std::string(int, double)>>();
/// string_requester->connect(responder); // NOT OK, string_requester expects std::string return type
///
/// const auto float_requester = std::make_shared<RequesterPort<float(int, double)>>();
/// float_requester->connect(responder); // NOT OK, float_requester expects float return type
///
/// // ---- Advanced usage below ----
///
/// // here we make a responder which returns int, but says that it is also OK to convert this int to a bool
/// const auto responder = std::make_shared<ResponderPort<int(int), bool>>( // int is implicitly convertible to bool, so this compiles fine
///   [](const int val) { return 42*val; }
/// );
/// const auto bool_requester = std::make_shared<RequesterPort<bool(int)>>();
/// bool_requester->connect(responder); // OK, int is not bool, but bool is registered as a valid conversion for responder
///
/// struct Base
/// {
///   virtual ~Base() = default;
/// };
///
/// struct Derived : public Base
/// {};
///
/// const auto derived_responder = std::make_shared<ResponderPort<Derived()>>(
///   []() { return Derived{}; }
/// );
///
/// // here we make a requester which expects a Base to be returned from the responder, but which will also
/// // accept converting (up-casting in this case) a response of type Derived
/// const auto base_requester = std::make_shared<RequesterPort<Base(), Derived>>(); // Derived is derived from Base (duh), so this compiles fine
/// base_requester->connect(derived_responder); // OK, Base is registered as a valid conversion for base_requester
/// ```
template<typename ReturnValue, typename... Args, typename... Variants>
class ResponderPort<ReturnValue(Args...), Variants...> final :
    public Port,
    public virtual detail::Responder<ReturnValue, Args...>,
    public detail::ResponderVariant<ReturnValue, Variants, Args...>...,
    public std::enable_shared_from_this<Port>
{
public:
  using Ptr = std::shared_ptr<ResponderPort>;

  /// \brief Create a new SlavePort
  /// \param callback The function to be called as a response to requests
  explicit ResponderPort(std::function<ReturnValue(Args...)> callback)
      : callback_{callback}
  {}

  ~ResponderPort() override = default;

  void connect(const Port::Ptr& ptr) override;

  void disconnect() noexcept override;

  void disconnect(const Port::Ptr& ptr) noexcept override;

  bool isConnected() const override;

  PortStatus getStatus() const override;

  /// \brief Respond to a request from a MasterPort
  /// \param args Any arguments required by the slave to produce the response
  /// \return The response
  ReturnValue respond(Args... args);

  ReturnValue respond(Args... args, const ReturnValue*) override;

private:
  ConnectionManager<ConnectPolicy::Multi> connection_manager_;
  size_t num_transactions_ = 0;
  std::function<ReturnValue(Args...)> callback_;
};

// ----- Implementation -----
template<typename ReturnValue, typename... Args, typename... Variants>
void ResponderPort<ReturnValue(Args...), Variants...>::connect(const Port::Ptr& ptr)
{
  connection_manager_.connect(shared_from_this(), ptr);
}

template<typename ReturnValue, typename... Args, typename... Variants>
bool ResponderPort<ReturnValue(Args...), Variants...>::isConnected() const
{
  return connection_manager_.isConnected();
}

template<typename ReturnValue, typename... Args, typename... Variants>
void ResponderPort<ReturnValue(Args...), Variants...>::disconnect() noexcept
{
  connection_manager_.disconnect(shared_from_this());
}

template<typename ReturnValue, typename... Args, typename... Variants>
void ResponderPort<ReturnValue(Args...), Variants...>::disconnect(const Port::Ptr& ptr) noexcept
{
  connection_manager_.disconnect(shared_from_this(), ptr);
}

template<typename ReturnValue, typename... Args, typename... Variants>
PortStatus ResponderPort<ReturnValue(Args...), Variants...>::getStatus() const
{
  return {
      connection_manager_.getNumConnections(),
      num_transactions_
  };
}

template<typename ReturnValue, typename... Args, typename... Variants>
ReturnValue ResponderPort<ReturnValue(Args...), Variants...>::respond(Args... args)
{
  return respond(args..., nullptr);
}

template<typename ReturnValue, typename... Args, typename... Variants>
ReturnValue ResponderPort<ReturnValue(Args...), Variants...>::respond(Args... args, const ReturnValue*)
{
  if constexpr (std::is_same_v<ReturnValue, void>)
  {
    callback_(args...);
    ++num_transactions_;
  }
  else
  {
    const auto value = callback_(args...);
    ++num_transactions_;

    return value;
  }
}
}
