// ======================================================================
/*!
 * \brief SmartMet Admin plugin implementation
 */
// ======================================================================

#include "Plugin.h"
#include <spine/ConfigTools.h>
#include <spine/HTTP.h>
#include <spine/SmartMet.h>
#include <spine/Table.h>
#include <spine/TableFormatterFactory.h>
#include <spine/TableFormatterOptions.h>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace SmartMet
{
namespace Plugin
{
namespace Admin
{

// ----------------------------------------------------------------------
/*!
 * \brief Main content handler
 */
// ----------------------------------------------------------------------

void Plugin::requestHandler(Spine::Reactor &theReactor,
                            const Spine::HTTP::Request &theRequest,
                            Spine::HTTP::Response &theResponse)
try
{
  using namespace SmartMet::Spine;
  theReactor.executeAdminRequest(
      theRequest,
      theResponse,
      [this](const HTTP::Request& request, HTTP::Response& response) -> bool
      {
          return authenticateRequest(request, response);
      });
}
catch (...)
{
  throw Fmi::Exception::Trace(BCP, "Operation failed!");
}

// ----------------------------------------------------------------------
/*!
 * \brief Plugin constructor
 */
// ----------------------------------------------------------------------

Plugin::Plugin(Spine::Reactor *theReactor, const char *theConfig) : itsModuleName("Admin")
{
  using namespace std::placeholders;
  using namespace Spine;
  try
  {
    if (theReactor->getRequiredAPIVersion() != SMARTMET_API_VERSION)
      throw Fmi::Exception(BCP, "Admin plugin and Server API version mismatch");

    // Enable sensible relative include paths
    std::filesystem::path p = theConfig;
    p.remove_filename();
    itsConfig.setIncludeDir(p.c_str());

    itsConfig.readFile(theConfig);
    Spine::expandVariables(itsConfig);

    // Password must be specified
    if (!itsConfig.exists("password"))
      throw Fmi::Exception(BCP, "Password not specified in the config file");

    // User must be specified
    if (!itsConfig.exists("user"))
      throw Fmi::Exception(BCP, "User not specified in the config file");

    std::string truePassword;
    std::string trueUser;
    itsConfig.lookupValue("user", trueUser);          // user exists in config at this point
    itsConfig.lookupValue("password", truePassword);  // password exists in config at this point
    addUser(trueUser, truePassword);

    // Register the handler
    if (!theReactor->addPrivateContentHandler(
            this,
            "/admin",
            [this](Spine::Reactor &theReactor,
                   const Spine::HTTP::Request &theRequest,
                   Spine::HTTP::Response &theResponse)
            { callRequestHandler(theReactor, theRequest, theResponse); }))
      throw Fmi::Exception(BCP, "Failed to register admin content handler");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Initializator, in this case trivial
 */
// ----------------------------------------------------------------------
void Plugin::init() {}

// ----------------------------------------------------------------------
/*!
 * \brief Returns authentication realm name
 */
// ----------------------------------------------------------------------
std::string Plugin::getRealm() const
{
  return "SmartMet Admin";
}

// ----------------------------------------------------------------------
/*!
 * \brief Shutdown the plugin
 */
// ----------------------------------------------------------------------

void Plugin::shutdown()
{
  std::cout << "  -- Shutdown requested (admin)\n";
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the plugin name
 */
// ----------------------------------------------------------------------

const std::string &Plugin::getPluginName() const
{
  return itsModuleName;
}
// ----------------------------------------------------------------------
/*!
 * \brief Return the required version
 */
// ----------------------------------------------------------------------

int Plugin::getRequiredAPIVersion() const
{
  return SMARTMET_API_VERSION;
}

// ----------------------------------------------------------------------
/*!
 * \brief Indicate this is an admin plugin
 */
// ----------------------------------------------------------------------

bool Plugin::isAdminQuery(const Spine::HTTP::Request & /* theRequest */) const
{
  return true;
}

}  // namespace Admin
}  // namespace Plugin
}  // namespace SmartMet

/*
 * Server knows us through the 'SmartMetPlugin' virtual interface, which
 * the 'Plugin' class implements.
 */

extern "C" SmartMetPlugin *create(SmartMet::Spine::Reactor *them, const char *config)
{
  return new SmartMet::Plugin::Admin::Plugin(them, config);
}

extern "C" void destroy(SmartMetPlugin *us)
{
  // This will call 'Plugin::~Plugin()' since the destructor is virtual
  delete us;  // NOLINT(cppcoreguidelines-owning-memory)
}

// ======================================================================
