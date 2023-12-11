#include "superflow/graph.h"
#include "superflow/yaml/yaml.h"

int main()
{
  try
  {
    const auto graph = flow::yaml::createGraph("", flow::yaml::FactoryMap{});
  } catch (...)
  {}
}
