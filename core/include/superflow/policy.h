// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

namespace flow
{
enum class GetMode
{
  Blocking,     ///< Attempts to retreive data from the buffer is blocking, so that the consumer will
                ///  wait until new data is added to the buffer. The wait will be aborted only if the
                ///  consumer is deactivated.
  Latched,      ///< If buffer is empty, latched mode acts like Blocking. Else, the first data in the
                ///  buffer will be read. With buffer size 1 you will always get newest data available.
  ReadyOnly,    ///< When connected to multiple producers one would usually wait for all of them to
                ///  produce data. This mode fetches only from ready producers, i.e. not necessary all
  AtLeastOneNew ///< Similar to Latched, but blocks until at least one of the producers have new data.
};

enum class ConnectPolicy
{
  Single,     ///< Only allow one ProducerPort to connect.
  Multi       ///< Allow multiple ProducerPort to connect.
};

enum class LeakPolicy
{
  Leaky,        ///< Oldest data is dropped when pushing to a full buffer
  PushBlocking  ///< Push blocks if buffer is full
};
}
