// ======================================================================
/*!
 * \brief SmartMet Admin plugin implementation
 */
// ======================================================================

#include "Plugin.h"
#include <boost/algorithm/string.hpp>
#include <boost/bind/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/locale.hpp>
#include <engines/contour/Engine.h>
#include <engines/geonames/Engine.h>
#include <engines/grid/Engine.h>
#include <engines/observation/Engine.h>
#include <engines/querydata/Engine.h>
#include <macgyver/Base64.h>
#include <macgyver/CacheStats.h>
#include <macgyver/StringConversion.h>
#include <macgyver/TimeFormatter.h>
#include <macgyver/TimeParser.h>
#include <spine/Convenience.h>
#include <spine/FmiApiKey.h>
#include <spine/SmartMet.h>
#include <spine/Table.h>
#include <spine/TableFormatterFactory.h>
#include <spine/TableFormatterOptions.h>
#include <functional>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace
{
void parseIntOption(std::set<int> &output, const std::string &option)
{
  if (option.empty())
    return;

  std::vector<std::string> parts;
  boost::algorithm::split(parts, option, boost::algorithm::is_any_of(","));
  for (const auto &p : parts)
    output.insert(Fmi::stoi(p));
}

bool sortRequestVector(const std::pair<std::string, std::string> &pair1,
                       const std::pair<std::string, std::string> &pair2)
{
  return pair1.first < pair2.first;
}

std::vector<std::pair<std::string, std::string>> getRequests()
{
  std::vector<std::pair<std::string, std::string>> ret = {
      {"clusterinfo", "Cluster information"},
      {"serviceinfo", "Currently provided services"},
      {"geonames", "Geonames information"},
      {"qengine", "Available querydata"},
      {"backends", "Backend information"},
      {"servicestats", "Service statistics"},
      {"producers", "Querydata producers"},
      {"parameters", "Querydata parameters"},
      {"obsproducers", "Observation producers"},
      {"obsparameters", "Observation parameters"},
      {"gridproducers", "Grid producers"},
      {"gridgenerations", "Grid generations"},
      {"gridgenerationsqd", "Grid newbase generations"},
      {"gridparameters", "Grid parameters"},
      {"activerequests", "Currently active requests"},
      {"stations", "Observation station information"},
      {"cachestats", "Cache statistics"}};

  return ret;
}

// ----------------------------------------------------------------------
/*!
 * \brief Set server (ContentEngine) logging activity
 */
// ----------------------------------------------------------------------

bool setLogging(SmartMet::Spine::Reactor &theReactor,
                const SmartMet::Spine::HTTP::Request &theRequest,
                SmartMet::Spine::HTTP::Response & /* theResponse */)
{
  try
  {
    // First parse if logging status change is requested
    auto loggingFlag = theRequest.getParameter("status");
    if (!loggingFlag)
      throw Fmi::Exception(BCP, "Logging parameter value not set.");

    std::string flag = *loggingFlag;
    // Logging status change requested
    if (flag == "enable")
    {
      theReactor.setLogging(true);
      return true;
    }
    if (flag == "disable")
    {
      theReactor.setLogging(false);
      return true;
    }
    throw Fmi::Exception(BCP, "Invalid logging parameter value: " + flag);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Set server (ContentEngine) logging activity status
 */
// ----------------------------------------------------------------------

bool getLogging(SmartMet::Spine::Reactor &theReactor,
                const SmartMet::Spine::HTTP::Request &theRequest,
                SmartMet::Spine::HTTP::Response &theResponse)
{
  using namespace SmartMet::Spine;

  try
  {
    Table reqTable;
    std::string format = optional_string(theRequest.getParameter("format"), "json");
    std::unique_ptr<TableFormatter> formatter(TableFormatterFactory::create(format));

    bool isCurrentlyLogging = theReactor.getLogging();

    if (isCurrentlyLogging)
      reqTable.set(0, 0, "Enabled");
    else
      reqTable.set(0, 0, "Disabled");

    std::vector<std::string> headers = {"LoggingStatus"};
    auto out = formatter->format(reqTable, headers, theRequest, TableFormatterOptions());

    std::string mime = formatter->mimetype() + "; charset=UTF-8";
    theResponse.setHeader("Content-Type", mime);

    // Set content
    theResponse.setContent(out);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace

namespace SmartMet
{
namespace Plugin
{
namespace Admin
{
namespace
{
std::string average_and_format(long total_microsecs, unsigned long requests)
{
  try
  {
    // Average global request time
    double average_time = total_microsecs / (1000.0 * requests);
    if (std::isnan(average_time))
      return "Not available";

    std::stringstream ss;
    ss << std::setprecision(4) << average_time;
    return ss.str();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}
}  // namespace

// ----------------------------------------------------------------------
/*!
 * \brief Perform an Admin query
 */
// ----------------------------------------------------------------------
bool Plugin::request(Spine::Reactor &theReactor,
                     const Spine::HTTP::Request &theRequest,
                     Spine::HTTP::Response &theResponse)
{
  try
  {
    std::string what = Spine::optional_string(theRequest.getParameter("what"), "");

    if (what.empty())
    {
      std::string ret("No request specified");
      theResponse.setContent(ret);
      return false;
    }

    using A = Spine::Reactor &;
    using B = const Spine::HTTP::Request &;
    using C = Spine::HTTP::Response &;

    std::map<std::string, std::function<bool(A a, B b, C c)>> handlers{
        {"clusterinfo", [this](A a, B b, C c) { return requestClusterInfo(a, b, c); }},
        {"serviceinfo", [this](A a, B b, C c) { return requestServiceInfo(a, b, c); }},
        {"reload", [this](A a, B b, C c) { return requestReload(a, b, c); }},
        {"geonames", [this](A a, B b, C c) { return requestGeonames(a, b, c); }},
        {"qengine", [this](A a, B b, C c) { return requestQEngineStatus(a, b, c); }},
        {"backends", [this](A a, B b, C c) { return requestBackendInfo(a, b, c); }},
        {"servicestats", [this](A a, B b, C c) { return requestServiceStats(a, b, c); }},
        {"producers", [this](A a, B b, C c) { return requestProducerInfo(a, b, c); }},
        {"parameters", [this](A a, B b, C c) { return requestParameterInfo(a, b, c); }},
        {"obsparameters", [this](A a, B b, C c) { return requestObsParameterInfo(a, b, c); }},
        {"obsproducers", [this](A a, B b, C c) { return requestObsProducerInfo(a, b, c); }},
        {"gridproducers", [this](A a, B b, C c) { return requestGridProducerInfo(a, b, c); }},
        {"gridgenerations", [this](A a, B b, C c) { return requestGridGenerationInfo(a, b, c); }},
        {"gridgenerationsqd",
         [this](A a, B b, C c) { return requestGridQdGenerationInfo(a, b, c); }},
        {"gridparameters", [this](A a, B b, C c) { return requestGridParameterInfo(a, b, c); }},
        {"setlogging", [this](A a, B b, C c) { return setLogging(a, b, c); }},
        {"getlogging", [this](A a, B b, C c) { return getLogging(a, b, c); }},
        {"lastrequests", [this](A a, B b, C c) { return requestLastRequests(a, b, c); }},
        {"activerequests", [this](A a, B b, C c) { return requestActiveRequests(a, b, c); }},
        {"pause", [this](A a, B b, C c) { return setPause(a, b, c); }},
        {"continue", [this](A a, B b, C c) { return setContinue(a, b, c); }},
        {"reloadstations", [this](A a, B b, C c) { return requestLoadStations(a, b, c); }},
        {"stations", [this](A a, B b, C c) { return requestObsStationInfo(a, b, c); }},
        {"list", [this](A a, B b, C c) { return listRequests(a, b, c); }},
        {"cachestats", [this](A a, B b, C c) { return requestCacheStats(a, b, c); }}};

    auto handler = handlers.find(what);
    if (handler != handlers.end())
      return handler->second(theReactor, theRequest, theResponse);

    // Unknown request,build response
    // Make MIME header
    std::string mime("text/html; charset=UTF-8");
    theResponse.setHeader("Content-Type", mime);

    std::string ret("Unknown admin request: '" + what + "'");
    theResponse.setContent(ret);
    return false;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform a clusterinfo query
 */
// ----------------------------------------------------------------------

bool Plugin::requestClusterInfo(Spine::Reactor & /* theReactor */,
                                const Spine::HTTP::Request & /* theRequest */,
                                Spine::HTTP::Response &theResponse)
{
  try
  {
    if (itsSputnik == nullptr)
    {
      theResponse.setContent("Sputnik engine is not available");
      return false;
    }

    std::ostringstream out;
    itsSputnik->status(out);

    // Make MIME header
    std::string mime("text/html; charset=UTF-8");
    theResponse.setHeader("Content-Type", mime);

    // Set content
    std::string ret = "<html><head><title>SmartMet Admin</title></head><body>";
    ret += out.str();
    ret += "</body></html>";
    theResponse.setContent(ret);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform a serviceinfo query
 */
// ----------------------------------------------------------------------

bool Plugin::requestServiceInfo(Spine::Reactor &theReactor,
                                const Spine::HTTP::Request & /* theRequest */,
                                Spine::HTTP::Response &theResponse)
{
  try
  {
    auto handlers = theReactor.getURIMap();

    std::string out =
        "<html><head><title>SmartMet Admin</title></head><body>\n"
        "<h3>Services currently provided by this server</h3>\n"
        "<ol>\n";
    for (const auto &handler : handlers)
      out += "<li>" + handler.first + "</li>\n";
    out += "</ol>\n";
    out += "</body></html>";

    // Make MIME header
    std::string mime("text/html; charset=UTF-8");
    theResponse.setHeader("Content-Type", mime);

    // Set content
    theResponse.setContent(out);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}
// ----------------------------------------------------------------------
/*!
 * \brief Perform a reload query
 */
// ----------------------------------------------------------------------

bool Plugin::requestReload(Spine::Reactor &theReactor,
                           const Spine::HTTP::Request & /* theRequest */,
                           Spine::HTTP::Response &theResponse)
{
  try
  {
    std::string out = "<html><head><title>SmartMet Admin</title></head><body>";

    auto *engine = theReactor.getSingleton("Geonames", nullptr);
    if (engine == nullptr)
    {
      theResponse.setContent("Geonames engine is not available");
      return false;
    }

    auto *geoengine = reinterpret_cast<Engine::Geonames::Engine *>(engine);
    time_t starttime = time(nullptr);
    if (!geoengine->reload())
    {
      out += "GeoEngine reload failed: ";
      out += geoengine->errorMessage();
      out += "\n";
      theResponse.setContent(out);
      return false;
    }

    time_t endtime = time(nullptr);
    long secs = endtime - starttime;
    out += "GeoEngine reloaded in ";
    out += Fmi::to_string(secs);
    out += " seconds\n";

    out += "</body></html>";

    // Make MIME header
    std::string mime("text/html; charset=UTF-8");
    theResponse.setHeader("Content-Type", mime);

    // Set content
    theResponse.setContent(out);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform a reloadstations query
 */
// ----------------------------------------------------------------------

bool Plugin::requestLoadStations(Spine::Reactor &theReactor,
                                 const Spine::HTTP::Request & /* theRequest */,
                                 Spine::HTTP::Response &theResponse)
{
  try
  {
    auto *engine = theReactor.getSingleton("Observation", nullptr);
    if (engine == nullptr)
    {
      theResponse.setContent("Observation engine is not available");
      return false;
    }

    auto *obsengine = reinterpret_cast<Engine::Observation::Engine *>(engine);

    obsengine->reloadStations();

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform a geonames status query
 */
// ----------------------------------------------------------------------

bool Plugin::requestGeonames(Spine::Reactor &theReactor,
                             const Spine::HTTP::Request &theRequest,
                             Spine::HTTP::Response &theResponse)
{
  try
  {
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "html");

    std::string dataType = Spine::optional_string(theRequest.getParameter("type"), "meta");

    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));

    auto *engine = theReactor.getSingleton("Geonames", nullptr);
    if (engine == nullptr)
    {
      std::string response = "Geonames engine is not available";
      theResponse.setContent(response);
      return false;
    }

    auto *geoengine = reinterpret_cast<Engine::Geonames::Engine *>(engine);

    Engine::Geonames::StatusReturnType status;

    if (dataType == "meta")
    {
      status = geoengine->metadataStatus();  // pair <table,headers>
    }
    else if (dataType == "cache")
    {
      status = geoengine->cacheStatus();  // pair <table,headers>
    }
    else
    {
      throw Fmi::Exception(BCP, "Invalid value for request parameter \"type\".");
    }

    // Make MIME header
    std::string mime(tableFormatter->mimetype() + "; charset=UTF-8");
    theResponse.setHeader("Content-Type", mime);

    auto out = tableFormatter->format(
        *status.first, status.second, theRequest, Spine::TableFormatterOptions());

    // Set content
    theResponse.setContent(out);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform qengine status query
 */
// ----------------------------------------------------------------------

bool Plugin::requestQEngineStatus(Spine::Reactor &theReactor,
                                  const Spine::HTTP::Request &theRequest,
                                  Spine::HTTP::Response &theResponse)
{
  try
  {
    // Get the Qengine
    auto *engine = theReactor.getSingleton("Querydata", nullptr);
    if (engine == nullptr)
    {
      std::string response = "Querydata engine not available";
      theResponse.setContent(response);
      return false;
    }

    auto *qengine = reinterpret_cast<Engine::Querydata::Engine *>(engine);

    // Optional producer filter
    std::string producer = Spine::optional_string(theRequest.getParameter("producer"), "");

    // Parse formatting options
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "debug");
    std::string projectionFormat =
        Spine::optional_string(theRequest.getParameter("projformat"), "newbase");

    if (tableFormat == "wxml")
    {
      std::string response = "Wxml formatting not supported";
      theResponse.setContent(response);
      return false;
    }

    std::string timeFormat = Spine::optional_string(theRequest.getParameter("timeformat"), "sql");

    std::pair<boost::shared_ptr<Spine::Table>, Spine::TableFormatter::Names> statusResult =
        qengine->getEngineContents(producer, timeFormat, projectionFormat);

    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));

    auto out = tableFormatter->format(
        *statusResult.first, statusResult.second, theRequest, Spine::TableFormatterOptions());

    if (tableFormat != "html")
      theResponse.setContent(out);
    else
    {
      // Only insert tags if using human readable mode
      std::string ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>"
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>"
          "<h1>QEngine Memory Mapped Files</h1>";
      ret += out;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform producer query
 */
// ----------------------------------------------------------------------

bool Plugin::requestProducerInfo(Spine::Reactor &theReactor,
                                 const Spine::HTTP::Request &theRequest,
                                 Spine::HTTP::Response &theResponse)
{
  try
  {
    // Get the Qengine
    auto *engine = theReactor.getSingleton("Querydata", nullptr);
    if (engine == nullptr)
    {
      std::string response = "Querydata engine not available";
      theResponse.setContent(response);
      return false;
    }
    auto *qengine = reinterpret_cast<Engine::Querydata::Engine *>(engine);

    // Parse formatting options
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "debug");

    if (tableFormat == "wxml")
    {
      std::string response = "Wxml formatting not supported";
      theResponse.setContent(response);
      return false;
    }

    // Optional producer filter
    auto producer = theRequest.getParameter("producer");

    std::string timeFormat = Spine::optional_string(theRequest.getParameter("timeformat"), "sql");

    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));

    std::pair<boost::shared_ptr<Spine::Table>, Spine::TableFormatter::Names> qengineProducerInfo =
        qengine->getProducerInfo(timeFormat, producer);

    auto qengine_out_producer = tableFormatter->format(*qengineProducerInfo.first,
                                                       qengineProducerInfo.second,
                                                       theRequest,
                                                       Spine::TableFormatterOptions());

    if (tableFormat == "html" || tableFormat == "debug")
      qengine_out_producer.insert(0, "<h1>Querydata producers</h1>");

    if (tableFormat != "html")
      theResponse.setContent(qengine_out_producer);
    else
    {
      // Only insert tags if using human readable mode
      std::string ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>";
      ret +=
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>";
      ret += qengine_out_producer;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform parameter query
 */
// ----------------------------------------------------------------------

bool Plugin::requestParameterInfo(Spine::Reactor &theReactor,
                                  const Spine::HTTP::Request &theRequest,
                                  Spine::HTTP::Response &theResponse)
{
  try
  {
    // Get the Qengine
    auto *engine = theReactor.getSingleton("Querydata", nullptr);
    if (engine == nullptr)
    {
      std::string response = "Querydata engine not available";
      theResponse.setContent(response);
      return false;
    }
    auto *qengine = reinterpret_cast<Engine::Querydata::Engine *>(engine);

    // Parse formatting options
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "debug");

    if (tableFormat == "wxml")
    {
      std::string response = "Wxml formatting not supported";
      theResponse.setContent(response);
      return false;
    }

    // Optional producer filter
    auto producer = theRequest.getParameter("producer");

    std::string timeFormat = Spine::optional_string(theRequest.getParameter("timeformat"), "sql");

    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));

    std::pair<boost::shared_ptr<Spine::Table>, Spine::TableFormatter::Names> qengineParameterInfo =
        qengine->getParameterInfo(producer);

    auto qengine_out_parameter = tableFormatter->format(*qengineParameterInfo.first,
                                                        qengineParameterInfo.second,
                                                        theRequest,
                                                        Spine::TableFormatterOptions());

    if (tableFormat == "html" || tableFormat == "debug")
      qengine_out_parameter.insert(0, "<h1>Querydata parameters</h1>");

    if (tableFormat != "html")
      theResponse.setContent(qengine_out_parameter);
    else
    {
      // Only insert tags if using human readable mode
      std::string ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>";
      ret +=
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>";
      ret += qengine_out_parameter;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform station group query
 */
// ----------------------------------------------------------------------

bool Plugin::requestObsProducerInfo(Spine::Reactor &theReactor,
                                    const Spine::HTTP::Request &theRequest,
                                    Spine::HTTP::Response &theResponse)
{
  try
  {
    // Get the Obsengine
    auto *engine = theReactor.getSingleton("Observation", nullptr);
    if (engine == nullptr)
    {
      std::string response = "Observation engine not available";
      theResponse.setContent(response);
      return false;
    }
    auto *obsengine = reinterpret_cast<Engine::Observation::Engine *>(engine);

    // Parse formatting options
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "debug");

    if (tableFormat == "wxml")
    {
      std::string response = "Wxml formatting not supported";
      theResponse.setContent(response);
      return false;
    }

    // Optional producer filter
    auto producer = theRequest.getParameter("producer");

    std::string timeFormat = Spine::optional_string(theRequest.getParameter("timeformat"), "sql");

    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));

    std::pair<boost::shared_ptr<Spine::Table>, Spine::TableFormatter::Names> obsengineProducerInfo =
        obsengine->getProducerInfo(producer);

    auto observation_out_producer = tableFormatter->format(*obsengineProducerInfo.first,
                                                           obsengineProducerInfo.second,
                                                           theRequest,
                                                           Spine::TableFormatterOptions());

    if (tableFormat == "html" || tableFormat == "debug")
      observation_out_producer.insert(0, "<h1>Observation producers</h1>");

    if (tableFormat != "html")
      theResponse.setContent(observation_out_producer);
    else
    {
      // Only insert tags if using human readable mode
      std::string ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>";
      ret +=
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>";
      ret += observation_out_producer;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform observation query
 */
// ----------------------------------------------------------------------

bool Plugin::requestObsParameterInfo(Spine::Reactor &theReactor,
                                     const Spine::HTTP::Request &theRequest,
                                     Spine::HTTP::Response &theResponse)
{
  try
  {
    // Get the Obsengine
    auto *engine = theReactor.getSingleton("Observation", nullptr);
    if (engine == nullptr)
    {
      std::string response = "Observation engine not available";
      theResponse.setContent(response);
      return false;
    }
    auto *obsengine = reinterpret_cast<Engine::Observation::Engine *>(engine);

    // Parse formatting options
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "debug");

    if (tableFormat == "wxml")
    {
      std::string response = "Wxml formatting not supported";
      theResponse.setContent(response);
      return false;
    }

    // Optional producer filter
    auto producer = theRequest.getParameter("producer");

    std::string timeFormat = Spine::optional_string(theRequest.getParameter("timeformat"), "sql");

    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));

    std::pair<boost::shared_ptr<Spine::Table>, Spine::TableFormatter::Names>
        obsengineParameterInfo = obsengine->getParameterInfo(producer);

    auto observation_out_parameter = tableFormatter->format(*obsengineParameterInfo.first,
                                                            obsengineParameterInfo.second,
                                                            theRequest,
                                                            Spine::TableFormatterOptions());

    if (tableFormat == "html" || tableFormat == "debug")
      observation_out_parameter.insert(0, "<h1>Observation parameters</h1>");

    if (tableFormat != "html")
      theResponse.setContent(observation_out_parameter);
    else
    {
      // Only insert tags if using human readable mode
      std::string ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>";
      ret +=
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>";
      ret += observation_out_parameter;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool Plugin::requestGridProducerInfo(Spine::Reactor &theReactor,
                                     const Spine::HTTP::Request &theRequest,
                                     Spine::HTTP::Response &theResponse)
{
  try
  {
    // Get the Obsengine
    auto *engine = theReactor.getSingleton("grid", nullptr);
    if (engine == nullptr)
    {
      std::string response = "Grid engine not available";
      theResponse.setContent(response);
      return false;
    }
    auto *gridEngine = reinterpret_cast<Engine::Grid::Engine *>(engine);

    // Parse formatting options
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "debug");

    if (tableFormat == "wxml")
    {
      std::string response = "Wxml formatting not supported";
      theResponse.setContent(response);
      return false;
    }

    // Optional producer filter
    auto producer = theRequest.getParameter("producer");

    std::string timeFormat = Spine::optional_string(theRequest.getParameter("timeformat"), "sql");
    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));
    std::pair<boost::shared_ptr<Spine::Table>, Spine::TableFormatter::Names> producerInfo =
        gridEngine->getProducerInfo(producer, timeFormat);
    auto grid_out_producer = tableFormatter->format(
        *producerInfo.first, producerInfo.second, theRequest, Spine::TableFormatterOptions());

    if (tableFormat == "html" || tableFormat == "debug")
      grid_out_producer.insert(0, "<h1>Grid producers</h1>");

    if (tableFormat != "html")
      theResponse.setContent(grid_out_producer);
    else
    {
      // Only insert tags if using human readable mode
      std::string ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>";
      ret +=
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>";
      ret += grid_out_producer;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool Plugin::requestGridGenerationInfo(Spine::Reactor &theReactor,
                                       const Spine::HTTP::Request &theRequest,
                                       Spine::HTTP::Response &theResponse)
{
  try
  {
    // Get the Obsengine
    auto *engine = theReactor.getSingleton("grid", nullptr);
    if (engine == nullptr)
    {
      std::string response = "Grid engine not available";
      theResponse.setContent(response);
      return false;
    }
    auto *gridEngine = reinterpret_cast<Engine::Grid::Engine *>(engine);

    // Parse formatting options
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "debug");

    if (tableFormat == "wxml")
    {
      std::string response = "Wxml formatting not supported";
      theResponse.setContent(response);
      return false;
    }

    // Optional producer filter
    auto producer = theRequest.getParameter("producer");

    std::string timeFormat = Spine::optional_string(theRequest.getParameter("timeformat"), "sql");
    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));
    std::pair<boost::shared_ptr<Spine::Table>, Spine::TableFormatter::Names> producerInfo =
        gridEngine->getGenerationInfo(producer, timeFormat);
    auto grid_out_producer = tableFormatter->format(
        *producerInfo.first, producerInfo.second, theRequest, Spine::TableFormatterOptions());

    if (tableFormat == "html" || tableFormat == "debug")
      grid_out_producer.insert(0, "<h1>Grid generations</h1>");

    if (tableFormat != "html")
      theResponse.setContent(grid_out_producer);
    else
    {
      // Only insert tags if using human readable mode
      std::string ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>";
      ret +=
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>";
      ret += grid_out_producer;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool Plugin::requestGridQdGenerationInfo(Spine::Reactor &theReactor,
                                         const Spine::HTTP::Request &theRequest,
                                         Spine::HTTP::Response &theResponse)
{
  try
  {
    // Get the Obsengine
    auto *engine = theReactor.getSingleton("grid", nullptr);
    if (engine == nullptr)
    {
      std::string response = "Grid engine not available";
      theResponse.setContent(response);
      return false;
    }
    auto *gridEngine = reinterpret_cast<Engine::Grid::Engine *>(engine);

    // Parse formatting options
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "debug");

    if (tableFormat == "wxml")
    {
      std::string response = "Wxml formatting not supported";
      theResponse.setContent(response);
      return false;
    }

    // Optional producer filter
    auto producer = theRequest.getParameter("producer");

    std::string timeFormat = Spine::optional_string(theRequest.getParameter("timeformat"), "sql");
    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));
    std::pair<boost::shared_ptr<Spine::Table>, Spine::TableFormatter::Names> producerInfo =
        gridEngine->getExtGenerationInfo(producer, timeFormat);
    auto grid_out_producer = tableFormatter->format(
        *producerInfo.first, producerInfo.second, theRequest, Spine::TableFormatterOptions());

    if (tableFormat == "html" || tableFormat == "debug")
      grid_out_producer.insert(0, "<h1>Grid Newbase generations</h1>");

    if (tableFormat != "html")
      theResponse.setContent(grid_out_producer);
    else
    {
      // Only insert tags if using human readable mode
      std::string ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>";
      ret +=
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>";
      ret += grid_out_producer;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool Plugin::requestGridParameterInfo(Spine::Reactor &theReactor,
                                      const Spine::HTTP::Request &theRequest,
                                      Spine::HTTP::Response &theResponse)
{
  try
  {
    // Get the Obsengine
    auto *engine = theReactor.getSingleton("grid", nullptr);
    if (engine == nullptr)
    {
      std::string response = "Grid engine not available";
      theResponse.setContent(response);
      return false;
    }
    auto *gridEngine = reinterpret_cast<Engine::Grid::Engine *>(engine);

    // Parse formatting options
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "debug");

    if (tableFormat == "wxml")
    {
      std::string response = "Wxml formatting not supported";
      theResponse.setContent(response);
      return false;
    }

    // Optional producer filter
    auto producer = theRequest.getParameter("producer");

    std::string timeFormat = Spine::optional_string(theRequest.getParameter("timeformat"), "sql");

    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));

    std::pair<boost::shared_ptr<Spine::Table>, Spine::TableFormatter::Names>
        gridEngineParameterInfo = gridEngine->getParameterInfo(producer);

    auto grid_out_parameter = tableFormatter->format(*gridEngineParameterInfo.first,
                                                     gridEngineParameterInfo.second,
                                                     theRequest,
                                                     Spine::TableFormatterOptions());

    if (tableFormat == "html" || tableFormat == "debug")
      grid_out_parameter.insert(0, "<h1>Grid parameters</h1>");

    if (tableFormat != "html")
      theResponse.setContent(grid_out_parameter);
    else
    {
      // Only insert tags if using human readable mode
      std::string ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>";
      ret +=
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>";
      ret += grid_out_parameter;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform a backends query
 */
// ----------------------------------------------------------------------

bool Plugin::requestBackendInfo(Spine::Reactor & /* theReactor */,
                                const Spine::HTTP::Request &theRequest,
                                Spine::HTTP::Response &theResponse)
{
  try
  {
    std::string service = Spine::optional_string(theRequest.getParameter("service"), "");
    std::string format = Spine::optional_string(theRequest.getParameter("format"), "debug");

    if (format == "wxml")
    {
      std::string response = "Wxml formatting not supported";
      theResponse.setContent(response);
      return false;
    }

    if (itsSputnik == nullptr)
    {
      std::string response = "Sputnik engine is not available";
      theResponse.setContent(response);
      return false;
    }

    std::shared_ptr<Spine::Table> table = itsSputnik->backends(service);

    boost::shared_ptr<Spine::TableFormatter> formatter(
        Spine::TableFormatterFactory::create(format));
    Spine::TableFormatter::Names names;
    names.push_back("Backend");
    names.push_back("IP");
    names.push_back("Port");

    auto out = formatter->format(*table, names, theRequest, Spine::TableFormatterOptions());

    std::ostringstream status;
    itsSputnik->status(status);
    out += status.str();

    // Make MIME header

    std::string mime = formatter->mimetype() + "; charset=UTF-8";
    theResponse.setHeader("Content-Type", mime);

    // Set content
    if (format != "html")
      theResponse.setContent(out);
    else
    {
      // Add html tags only when using human readable format
      std::string ret = "<html><head><title>SmartMet Admin</title></head><body>";
      ret += out;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get server (ContentEngine) logged requests (last N minutes)
 */
// ----------------------------------------------------------------------

bool Plugin::requestLastRequests(Spine::Reactor &theReactor,
                                 const Spine::HTTP::Request &theRequest,
                                 Spine::HTTP::Response &theResponse)
{
  using namespace boost::placeholders;
  try
  {
    Spine::Table reqTable;
    std::string format = Spine::optional_string(theRequest.getParameter("format"), "json");
    std::unique_ptr<Spine::TableFormatter> formatter(Spine::TableFormatterFactory::create(format));

    auto givenMinutes = theRequest.getParameter("minutes");
    unsigned int minutes = 1;
    if (givenMinutes)
      minutes = boost::lexical_cast<unsigned int>(*givenMinutes);

    // Obtain logging information
    std::string pluginName = Spine::optional_string(theRequest.getParameter("plugin"), "all");
    auto currentRequests =
        theReactor.getLoggedRequests(pluginName);  // This is type tuple<bool,LogRange,posix_time>

    auto firstValidTime =
        boost::posix_time::second_clock::local_time() - boost::posix_time::minutes(minutes);

    std::size_t row = 0;
    for (const auto &req : std::get<1>(currentRequests))
    {
      auto firstConsidered = std::find_if(req.second.begin(),
                                          req.second.end(),
                                          [&firstValidTime](const Spine::LoggedRequest &compare)
                                          { return compare.getRequestEndTime() > firstValidTime; });

      for (auto reqIt = firstConsidered; reqIt != req.second.end();
           ++reqIt)  // NOLINT(modernize-loop-convert)
      {
        std::size_t column = 0;

        std::string endtime = Fmi::to_iso_extended_string(reqIt->getRequestEndTime().time_of_day());

        std::string msec_duration = average_and_format(
            reqIt->getAccessDuration().total_microseconds(), 1);  // just format the single duration

        reqTable.set(column, row, endtime);
        ++column;

        reqTable.set(column, row, msec_duration);
        ++column;

        reqTable.set(column, row, reqIt->getRequestString());

        ++row;
      }
    }

    std::vector<std::string> headers = {"Time", "Duration", "RequestString"};
    auto out = formatter->format(reqTable, headers, theRequest, Spine::TableFormatterOptions());

    // Set MIME
    std::string mime = formatter->mimetype() + "; charset=UTF-8";
    theResponse.setHeader("Content-Type", mime);

    // Set content
    theResponse.setContent(out);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get server active requests
 */
// ----------------------------------------------------------------------

bool Plugin::requestActiveRequests(Spine::Reactor &theReactor,
                                   const Spine::HTTP::Request &theRequest,
                                   Spine::HTTP::Response &theResponse)
{
  try
  {
    Spine::Table reqTable;
    std::string format = Spine::optional_string(theRequest.getParameter("format"), "json");
    std::unique_ptr<Spine::TableFormatter> formatter(Spine::TableFormatterFactory::create(format));

    // Obtain logging information
    auto requests = theReactor.getActiveRequests();

    auto now = boost::posix_time::microsec_clock::universal_time();

    std::size_t row = 0;
    for (const auto &id_info : requests)
    {
      const auto id = id_info.first;
      const auto &time = id_info.second.time;
      const auto &req = id_info.second.request;

      auto duration = now - time;

      const bool check_access_token = true;
      auto apikey = Spine::FmiApiKey::getFmiApiKey(req, check_access_token);

      std::size_t column = 0;
      reqTable.set(column++, row, Fmi::to_string(id));
      reqTable.set(column++, row, Fmi::to_iso_extended_string(time.time_of_day()));
      reqTable.set(column++, row, Fmi::to_string(duration.total_milliseconds() / 1000.0));
      reqTable.set(column++, row, req.getClientIP());
      reqTable.set(column++, row, apikey ? *apikey : "-");
      reqTable.set(column++, row, req.getURI());
      ++row;
    }

    std::vector<std::string> headers = {
        "Id", "Time", "Duration", "ClientIP", "Apikey", "RequestString"};
    auto out = formatter->format(reqTable, headers, theRequest, Spine::TableFormatterOptions());

    // Set MIME
    std::string mime = formatter->mimetype() + "; charset=UTF-8";
    theResponse.setHeader("Content-Type", mime);

    // Set content
    theResponse.setContent(out);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool Plugin::requestObsStationInfo(Spine::Reactor &theReactor,
                                   const Spine::HTTP::Request &theRequest,
                                   Spine::HTTP::Response &theResponse)
{
  try
  {
    // Get the Obsengine
    auto *engine = theReactor.getSingleton("Observation", nullptr);
    if (engine == nullptr)
    {
      std::string response = "Observation engine not available";
      theResponse.setContent(response);
      return false;
    }
    auto *obsengine = reinterpret_cast<Engine::Observation::Engine *>(engine);

    // Parse formatting options
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "debug");

    if (tableFormat == "wxml")
    {
      std::string response = "Wxml formatting not supported";
      theResponse.setContent(response);
      return false;
    }

    std::string timeFormat = Spine::optional_string(theRequest.getParameter("timeformat"), "sql");

    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));

    Engine::Observation::StationOptions options;
    parseIntOption(options.fmisid, Spine::optional_string(theRequest.getParameter("fmisid"), ""));
    parseIntOption(options.lpnn, Spine::optional_string(theRequest.getParameter("lpnn"), ""));
    parseIntOption(options.wmo, Spine::optional_string(theRequest.getParameter("wmo"), ""));
    parseIntOption(options.rwsid, Spine::optional_string(theRequest.getParameter("rwsid"), ""));
    options.type = Spine::optional_string(theRequest.getParameter("type"), "");
    options.name = Spine::optional_string(theRequest.getParameter("name"), "");
    options.iso2 = Spine::optional_string(theRequest.getParameter("country"), "");
    options.region = Spine::optional_string(theRequest.getParameter("region"), "");
    options.timeformat = Spine::optional_string(theRequest.getParameter("timeformat"), "iso");
    std::string starttime = Spine::optional_string(theRequest.getParameter("starttime"), "");
    std::string endtime = Spine::optional_string(theRequest.getParameter("endtime"), "");
    if (!starttime.empty())
      options.start_time = Fmi::TimeParser::parse(starttime);
    else
      options.start_time = boost::posix_time::not_a_date_time;
    if (!endtime.empty())
      options.end_time = Fmi::TimeParser::parse(endtime);
    else
      options.end_time = boost::posix_time::not_a_date_time;

    std::string bbox_string = Spine::optional_string(theRequest.getParameter("bbox"), "");
    if (!bbox_string.empty())
      options.bbox = Spine::BoundingBox(bbox_string);

    std::pair<boost::shared_ptr<Spine::Table>, Spine::TableFormatter::Names> obsengineStationInfo =
        obsengine->getStationInfo(options);

    auto stations_out = tableFormatter->format(*obsengineStationInfo.first,
                                               obsengineStationInfo.second,
                                               theRequest,
                                               Spine::TableFormatterOptions());

    if (tableFormat == "html" || tableFormat == "debug")
      stations_out.insert(0, "<h1>Observation stations</h1>");

    if (tableFormat != "html")
      theResponse.setContent(stations_out);
    else
    {
      // Only insert tags if using human readable mode
      std::string ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>";
      ret +=
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>";
      ret += stations_out;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool Plugin::requestCacheStats(Spine::Reactor &theReactor,
                               const Spine::HTTP::Request &theRequest,
                               Spine::HTTP::Response &theResponse)
{
  try
  {
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "html");
    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));
    boost::shared_ptr<Spine::Table> table(new Spine::Table());
    Spine::TableFormatter::Names header_names{"#",
                                              "cache_name",
                                              "maxsize",
                                              "size",
                                              "inserts",
                                              "hits",
                                              "misses",
                                              "hitrate",
                                              "hits/min",
                                              "inserts/min",
                                              "created",
                                              "age"};

    auto now = boost::posix_time::microsec_clock::universal_time();
    auto cache_stats = theReactor.getCacheStats();

    Spine::Table data_table;

    auto timeFormat = Spine::optional_string(theRequest.getParameter("timeformat"), "sql");
    std::unique_ptr<Fmi::TimeFormatter> timeFormatter(Fmi::TimeFormatter::create(timeFormat));

    size_t row = 1;
    for (const auto &item : cache_stats)
    {
      const auto &name = item.first;
      const auto &stat = item.second;
      auto duration = (now - stat.starttime).total_seconds();
      auto n = stat.hits + stat.misses;
      auto hit_rate = (n == 0 ? 0.0 : stat.hits * 100.0 / n);
      auto hits_per_min = (duration == 0 ? 0.0 : 60.0 * stat.hits / duration);
      auto inserts_per_min = (duration == 0 ? 0.0 : 60.0 * stat.inserts / duration);

      data_table.set(0, row, Fmi::to_string(row));
      data_table.set(1, row, name);
      data_table.set(2, row, Fmi::to_string(stat.maxsize));
      data_table.set(3, row, Fmi::to_string(stat.size));
      data_table.set(4, row, Fmi::to_string(stat.inserts));
      data_table.set(5, row, Fmi::to_string(stat.hits));
      data_table.set(6, row, Fmi::to_string(stat.misses));
      data_table.set(7, row, Fmi::to_string("%.1f", hit_rate));
      data_table.set(8, row, Fmi::to_string("%.1f", hits_per_min));
      data_table.set(9, row, Fmi::to_string("%.1f", inserts_per_min));
      data_table.set(10, row, timeFormatter->format(stat.starttime));
      data_table.set(11, row, Fmi::to_simple_string(now - stat.starttime));
      row++;
    }

    auto cache_stats_output = tableFormatter->format(
        data_table, header_names, theRequest, Spine::TableFormatterOptions());

    if (tableFormat == "html" || tableFormat == "debug")
      cache_stats_output.insert(0, "<h1>CacheStatistics</h1>");

    if (tableFormat != "html")
      theResponse.setContent(cache_stats_output);
    else
    {
      // Only insert tags if using human readable mode
      std::string ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>";
      ret +=
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>";
      ret += cache_stats_output;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime);
    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Lists all requests supported by admin plugin
 */
// ----------------------------------------------------------------------

bool Plugin::listRequests(Spine::Reactor & /* theReactor */,
                          const Spine::HTTP::Request &theRequest,
                          Spine::HTTP::Response &theResponse)
{
  try
  {
    // Parse formatting options
    std::string tableFormat = Spine::optional_string(theRequest.getParameter("format"), "debug");

    if (tableFormat == "wxml")
    {
      std::string response = "Wxml formatting not supported";
      theResponse.setContent(response);
      return false;
    }

    std::unique_ptr<Spine::TableFormatter> tableFormatter(
        Spine::TableFormatterFactory::create(tableFormat));

    //   std::pair<boost::shared_ptr<Spine::Table>, Spine::TableFormatter::Names>
    //   obsengineStationInfo =  obsengine->getStationInfo(options);

    Spine::Table resultTable;
    Spine::TableFormatter::Names headers{"Request", "Response"};

    std::vector<std::pair<std::string, std::string>> requests = getRequests();
    std::sort(requests.begin(), requests.end(), sortRequestVector);

    unsigned int row = 0;
    for (const auto &r : requests)
    {
      resultTable.set(0, row, r.first);
      resultTable.set(1, row, r.second);
      row++;
    }

    auto requests_out =
        tableFormatter->format(resultTable, headers, theRequest, Spine::TableFormatterOptions());

    if (tableFormat == "html" || tableFormat == "debug")
      requests_out.insert(0, "<h1>Admin requests</h1>");

    if (tableFormat != "html")
      theResponse.setContent(requests_out);
    else
    {
      // Only insert tags if using human readable mode
      std::string ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>";
      ret +=
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>";
      ret += requests_out;
      ret += "</body></html>";
      theResponse.setContent(ret);
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform a backends query
 */
// ----------------------------------------------------------------------

bool Plugin::requestServiceStats(Spine::Reactor &theReactor,
                                 const Spine::HTTP::Request &theRequest,
                                 Spine::HTTP::Response &theResponse)
{
  try
  {
    std::vector<std::string> headers{
        "Handler", "LastMinute", "LastHour", "Last24Hours", "AverageDuration"};

    std::string formatstream;

    Spine::Table statsTable;

    std::string format = Spine::optional_string(theRequest.getParameter("format"), "json");

    std::unique_ptr<Spine::TableFormatter> formatter(Spine::TableFormatterFactory::create(format));

    std::string pluginName = Spine::optional_string(theRequest.getParameter("plugin"), "all");
    auto currentRequests =
        theReactor.getLoggedRequests(pluginName);  // This is type tuple<bool,LogRange,posix_time>

    auto currentTime = boost::posix_time::microsec_clock::local_time();

    std::size_t row = 0;
    unsigned long total_minute = 0;
    unsigned long total_hour = 0;
    unsigned long total_day = 0;
    long global_microsecs = 0;

    for (const auto &reqpair : std::get<1>(currentRequests))
    {
      // Lets calculate how many hits we have in minute,hour and day and since start
      unsigned long inMinute = 0;
      unsigned long inHour = 0;
      unsigned long inDay = 0;
      long total_microsecs = 0;
      // We go from newest to oldest

      for (const auto &item : reqpair.second)
      {
        auto sinceDuration = currentTime - item.getRequestEndTime();
        auto accessDuration = item.getAccessDuration();

        total_microsecs += accessDuration.total_microseconds();

        global_microsecs += accessDuration.total_microseconds();

        if (sinceDuration < boost::posix_time::hours(24))
        {
          ++inDay;
          ++total_day;
          if (sinceDuration < boost::posix_time::hours(1))
          {
            ++inHour;
            ++total_hour;
            if (sinceDuration < boost::posix_time::minutes(1))
            {
              ++inMinute;
              ++total_minute;
            }
          }
        }
      }

      std::size_t column = 0;

      statsTable.set(column, row, reqpair.first);
      ++column;

      statsTable.set(column, row, Fmi::to_string(inMinute));
      ++column;

      statsTable.set(column, row, Fmi::to_string(inHour));
      ++column;

      statsTable.set(column, row, Fmi::to_string(inDay));
      ++column;

      std::string msecs = average_and_format(total_microsecs, inDay);
      statsTable.set(column, row, msecs);

      ++row;
    }

    // Finally insert totals
    std::size_t column = 0;

    statsTable.set(column, row, "Total requests");
    ++column;

    statsTable.set(column, row, Fmi::to_string(total_minute));
    ++column;

    statsTable.set(column, row, Fmi::to_string(total_hour));
    ++column;

    statsTable.set(column, row, Fmi::to_string(total_day));
    ++column;

    std::string msecs = average_and_format(global_microsecs, total_day);
    statsTable.set(column, row, msecs);

    auto out = formatter->format(statsTable, headers, theRequest, Spine::TableFormatterOptions());

    // Set MIME
    std::string mime = formatter->mimetype() + "; charset=UTF-8";
    theResponse.setHeader("Content-Type", mime);

    // Set content
    theResponse.setContent(out);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Request a pause
 */
// ----------------------------------------------------------------------

bool Plugin::setPause(Spine::Reactor & /* theReactor */,
                      const Spine::HTTP::Request &theRequest,
                      Spine::HTTP::Response &theResponse)
{
  try
  {
    theResponse.setHeader("Content-Type", "text/plain");

    if (itsSputnik == nullptr)
    {
      theResponse.setContent("No Sputnik process to pause");
      return true;
    }

    // Optional deadline or duration:

    auto time_opt = theRequest.getParameter("time");
    auto duration_opt = theRequest.getParameter("duration");

    if (time_opt)
    {
      auto deadline = Fmi::TimeParser::parse(*time_opt);
      itsSputnik->setPauseUntil(deadline);
      theResponse.setContent("Paused Sputnik until " + Fmi::to_iso_string(deadline));
    }
    else if (duration_opt)
    {
      auto duration = Fmi::TimeParser::parse_duration(*duration_opt);
      auto deadline = boost::posix_time::second_clock::universal_time() + duration;
      itsSputnik->setPauseUntil(deadline);
      theResponse.setContent("Paused Sputnik until " + Fmi::to_iso_string(deadline));
    }
    else
    {
      itsSputnik->setPause();
      theResponse.setContent("Paused Sputnik until a continue request arrives");
    }

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Request a continue
 */
// ----------------------------------------------------------------------

bool Plugin::setContinue(Spine::Reactor & /* theReactor */,
                         const Spine::HTTP::Request &theRequest,
                         Spine::HTTP::Response &theResponse)
{
  try
  {
    theResponse.setHeader("Content-Type", "text/plain");

    if (itsSputnik == nullptr)
    {
      theResponse.setContent("No Sputnik process to continue");
      return true;
    }

    // Optional deadline or duration:

    auto time_opt = theRequest.getParameter("time");
    auto duration_opt = theRequest.getParameter("duration");

    if (time_opt)
    {
      auto deadline = Fmi::TimeParser::parse(*time_opt);
      itsSputnik->setPauseUntil(deadline);
      theResponse.setContent("Paused Sputnik until " + Fmi::to_iso_string(deadline));
    }
    else if (duration_opt)
    {
      auto duration = Fmi::TimeParser::parse_duration(*duration_opt);
      auto deadline = boost::posix_time::second_clock::universal_time() + duration;
      itsSputnik->setPauseUntil(deadline);
      theResponse.setContent("Paused Sputnik until " + Fmi::to_iso_string(deadline));
    }
    else
    {
      itsSputnik->setContinue();
      theResponse.setContent("Sputnik continue request made");
    }

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Main content handler
 */
// ----------------------------------------------------------------------

void Plugin::requestHandler(Spine::Reactor &theReactor,
                            const Spine::HTTP::Request &theRequest,
                            Spine::HTTP::Response &theResponse)
{
  try
  {
    // Check authentication first
    bool hasValidAuthentication = authenticateRequest(theRequest, theResponse);

    if (!hasValidAuthentication)
      return;  // Auhentication failure

    if (checkRequest(theRequest, theResponse, false))
    {
      return;
    }

    try
    {
      // We return JSON, hence we should enable CORS
      theResponse.setHeader("Access-Control-Allow-Origin", "*");

      const int expires_seconds = 1;
      boost::posix_time::ptime t_now = boost::posix_time::second_clock::universal_time();

      bool response = request(theReactor, theRequest, theResponse);

      if (response)
        theResponse.setStatus(Spine::HTTP::Status::ok);
      else
        theResponse.setStatus(Spine::HTTP::Status::not_implemented);

      // Adding response headers

      boost::posix_time::ptime t_expires = t_now + boost::posix_time::seconds(expires_seconds);
      boost::shared_ptr<Fmi::TimeFormatter> tformat(Fmi::TimeFormatter::create("http"));
      std::string cachecontrol = "public, max-age=" + std::to_string(expires_seconds);
      std::string expiration = tformat->format(t_expires);
      std::string modification = tformat->format(t_now);

      theResponse.setHeader("Cache-Control", cachecontrol);
      theResponse.setHeader("Expires", expiration);
      theResponse.setHeader("Last-Modified", modification);

      /* This will need some thought
             if(response.first.size() == 0)
             {
             std::cerr << "Warning: Empty input for request "
             << theRequest.getOriginalQueryString()
             << " from "
             << theRequest.getClientIP()
             << std::endl;
             }
      */

#ifdef MYDEBUG
      std::cout << "Output:" << std::endl << response << std::endl;
#endif
    }
    catch (...)
    {
      // Catching all exceptions

      Fmi::Exception exception(BCP, "Request processing exception!", nullptr);
      exception.addParameter("URI", theRequest.getURI());
      exception.addParameter("ClientIP", theRequest.getClientIP());
      exception.printError();

      theResponse.setStatus(Spine::HTTP::Status::bad_request);

      // Adding the first exception information into the response header

      std::string firstMessage = exception.what();
      boost::algorithm::replace_all(firstMessage, "\n", " ");
      firstMessage = firstMessage.substr(0, 300);
      theResponse.setHeader("X-Admin-Error", firstMessage);
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Plugin constructor
 */
// ----------------------------------------------------------------------

Plugin::Plugin(Spine::Reactor *theReactor, const char *theConfig) : itsModuleName("Admin")
{
  using namespace boost::placeholders;
  try
  {
    if (theReactor->getRequiredAPIVersion() != SMARTMET_API_VERSION)
      throw Fmi::Exception(BCP, "Admin plugin and Server API version mismatch");

    // Enable sensible relative include paths
    boost::filesystem::path p = theConfig;
    p.remove_filename();
    itsConfig.setIncludeDir(p.c_str());

    itsConfig.readFile(theConfig);

    // Password must be specified
    if (!itsConfig.exists("password"))
      throw Fmi::Exception(BCP, "Password not specified in the config file");

    // User must be specified
    if (!itsConfig.exists("user"))
      throw Fmi::Exception(BCP, "User not specified in the config file");

    // Get Sputnik if available
    auto *engine = theReactor->getSingleton("Sputnik", nullptr);
    if (engine != nullptr)
      itsSputnik = reinterpret_cast<Engine::Sputnik::Engine *>(engine);

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
 * \brief Return true if a service requiring authentication is requested
 */
// ----------------------------------------------------------------------

bool Plugin::isAuthenticationRequired(const Spine::HTTP::Request &theRequest) const
{
  try
  {
    std::string what = Spine::optional_string(theRequest.getParameter("what"), "");

    if (what == "reload")
      return true;
    if (what == "pause")
      return true;
    if (what == "continue")
      return true;

    return false;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

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
 * \brief Performance query implementation.
 *
 * We want admin calls to always be processed ASAP
 */
// ----------------------------------------------------------------------

bool Plugin::queryIsFast(const Spine::HTTP::Request & /* theRequest */) const
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
