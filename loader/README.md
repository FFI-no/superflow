
### loader
The `loader` module enables dynamic loading of proxel libraries (shared libraries) like plugins.
A `loader`-compatible library has embedded a list of <i>Factory</i>s for the proxels it contains,
which assists in the creation of a corresponding _FactoryMap_.
The advantage of this is that the user doesn't have to include any specific header files from the
library in their consuming application, and with the addition of the `yaml` module,
no hard coded proxel names are required at all in the compiled client code.

There are three important aspects when dealing with proxel libraries:

1. When compiling the proxel library, you must have already decided on a concrete `PropertyList` (or "value adapter").   
   At the time of writing, the `flow::yaml::YAMLPropertyList` is the only PropertyList we have implemented.
2. For each proxel, the factory must be "registered" using the macro `REGISTER_PROXEL_FACTORY`
   from the header file `"superflow/loader/register_factory.h"`. 
3. When loading the library into the consuming application, create a `flow::load::ProxelLibrary` for each shared library,   
   and keep them in scope as long as their proxels are in use. 

#### 1. PropertyList
The interface of a `PropertyList` is defined through the templated function
`flow::value<T>` from `"superflow/value.h"`.
It is required that a valid `PropertyList` defines the following methods:

- `bool hasKey(const std::string& key)`, which tells if the given key exists or not.
- `T convertValue(const std::string& key)`, which retrieves a value from the list.

In addition, `flow::load::loadFactories<PropertyList>` demands the existence of the static string `PropertyList::adapter_name`.   
It must contain the value of the macro `LOADER_ADAPTER_NAME` (see the next section).

#### 2. REGISTER_PROXEL_FACTORY
This macro will export a _named symbol_ to a _named section_ in the shared library,
which can later be retrieved by those names. See the `Boost::DLL` documentation for more details.

For the user, we want this to be as pleasant an experience as possible:

```cpp
#include "my/proxel.h"
#include "superflow/loader/register_factory.h"

template<typename PropertyList>
flow::Proxel::Ptr createMyProxel(const PropertyList& adapter)
{
  return std::make_shared<MyProxel>(
    flow::value<int>(adapter, "key")
  );
}

REGISTER_PROXEL_FACTORY(MyProxel, createMyProxel)
```

In order for this to work, we demand that the author of the PropertyList defines the following macros:

- `LOADER_ADAPTER_HEADER`, path to the concrete property list header file
- `LOADER_ADAPTER_NAME`, a short, unique identifier for the property list
- `LOADER_ADAPTER_TYPE`, the concrete type of property list

For the `flow::yaml` module, these macros are defined through the target's "interface compile definitions".
That means that when you link your proxel library to `flow::yaml`, they will be automatically defined.

#### 3. ProxelLibrary

A proxel library and its _FactoryMap_ are accessed through the use of a `flow::load::ProxelLibrary` object.
You construct it using the path to the shared library file, and fetch the _FactoryMap_ using the method `loadFactories<PropertyList>`.
An object of this class must not go out of scope as long as its proxels are in use.
That will cause the shared library to be unloaded, and your application to crash!

```cpp
const flow::load::ProxelLibrary library{"path/to/library"};
const auto factories = library.loadFactories<flow::yaml::YAMLPropertyList>();
```

You can collect factories from multiple libraries using the free function `loadFactories`.
Remember that the libraries mustn't go out of scope, hence we keep the vector.

```cpp
const std::vector<flow::load::ProxelLibrary> library_paths{
   {"path/to/library1"},
   {"path/to/library2"}
  };

const auto factories = flow::load::loadFactories<flow::yaml::YAMLPropertyList>(
  library_paths
);
```

##### Pro tip
If you separate the name of the library and the path to the directory in which the library resides,
Boost can automatically determine the prefix and postfix of the shared library file.

```cpp
const std::string library_directory{"/directory"};
const std::string library_name{"myproxels"};
const flow::load::ProxelLibrary library{library_directory, library_name};

/// Linux result: /directory/libmyproxels.so
/// Windows result: /directory/myproxels.dll
```

#### Further reading

See the tests in the `loader` module for more examples, or read the report for more details.

---
