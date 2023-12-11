// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/port_status.h"

#include <map>
#include <ostream>
#include <string>

namespace flow
{
struct ProxelStatus
{
  enum class State
  {
    AwaitingInput,
    AwaitingRequest,
    AwaitingResponse,
    Crashed,
    NotConnected,
    Paused,
    Running,
    Unavailable,
    Undefined,
    Warning,
  };

  State state;
  std::string info;
  std::map<std::string, PortStatus> ports;
};

/// A map with the latest ProxelStatuses, as key/value pairs such as { proxel_name: ProxelStatus }
using ProxelStatusMap = std::map<std::string, ProxelStatus>;

/// \brief Write enum ProxelStatus as string to a std::ostream
inline std::ostream& operator<<(std::ostream& os, const ProxelStatus::State& state)
{
  switch (state)
  {
    case ProxelStatus::State::AwaitingInput:
      return os << "NO INPUT";
    case ProxelStatus::State::AwaitingRequest:
      return os << "NO REQUEST";
    case ProxelStatus::State::AwaitingResponse:
      return os << "NO RESPONSE";
    case ProxelStatus::State::Crashed:
      return os << "CRASHED";
    case ProxelStatus::State::NotConnected:
      return os << "NOT CONNECTED";
    case ProxelStatus::State::Paused:
      return os << "PAUSED";
    case ProxelStatus::State::Running:
      return os << "RUNNING";
    case ProxelStatus::State::Unavailable:
      return os << "UNAVAILABLE";
    case ProxelStatus::State::Warning:
      return os << "WARNING";
    default:
      return os << "UNDEFINED";
  }
}
}
