// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/yaml/yaml_property_list.h"

#include "superflow/factory_map.h"
#include "superflow/proxel_config.h"

namespace flow::yaml
{
using Factory = flow::Factory<YAMLPropertyList>;
using FactoryMap = flow::FactoryMap<YAMLPropertyList>;
using ProxelConfig = flow::ProxelConfig<YAMLPropertyList>;
}
