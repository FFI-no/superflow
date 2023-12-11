// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/requester_port.h"

#include "gtest/gtest.h"

using namespace flow;

TEST(RequesterResponderPort, matchingPortsConnects)
{
  auto out = std::make_shared<RequesterPort<void(int)>>();
  auto in1 = std::make_shared<ResponderPort<void(int)>>([](int){});
  auto in2 = std::make_shared<ResponderPort<void(int)>>([](int){});

  ASSERT_NO_FATAL_FAILURE(out->connect(in1));
}

TEST(RequesterPort, disconnectNoThrow)
{
  auto out = std::make_shared<RequesterPort<void(int)>>();

  ASSERT_NO_FATAL_FAILURE(out->disconnect());
}

TEST(ResponderPort, disconnectNoThrow)
{
  auto in = std::make_shared<ResponderPort<void(int)>>([](int)
                                                   {});

  ASSERT_NO_FATAL_FAILURE(in->disconnect());
}

TEST(RequesterResponderPort, connectAfterDisconnectDoesNotThrow)
{
  auto out = std::make_shared<RequesterPort<void(int)>>();
  auto in1 = std::make_shared<ResponderPort<void(int)>>([](int){});

  ASSERT_NO_FATAL_FAILURE(out->connect(in1));
  ASSERT_NO_FATAL_FAILURE(out->disconnect());

  auto in2 = std::make_shared<ResponderPort<void(int)>>([](int){});
  ASSERT_NO_FATAL_FAILURE(in2->connect(out));
}

TEST(RequesterResponderPort, mismatchingPortsThrows)
{
  auto master = std::make_shared<RequesterPort<void(int)>>();
  auto slave1 = std::make_shared<ResponderPort<void(float)>>([](float)
                                                         {});
  auto slave2 = std::make_shared<ResponderPort<bool(int)>>([](int) -> bool
                                                       { return false; });

  ASSERT_THROW(master->connect(slave1), std::invalid_argument);
  ASSERT_THROW(slave1->connect(master), std::invalid_argument);
  ASSERT_THROW(slave2->connect(master), std::invalid_argument);
}

TEST(RequesterResponderPort, connectWorksBothWays)
{
  {
    auto out = std::make_shared<RequesterPort<void(int)>>();
    auto in = std::make_shared<ResponderPort<void(int)>>([](int)
                                                     {});

    ASSERT_FALSE(out->isConnected());
    ASSERT_FALSE(in->isConnected());

    out->connect(in);

    ASSERT_TRUE(out->isConnected());
    ASSERT_TRUE(in->isConnected());
  }

  {
    auto out = std::make_shared<RequesterPort<void(int)>>();
    auto in = std::make_shared<ResponderPort<void(int)>>([](int)
                                                     {});

    ASSERT_FALSE(out->isConnected());
    ASSERT_FALSE(in->isConnected());

    in->connect(out);

    ASSERT_TRUE(out->isConnected());
    ASSERT_TRUE(in->isConnected());
  }
}

TEST(RequesterResponderPort, newConnectToMasterThrows)
{
  auto requester = std::make_shared<RequesterPort<void(int)>>();

  auto responder_1 = std::make_shared<ResponderPort<void(int)>>([](int){});
  auto responder_2 = std::make_shared<ResponderPort<void(int)>>([](int){});

  requester->connect(responder_1);

  ASSERT_TRUE(requester->isConnected());
  ASSERT_TRUE(responder_1->isConnected());

  ASSERT_THROW(requester->connect(responder_2), std::runtime_error);

  ASSERT_TRUE(requester->isConnected());
  ASSERT_TRUE(responder_1->isConnected());
  ASSERT_FALSE(responder_2->isConnected());
}

TEST(RequesterResponderPort, newConnectToSlaveDoesNotDisconnectOldMaster)
{
  auto out1 = std::make_shared<RequesterPort<void(int)>>();
  auto out2 = std::make_shared<RequesterPort<void(int)>>();
  auto in = std::make_shared<ResponderPort<void(int)>>([](int){});

  in->connect(out1);

  ASSERT_TRUE(in->isConnected());
  ASSERT_EQ(in->getStatus().num_connections, 1);
  ASSERT_TRUE(out1->isConnected());

  in->connect(out2);

  ASSERT_TRUE(in->isConnected());
  ASSERT_EQ(in->getStatus().num_connections, 2);
  ASSERT_TRUE(out1->isConnected());
  ASSERT_TRUE(out2->isConnected());
}

