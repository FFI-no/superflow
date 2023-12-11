// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "gtest/gtest.h"
#include "templated_testproxel.h"
#include "superflow/graph.h"

using namespace flow;

class TestProxel : public Proxel
{
public:
  TestProxel()
      : thread_id_{}
      , stop_was_called_{false}
  {}

  using Ptr = std::shared_ptr<TestProxel>;

  void start() override
  {
    if (not what_str_.empty())
    { throw std::runtime_error(what_str_); }
    run();
  }

  void stop() noexcept override
  { stop_was_called_ = true; }

  std::thread::id getThreadId() const
  { return thread_id_; }

  bool stopWasCalled() const
  { return stop_was_called_; };

  void setException(const std::string& what)
  { what_str_ = what; }

private:
  void run()
  { thread_id_ = std::this_thread::get_id(); }

  std::thread::id thread_id_;
  bool stop_was_called_;
  std::string what_str_;
};

struct HijackCerr
{
  HijackCerr()
    : cerr_(std::cerr.rdbuf(buffer_.rdbuf()))
  {}

  ~HijackCerr()
  { std::cerr.rdbuf(cerr_); }

  std::string getString()
  { return buffer_.str(); }

private:
  std::stringstream buffer_;
  std::streambuf* cerr_;
};

TEST(Graph, constructsWithEmptySet)
{ EXPECT_NO_FATAL_FAILURE(Graph{}); }

TEST(Graph, constructsWithOneProxel)
{ EXPECT_NO_FATAL_FAILURE(Graph({{"id", std::make_shared<TestProxel>()}})); }

TEST(Graph, constructsWithSeveralProxels)
{
  EXPECT_NO_FATAL_FAILURE(
      Graph(
      {
        { "id1", std::make_shared<TestProxel>() },
        { "id2", std::make_shared<TestProxel>() },
        { "id3", std::make_shared<TestProxel>() }
      }
  ));
}

TEST(Graph, constructsWithMap)
{
  std::map<std::string, Proxel::Ptr> procs;
  procs.emplace("id1", std::make_shared<TestProxel>());
  procs.emplace("id2", std::make_shared<TestProxel>());

  EXPECT_NO_FATAL_FAILURE(Graph(std::move(procs)));
}

TEST(Graph, proxelsAreStartedInSeperateThreads)
{
  TestProxel::Ptr proxel_A{std::make_shared<TestProxel>()};
  TestProxel::Ptr proxel_B{std::make_shared<TestProxel>()};

  Graph flow{
      {
          {"id_a", proxel_A},
          {"id_b", proxel_B}
      }
  };
  flow.start();
  flow.stop();

  EXPECT_NE(std::thread::id{}, proxel_A->getThreadId());
  EXPECT_NE(std::thread::id{}, proxel_B->getThreadId());
  EXPECT_NE(std::this_thread::get_id(), proxel_A->getThreadId());
  EXPECT_NE(std::this_thread::get_id(), proxel_B->getThreadId());
  EXPECT_NE(proxel_A->getThreadId(), proxel_B->getThreadId());
}

TEST(Graph, proxelsAreStopped)
{
  TestProxel::Ptr proxel_A{std::make_shared<TestProxel>()};
  TestProxel::Ptr proxel_B{std::make_shared<TestProxel>()};

  Graph flow{{{"a", proxel_A}, {"b", proxel_B}}};
  flow.start();
  flow.stop();

  EXPECT_TRUE(proxel_A->stopWasCalled());
  EXPECT_TRUE(proxel_B->stopWasCalled());
}

TEST(Graph, destructorCallsStop)
{
  TestProxel::Ptr proxel_A{std::make_shared<TestProxel>()};
  TestProxel::Ptr proxel_B{std::make_shared<TestProxel>()};

  {
    Graph flow{{{"a", proxel_A}, {"b", proxel_B}}};
    flow.start();
  }

  EXPECT_TRUE(proxel_A->stopWasCalled());
  EXPECT_TRUE(proxel_B->stopWasCalled());
}

TEST(Graph, doublestartThrowsException)
{
  TestProxel::Ptr proxel_A{std::make_shared<TestProxel>()};
  TestProxel::Ptr proxel_B{std::make_shared<TestProxel>()};

  Graph flow{{{"a", proxel_A}, {"b", proxel_B}}};
  flow.start();
  EXPECT_THROW(flow.start(), std::runtime_error);
  flow.stop();
  EXPECT_NO_THROW(flow.start());
  flow.stop();
}

TEST(Graph, addingSameIdTwiceThrows)
{
  Proxel::Ptr proxel_A{std::make_shared<TestProxel>()};
  Proxel::Ptr proxel_B{std::make_shared<TestProxel>()};

  Graph flow1;
  EXPECT_NO_THROW(flow1.add("id1", std::move(proxel_A)));
  EXPECT_THROW(flow1.add("id1", std::move(proxel_B)), std::invalid_argument);
}

TEST(Graph, connectCompatiblePorts)
{
  Graph flow(
      {
          {"out", std::make_shared<TemplatedProxel<int>>(42)},
          {"in",  std::make_shared<TemplatedProxel<int>>(0)}
      }
  );

  ASSERT_NO_THROW(flow.connect("out", "outport", "in", "inport"));
}

TEST(Graph, connectIncompatiblePortsThrows)
{
  Graph flow(
      {
          {"out", std::make_shared<TemplatedProxel<int>>(42)},
          {"in",  std::make_shared<TemplatedProxel<double>>(0.0)}
      }
  );

  ASSERT_THROW(flow.connect("out", "outport", "in", "inport"), std::invalid_argument);
}

