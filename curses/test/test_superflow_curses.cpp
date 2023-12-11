// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/curses/graph_gui.h"
#include "superflow/graph.h"

#include "gtest/gtest.h"

using namespace flow;
using namespace flow::curses;

TEST(SuperFlowCurses, compiles)
{
  Graph graph(std::map<std::string, Proxel::Ptr>{});
  GraphGUI gui;
}
