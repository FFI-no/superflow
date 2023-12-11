#include "superflow/loader/load_factories.h"
#include "superflow/yaml/yaml_property_list.h"

int main()
{
  try
  {
    const std::vector<flow::load::ProxelLibrary> libraries{{"path to library"}};

    const auto factory_map = flow::load::loadFactories<flow::yaml::YAMLPropertyList>(libraries);
  } catch (...)
  {}
}
