from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import load
from conan.tools.system.package_manager import Apt

required_conan_version = ">=1.60.0 <2.0 || >=2.0.5"

class superflowRecipe(ConanFile):
    name = "superflow"
    package_type = "library"

    license = "MIT"
    author = "FFI"
    homepage = "https://github.com/ffi-no/superflow"
    url = homepage
    description = "An efficient processing framework for modern C++"
    topics = ("c++")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "all":    [False, True],
        "curses": [False, True],
        "loader": [False, True],
        "yaml":   [False, True],
        "shared": [False, True],
        "tests":  [False, True],
    }
    default_options = {
        "all":    False,
        "shared": False,
        "tests":  False,
    }
    exports_sources = (
        "CMakeLists.txt",
        "LICENSE",
        "cmake/*",
        "core/*",
        "curses/*",
        "loader/*",
        "yaml/*",
        )

    def set_version(self):
        import re
        import os
        self.version = re.search(r"project\(\S+ VERSION (\d+(\.\d+){1,3})",
            load(self, os.path.join(self.recipe_folder, "CMakeLists.txt"))).group(1).strip()

    def validate(self):
        if self.settings.os == "Windows" and self.options.curses:
            raise ConanInvalidConfiguration("Windows not supported for module 'curses'")

    def config_options(self):
        pass

    def configure(self):
        if self.options.all:
            self.options.curses = True
            self.options.loader = True
            self.options.yaml = True

        if self.options.loader:
            self.options.yaml = True

        for lib_name in ['curses', 'loader', 'yaml']:
            if not getattr(self.options, lib_name):
                setattr(self.options, lib_name, False)

        opts = sorted(self.options._package_options._data)
        width = len(max(opts, key=len)) + 1
        self.output.info("superflow options configured:\n" + "\n".join([f"  {o: <{width}}: {str(getattr(self.options, o))}" for o in opts]))

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["BUILD_SHARED_LIBS"] = self.options.shared
        tc.variables["BUILD_TESTS"] = self.options.tests
        tc.variables["BUILD_all"] = self.options.all
        tc.variables["BUILD_curses"] = self.options.curses
        tc.variables["BUILD_loader"] = self.options.loader
        tc.variables["BUILD_yaml"] = self.options.yaml
        tc.generate()

    def requirements(self):
        if self.options.curses:
            self.requires("ncursescpp/1.0.0")

        if self.options.loader:
            self.requires("boost/1.76.0", transitive_headers=True)

        if self.options.yaml:
            self.requires("yaml-cpp/0.8.0", transitive_headers=True)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.tests:
            self.run(f"ctest --output-on-failure --output-junit report.xml")
    
    def build_requirements(self):
        self.tool_requires("cmake/3.27.4")

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def layout(self):
        cmake_layout(self)

    def package_info(self):
        for lib_name in ['core', 'curses', 'loader', 'yaml']:
            if lib_name == 'core' or getattr(self.options, lib_name):
                self.output.info("adding component '{}'".format(lib_name))
                self.cpp_info.components[lib_name].set_property("cmake_target_name", f"{self.name}::{lib_name}")
                self.cpp_info.components[lib_name].set_property("cmake_file_name", f"{self.name}-{lib_name}")
                self.cpp_info.components[lib_name].libs = ['superflow-' + lib_name]
                if lib_name != 'core':
                    self.cpp_info.components[lib_name].requires.append('core')

        if self.settings.os == "Linux":
            self.cpp_info.components['core'].system_libs = ["pthread"]

        if self.options.curses:
            self.cpp_info.components['curses'].requires.append('ncursescpp::ncursescpp')

        if self.options.loader:
            self.cpp_info.components['loader'].requires.append('boost::boost')
            if self.options.yaml:
                self.cpp_info.components['loader'].requires.append('yaml')
            if self.settings.os == "Linux":
                self.cpp_info.components['loader'].system_libs.append('dl')

        if self.options.yaml:
            self.cpp_info.components['yaml'].requires.append('yaml-cpp::yaml-cpp')
            self.cpp_info.components['yaml'].defines = [
                'LOADER_ADAPTER_HEADER=\"superflow/yaml/yaml_property_list.h\"',
                'LOADER_ADAPTER_NAME=YAML',
                'LOADER_ADAPTER_TYPE=flow::yaml::YAMLPropertyList',
            ]

        if self.settings.build_type == "Debug":
            for component in self.cpp_info.components.values():
                for i, lib in enumerate(component.libs):
                    component.libs[i] = lib + 'd'

        for k, v in self.cpp_info.components.items():
            self.output.info("{:<11} -> {}".format("{} libs".format(k), v.libs))
            self.output.info("{:<11} -> {}".format("{} requires".format(k), v.requires))