TEST(RequesterResponderPort, doubleConnectOfSamePortsNoFail)
{
  auto requester = std::make_shared<RequesterPort<void(int)>>();
  auto responder_1 = std::make_shared<ResponderPort<void(int)>>([](int){});
  auto responder_2 = std::make_shared<ResponderPort<void(int)>>([](int){});

  ASSERT_NO_FATAL_FAILURE(requester->connect(responder_1));
  ASSERT_NO_FATAL_FAILURE(requester->connect(responder_1));
  ASSERT_NO_FATAL_FAILURE(responder_1->connect(requester));
}

TEST(RequesterResponderPort, disconnectWorksBothWays)
{
  {
    auto out = std::make_shared<RequesterPort<void(int)>>();
    auto in = std::make_shared<ResponderPort<void(int)>>([](int){});

    out->connect(in);

    ASSERT_TRUE(out->isConnected());
    ASSERT_TRUE(in->isConnected());

    out->disconnect();

    ASSERT_FALSE(out->isConnected());
    ASSERT_FALSE(in->isConnected());
  }

  {
    auto out = std::make_shared<RequesterPort<void(int)>>();
    auto in = std::make_shared<ResponderPort<void(int)>>([](int)
                                                     {});

    out->connect(in);

    ASSERT_TRUE(out->isConnected());
    ASSERT_TRUE(in->isConnected());

    in->disconnect();

    ASSERT_FALSE(out->isConnected());
    ASSERT_FALSE(in->isConnected());
  }
}

TEST(RequesterResponderPort, slaveRespondsToMaster)
{
  auto master = std::make_shared<RequesterPort<int(int)>>();
  auto slave = std::make_shared<ResponderPort<int(int)>>([](int a)
                                                     { return a; });

  ASSERT_NO_FATAL_FAILURE(master->connect(slave));
  auto value = master->request(42);
  ASSERT_EQ(value, 42);
}

TEST(RequesterResponderPort, requestAfterDisconnectThrows)
{
  auto master = std::make_shared<RequesterPort<int(int)>>();
  auto slave = std::make_shared<ResponderPort<int(int)>>([](int a)
                                                     { return a; });

  ASSERT_THROW(master->request(42), std::runtime_error);
}

TEST(RequesterResponderPort, SlaveCanModifyValuePassedByReference)
{
  auto master = std::make_shared<RequesterPort<int(int&, int)>>();
  auto slave = std::make_shared<ResponderPort<int(int&, int)>>([](int& a, int b)
                                                           {
                                                             a += 2;
                                                             return a + b;
                                                           });

  ASSERT_NO_FATAL_FAILURE(master->connect(slave));
  int a = 20;
  auto value = master->request(a, 20);
  ASSERT_EQ(value, 42);
  ASSERT_EQ(a, 22);
}

TEST(RequesterResponderPort, PassingSharedPointers)
{
  class A
  {
  public:
    int value{0};
  };

  ResponderPort<void(std::shared_ptr<A>)> slave([](std::shared_ptr<A> ptr)
                                            { ptr->value = 42; });

  auto a = std::make_shared<A>();
  ASSERT_EQ(a->value, 0);
  slave.respond(a);
  ASSERT_EQ(a->value, 42);
}

TEST(RequesterResponderPort, BindMemberFunction)
{
  class A
  {
    int func()
    { return value; }

  public:
    int value{42};

    using PortType = ResponderPort<int(void)>;
    std::shared_ptr<PortType> port = std::make_shared<PortType>([this]()
                                                                { return func(); });
  };

  A a;
  a.value = 40;
  auto value = a.port->respond();
  ASSERT_EQ(value, 40);
}