TEST(Graph, invalidConnectThrowsErrorWhichContainsProxelAndPortNames)
{
  const std::string proxel1_name = "proxel1";
  const std::string proxel2_name = "proxel2";
  const std::string proxel1_port_name = "outport";
  const std::string proxel2_port_name = "inport";

  Graph flow(
      {
          {proxel1_name, std::make_shared<TemplatedProxel<int>>(42)},
          {proxel2_name,  std::make_shared<TemplatedProxel<double>>(0.0)}
      }
  );

  std::string what;

  try
  {
    flow.connect(
        proxel1_name,
        proxel1_port_name,
        proxel2_name,
        proxel2_port_name
    );
  }
  catch (const std::invalid_argument& e)
  {
    what = e.what();
  }

  ASSERT_NE(what.find(proxel1_name), std::string::npos);
  ASSERT_NE(what.find(proxel1_port_name), std::string::npos);
  ASSERT_NE(what.find(proxel2_name), std::string::npos);
  ASSERT_NE(what.find(proxel2_port_name), std::string::npos);
}

TEST(Graph, connectNonExistingPortsThrows)
{
  Graph flow(
      {
          {"out", std::make_shared<TemplatedProxel<int>>(42)},
          {"in",  std::make_shared<TemplatedProxel<int>>(0)}
      }
  );

  ASSERT_THROW(flow.connect("out", "nonexisting_port", "in", "inport"), std::invalid_argument);
}

TEST(Graph, valuePropagates)
{
  constexpr int value{42};
  Proxel::Ptr proc_out{std::make_shared<TemplatedProxel<int>>(value)};
  auto proc_in = std::make_shared<TemplatedProxel<int>>(0);

  Graph flow(
      {
          {"out", proc_out},
          {"in",  proc_in}
      }
  );

  flow.connect("out", "outport", "in", "inport");

  flow.start();
  flow.stop();

  ASSERT_EQ(value, proc_in->getValue());
}

TEST(Graph, connectToSelfThrows)
{
  Graph flow(
      {{"proxel", std::make_shared<TemplatedProxel<int>>(42)}}
  );

  ASSERT_THROW(flow.connect("proxel", "outport", "proxel", "inport"), std::invalid_argument);
}

TEST(Graph, handleExceptionWithDefaultLogger)
{
  TestProxel::Ptr proxel_A{std::make_shared<TestProxel>()};
  TestProxel::Ptr proxel_B{std::make_shared<TestProxel>()};

  const std::string msg{"mayday"};
  proxel_A->setException(msg);

  Graph flow{{{"a", proxel_A}, {"b", proxel_B}}};
  constexpr bool handle_exceptions = true;

  HijackCerr db_cooper;

  flow.start(handle_exceptions);
  flow.stop();

  {
    std::ostringstream os;
    os << "Proxel 'a' crashed with exception:\n  \"mayday\"\n";
    ASSERT_EQ(os.str(), db_cooper.getString());
  }
}

TEST(Graph, handleExceptionWithQuietLogger)
{
  TestProxel::Ptr proxel_A{std::make_shared<TestProxel>()};
  TestProxel::Ptr proxel_B{std::make_shared<TestProxel>()};

  const std::string msg{"mayday"};
  proxel_A->setException(msg);

  Graph flow{{{"a", proxel_A}, {"b", proxel_B}}};
  constexpr bool handle_exceptions = true;

  HijackCerr db_cooper;

  flow.start(handle_exceptions, flow::Graph::quietCrashLogger);
  flow.stop();

  ASSERT_EQ(std::string{}, db_cooper.getString());
}

TEST(Graph, handleExceptionWithCustomLogger)
{
  TestProxel::Ptr proxel_A{std::make_shared<TestProxel>()};
  TestProxel::Ptr proxel_B{std::make_shared<TestProxel>()};

  const std::string msg{"mayday"};
  proxel_A->setException(msg);

  std::string what_proxel;
  std::string what_msg;
  flow::Graph::CrashLogger custom_sink = [&what_proxel, &what_msg](const auto& proxel_name , const auto& what)
  {
    what_proxel = proxel_name;
    what_msg = what;
  };

  Graph flow{{{"a", proxel_A}, {"b", proxel_B}}};
  constexpr bool handle_exceptions = true;

  flow.start(handle_exceptions, custom_sink);
  flow.stop();

  EXPECT_EQ(what_proxel, "a");
  EXPECT_EQ(what_msg, msg);

  EXPECT_TRUE(proxel_A->stopWasCalled());
  EXPECT_TRUE(proxel_B->stopWasCalled());
}

TEST(Graph, handleExceptionWithExceptionPtr)
{
  TestProxel::Ptr proxel_A{std::make_shared<TestProxel>()};
  TestProxel::Ptr proxel_B{std::make_shared<TestProxel>()};

  const std::string msg{"mayday"};
  proxel_A->setException(msg);

  std::map<std::string, std::exception_ptr> epq;
  flow::Graph::CrashLogger custom_sink = [&epq](const auto& proxel_name , const auto&)
  {
    epq.emplace(proxel_name, std::current_exception());
  };

  Graph flow{{{"a", proxel_A}, {"b", proxel_B}}};
  constexpr bool handle_exceptions = true;

  flow.start(handle_exceptions, custom_sink);
  flow.stop();

  ASSERT_FALSE(epq.empty());
  ASSERT_TRUE(epq.count("a") > 0);
  try
  {
    std::rethrow_exception(epq.at("a"));
  }
  catch (const std::exception& e)
  {
    EXPECT_EQ(msg, e.what());
  }
}