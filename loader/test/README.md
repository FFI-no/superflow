How the dummy so-files for testing rtld_now were created: (main is not required)

```bash
g++ -c -Wall -fpic missing_dependency.cpp
g++ -shared -o libmissing_dependency.so missing_dependency.o

g++ -c -Wall -fpic dependent_library.cpp
g++ -L. -shared -o libdependent_library.so dependent_library.o -lmissing_dependency

LD_LIBRARY_PATH=${PWD} g++ -L. -Wall -o main main.cpp -ldependent_library
LD_LIBRARY_PATH=${PWD} ./main

cp libmissing_dependency.so superflow/loader/test/libs/
cp libdependent_library.so superflow/loader/test/libs/
```

**missing_dependency.hpp**
```cpp
#pragma once

void function_in_missing_dependency();
```

**missing_dependency.cpp**
```cpp
#include "missing_dependency.hpp"
#include <iostream>

void function_in_missing_dependency()
{ std::cout << "Hello, I am a function in a missing shared library" << std::endl; }
```

**dependent_library.hpp**
```cpp
#pragma once

void dependent_function();
```

**dependent_library.cpp**
```cpp
#include "dependent_library.hpp"
#include "missing_dependency.hpp"

void dependent_function()
{ function_in_missing_dependency(); }
```

**main.cpp**
```cpp
#include "dependent_library.hpp"

int main()
{ dependent_function(); }
```