TEST(RequesterResponderPort, pointersAreFreedOnDisconnect)
{
  {
    auto out = std::make_shared<RequesterPort<void(int)>>();
    auto in = std::make_shared<ResponderPort<void(int)>>([](int)
                                                     {});

    ASSERT_EQ(out.use_count(), 1);
    ASSERT_EQ(in.use_count(), 1);

    out->connect(in);

    ASSERT_EQ(out.use_count(), 2);
    ASSERT_EQ(in.use_count(), 3);

    out->disconnect();

    ASSERT_EQ(out.use_count(), 1);
    ASSERT_EQ(in.use_count(), 1);
  }

  {
    auto out = std::make_shared<RequesterPort<void(int)>>();
    auto in = std::make_shared<ResponderPort<void(int)>>([](int)
                                                     {});

    ASSERT_EQ(out.use_count(), 1);
    ASSERT_EQ(in.use_count(), 1);

    in->connect(out);

    ASSERT_EQ(out.use_count(), 2);
    ASSERT_EQ(in.use_count(), 3);

    in->disconnect();

    ASSERT_EQ(out.use_count(), 1);
    ASSERT_EQ(in.use_count(), 1);
  }
}

TEST(RequesterResponderPort, conversion)
{
  {
    const auto responder = std::make_shared<ResponderPort<int(), bool>>(
      [](){ return 10; }
    );
    const auto int_requester = std::make_shared<RequesterPort<int()>>();

    ASSERT_NO_THROW(int_requester->connect(responder));
    ASSERT_EQ(int_requester->request(), 10);
  }

  {
    const auto responder = std::make_shared<ResponderPort<int(), bool>>(
      [](){ return 10; }
    );
    const auto bool_requester = std::make_shared<RequesterPort<bool()>>();

    ASSERT_NO_THROW(bool_requester->connect(responder));
    ASSERT_EQ(bool_requester->request(), true);
  }
}

namespace
{
struct IntClass
{
  std::string name;
  int val;

  operator int() const  // NOLINT intentionally implicit
  {
    return val;
  }

  explicit operator bool() const
  {
    return val;
  }
};
}

TEST(RequesterResponderPort, implicit_conversion)
{
  {
    const auto responder = std::make_shared<ResponderPort<IntClass(), int>>(
      [](){ return IntClass{"name", 10}; }
    );
    const auto int_class_requester = std::make_shared<RequesterPort<IntClass()>>();

    ASSERT_NO_THROW(int_class_requester->connect(responder));
    const IntClass int_class = int_class_requester->request();
    ASSERT_EQ(int_class.name, "name");
    ASSERT_EQ(int_class.val, 10);
  }

  {
    const auto responder = std::make_shared<ResponderPort<IntClass(), int>>(
      [](){ return IntClass{"name", 10}; }
    );
    const auto int_requester = std::make_shared<RequesterPort<int()>>();

    ASSERT_NO_THROW(int_requester->connect(responder));
    ASSERT_EQ(int_requester->request(), 10);
  }
}

TEST(RequesterResponderPort, explicit_conversion)
{
  {
    const auto responder = std::make_shared<ResponderPort<IntClass(), bool>>(
      [](){ return IntClass{"name", 10}; }
    );
    const auto requester = std::make_shared<RequesterPort<IntClass()>>();

    ASSERT_NO_THROW(requester->connect(responder));
    const IntClass int_class = requester->request();
    ASSERT_EQ(int_class.name, "name");
    ASSERT_EQ(int_class.val, 10);
  }

  {
    const auto responder = std::make_shared<ResponderPort<IntClass(), bool>>(
      [](){ return IntClass{"name", 10}; }
    );
    const auto requester = std::make_shared<RequesterPort<bool()>>();

    ASSERT_NO_THROW(requester->connect(responder));
    ASSERT_EQ(requester->request(), true);
  }
}

namespace
{
struct CIntClass
{
  CIntClass(
    const std::string& name_in,
    const int val_in
  )
    : name{name_in}
    , val{val_in}
  {}

  CIntClass(
    const std::string& name_in
  )
    : name{name_in}
    , val{0}
  {}

  explicit CIntClass(
    const IntClass& int_class
  )
    : name{int_class.name}
    , val{int_class.val}
  {}

