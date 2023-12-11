#include "superflow/loader/proxel_library.h"
#include "boost/dll/library_info.hpp"

namespace flow::load
{

ProxelLibrary::ProxelLibrary(const boost::dll::fs::path& path_to_directory, const std::string& library_name)
  : ProxelLibrary{
  path_to_directory / library_name,
  boost::dll::load_mode::append_decorations | boost::dll::load_mode::rtld_now}
{}

ProxelLibrary::ProxelLibrary(const boost::dll::fs::path& full_path)
  : ProxelLibrary{
  full_path,
  boost::dll::load_mode::rtld_now}
{}

ProxelLibrary::ProxelLibrary(const boost::dll::fs::path& path, boost::dll::load_mode::type load_mode)
try
  : library_{
  path,
  load_mode}
{}
catch (boost::system::system_error& e)
{
  const auto error_code = e.code();
  const auto error_code_value = std::to_string(error_code.value());
  const std::string error_code_category_name{error_code.category().name()};
  const auto error_code_message = error_code.message();
  const std::string what =
    "Exception occurred in constructor 'ProxelLibrary(\"" + path.string() + "\")':\n  "
    + "error_code " + error_code_value + " (" + error_code_category_name + ") message: " + error_code_message + ". What:\n  "
    + e.what();

  throw std::invalid_argument(what);
}
catch (std::ios_base::failure& e)
{
  throw std::invalid_argument(
    "Exception occurred in constructor 'ProxelLibrary(\"" + path.string() + "\")', probably due to non-existing path.\n  " +
    e.what());
}

std::vector<std::string> ProxelLibrary::getSectionSymbols(const std::string& adapter_name) const
{
  boost::dll::library_info info(library_.location());
  std::vector<std::string> symbols = info.symbols(adapter_name);
  if (symbols.empty())
  {
    throw std::invalid_argument(
      "no section '" + adapter_name + "' found in library, or no proxel factories in in it."
    );
  }
  return symbols;
}

std::string ProxelLibrary::extractFactoryName(const std::string& symbol, const std::string& adapter_name)
{
  return symbol.substr(adapter_name.size() + 1);
}
}