  std::string name;
  int val;
};
}

TEST(RequesterResponderPort, implicit_ctor_conversion)
{
  {
    const auto responder = std::make_shared<ResponderPort<CIntClass()>>(
      [](){ return CIntClass{"namea", 10}; }
    );
    const auto requester = std::make_shared<RequesterPort<CIntClass(), std::string>>();

    ASSERT_NO_THROW(requester->connect(responder));

    const auto res = requester->request();
    ASSERT_EQ(res.name, "namea");
    ASSERT_EQ(res.val, 10);
  }

  {
    const auto responder = std::make_shared<ResponderPort<std::string()>>(
      [](){ return "name"; }
    );
    const auto requester = std::make_shared<RequesterPort<CIntClass(), std::string>>();

    ASSERT_NO_THROW(requester->connect(responder));

    const auto res = requester->request();
    ASSERT_EQ(res.name, "name");
    ASSERT_EQ(res.val, 0);
  }
}

TEST(RequesterResponderPort, explicit_ctor_conversion)
{
  {
    const auto responder = std::make_shared<ResponderPort<CIntClass()>>(
      [](){ return CIntClass{"namea", 10}; }
    );
    const auto requester = std::make_shared<RequesterPort<CIntClass(), IntClass>>();

    ASSERT_NO_THROW(requester->connect(responder));

    const CIntClass res = requester->request();
    ASSERT_EQ(res.name, "namea");
    ASSERT_EQ(res.val, 10);
  }

  {
    const auto responder = std::make_shared<ResponderPort<IntClass()>>(
      [](){ return IntClass{"name", 15}; }
    );
    const auto requester = std::make_shared<RequesterPort<CIntClass(), IntClass>>();

    ASSERT_NO_THROW(requester->connect(responder));

    const CIntClass res = requester->request();
    ASSERT_EQ(res.name, "name");
    ASSERT_EQ(res.val, 15);
  }
}

namespace
{
class Base
{
public:
  virtual ~Base() = default;
};

class Derived : public Base
{};
}

TEST(RequesterResponderPort, up_cast)
{
  {
    const auto responder = std::make_shared<ResponderPort<Derived(), Base>>(
      [](){ return Derived{}; }
    );
    const auto requester = std::make_shared<RequesterPort<Derived()>>();

    ASSERT_NO_THROW(responder->connect(requester));

    const Derived res = requester->request();
  }

  {
    const auto responder = std::make_shared<ResponderPort<Derived(), Base>>(
      [](){ return Derived{}; }
    );
    const auto requester = std::make_shared<RequesterPort<Base()>>();

    ASSERT_NO_THROW(responder->connect(requester));

    const Base res = requester->request();
  }

  {
    const auto responder = std::make_shared<ResponderPort<Derived()>>(
      [](){ return Derived{}; }
    );
    const auto requester = std::make_shared<RequesterPort<Derived(), Derived>>();

    ASSERT_NO_THROW(responder->connect(requester));

    const Derived res = requester->request();
  }

  {
    const auto responder = std::make_shared<ResponderPort<Derived()>>(
      [](){ return Derived{}; }
    );
    const auto requester = std::make_shared<RequesterPort<Base(), Derived>>();

    ASSERT_NO_THROW(responder->connect(requester));

    const Base res = requester->request();
  }
}

TEST(RequesterResponderPort, std_variant_conversions)
{
  {
    const auto responder = std::make_shared<ResponderPort<int()>>(
      [](){ return 10; }
    );
    const auto requester = std::make_shared<RequesterPort<std::variant<int, bool>()>>();

    ASSERT_NO_THROW(responder->connect(requester));
    ASSERT_EQ(std::get<int>(requester->request()), 10);
  }

  {
    const auto responder = std::make_shared<ResponderPort<bool()>>(
      [](){ return false; }
    );
    const auto requester = std::make_shared<RequesterPort<std::variant<int, bool>()>>();

    ASSERT_NO_THROW(responder->connect(requester));
    ASSERT_EQ(std::get<bool>(requester->request()), false);
  }
}
