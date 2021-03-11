// ======================================================================
/*!
 * \brief SmartMet Admin plugin implementation
 */
// ======================================================================

#include "Plugin.h"

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/locale.hpp>
#include <engines/contour/Engine.h>
#include <engines/geonames/Engine.h>
#include <engines/observation/Engine.h>
#include <engines/querydata/Engine.h>
#include <engines/grid/Engine.h>
#include <macgyver/Base64.h>
#include <macgyver/StringConversion.h>
#include <macgyver/TimeParser.h>
#include <spine/Convenience.h>
#include <spine/FmiApiKey.h>
#include <spine/SmartMet.h>
#include <spine/Table.h>
#include <spine/TableFormatterFactory.h>
#include <spine/TableFormatterOptions.h>

#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace
{
bool isNotOld(const boost::posix_time::ptime &target, const SmartMet::Spine::LoggedRequest &compare)
{
  return compare.getRequestEndTime() > target;
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
    if (what == "clusterinfo")
      return requestClusterInfo(theReactor, theRequest, theResponse);
    if (what == "serviceinfo")
      return requestServiceInfo(theReactor, theRequest, theResponse);
    if (what == "reload")
      return requestReload(theReactor, theRequest, theResponse);
    if (what == "geonames")
      return requestGeonames(theReactor, theRequest, theResponse);
    if (what == "qengine")
      return requestQEngineStatus(theReactor, theRequest, theResponse);
    if (what == "backends")
      return requestBackendInfo(theReactor, theRequest, theResponse);
    if (what == "servicestats")
      return requestServiceStats(theReactor, theRequest, theResponse);
    if (what == "producers")
      return requestProducerInfo(theReactor, theRequest, theResponse);
    if (what == "parameters")
      return requestParameterInfo(theReactor, theRequest, theResponse);
    if (what == "obsparameters")
      return requestObsParameterInfo(theReactor, theRequest, theResponse);
    if (what == "obsproducers")
      return requestObsProducerInfo(theReactor, theRequest, theResponse);
    if (what == "gridproducers")
      return requestGridProducerInfo(theReactor, theRequest, theResponse);
    if (what == "gridparameters")
      return requestGridParameterInfo(theReactor, theRequest, theResponse);
    if (what == "setlogging")
      return setLogging(theReactor, theRequest, theResponse);
    if (what == "getlogging")
      return getLogging(theReactor, theRequest, theResponse);
    if (what == "lastrequests")
      return requestLastRequests(theReactor, theRequest, theResponse);
    if (what == "cachesizes")
      return requestCacheSizes(theReactor, theRequest, theResponse);
    if (what == "activerequests")
      return requestActiveRequests(theReactor, theRequest, theResponse);
    if (what == "pause")
      return setPause(theReactor, theRequest, theResponse);
    if (what == "continue")
      return setContinue(theReactor, theRequest, theResponse);
    if (what == "reloadstations")
      return requestLoadStations(theReactor, theRequest, theResponse);

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
    std::unique_ptr<Spine::TableFormatter> tableFormatter(Spine::TableFormatterFactory::create(tableFormat));
    std::pair<boost::shared_ptr<Spine::Table>, Spine::TableFormatter::Names> producerInfo = gridEngine->getProducerInfo(producer);
    auto grid_out_producer = tableFormatter->format(*producerInfo.first,producerInfo.second,theRequest,Spine::TableFormatterOptions());

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
        gridEngineParameterInfo.second,theRequest,Spine::TableFormatterOptions());

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

    boost::shared_ptr<Spine::Table> table = itsSputnik->backends(service);

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
 * \brief Set server (ContentEngine) logging activity
 */
// ----------------------------------------------------------------------

bool Plugin::setLogging(Spine::Reactor &theReactor,
                        const Spine::HTTP::Request &theRequest,
                        Spine::HTTP::Response & /* theResponse */)
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

bool Plugin::getLogging(Spine::Reactor &theReactor,
                        const Spine::HTTP::Request &theRequest,
                        Spine::HTTP::Response &theResponse)
{
  try
  {
    Spine::Table reqTable;
    std::string format = Spine::optional_string(theRequest.getParameter("format"), "json");
    std::unique_ptr<Spine::TableFormatter> formatter(Spine::TableFormatterFactory::create(format));

    bool isCurrentlyLogging = theReactor.getLogging();

    if (isCurrentlyLogging)
      reqTable.set(0, 0, "Enabled");
    else
      reqTable.set(0, 0, "Disabled");

    std::vector<std::string> headers = {"LoggingStatus"};
    auto out = formatter->format(reqTable, headers, theRequest, Spine::TableFormatterOptions());

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
 * \brief Get server (ContentEngine) logged requests (last N minutes)
 */
// ----------------------------------------------------------------------

bool Plugin::requestLastRequests(Spine::Reactor &theReactor,
                                 const Spine::HTTP::Request &theRequest,
                                 Spine::HTTP::Response &theResponse)
{
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
    auto currentRequests =
        theReactor.getLoggedRequests();  // This is type tuple<bool,LogRange,posix_time>

    auto firstValidTime =
        boost::posix_time::second_clock::local_time() - boost::posix_time::minutes(minutes);

    std::size_t row = 0;
    for (const auto &req : std::get<1>(currentRequests))
    {
      auto firstConsidered = std::find_if(
          req.second.begin(), req.second.end(), boost::bind(::isNotOld, firstValidTime, _1));

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

// ----------------------------------------------------------------------
/*!
 * \brief Get server (ContentEngine) logged requests (last N minutes)
 */
// ----------------------------------------------------------------------

bool Plugin::requestCacheSizes(Spine::Reactor &theReactor,
                               const Spine::HTTP::Request &theRequest,
                               Spine::HTTP::Response &theResponse)
{
  try
  {
    auto *engine = theReactor.getSingleton("Contour", nullptr);
    if (engine == nullptr)
    {
      theResponse.setContent("Contour engine is not available");
      return false;
    }

    auto *cengine = static_cast<Engine::Contour::Engine *>(engine);

    engine = theReactor.getSingleton("Querydata", nullptr);
    if (engine == nullptr)
    {
      theResponse.setContent("Querydata engine is not available");
      return false;
    }

    auto *qengine = static_cast<Engine::Querydata::Engine *>(engine);

    auto q_cache = qengine->getCacheSizes();
    auto c_cache = cengine->getCacheSizes();

    std::string ret =
        "{\"contour_cache_max_size\": " + Fmi::to_string(c_cache.contour_cache_max_size) + ",\n" +
        "\"contour_cache_size\": " + Fmi::to_string(c_cache.contour_cache_size) + ",\n" +
        "\"coordinate_cache_max_size\": " + Fmi::to_string(q_cache.coordinate_cache_max_size) +
        ",\n" + "\"coordinate_cache_size\": " + Fmi::to_string(q_cache.coordinate_cache_size) +
        ",\n" + "\"values_cache_max_size\": " + Fmi::to_string(q_cache.values_cache_max_size) +
        ",\n" + "\"values_cache_size\": " + Fmi::to_string(q_cache.values_cache_size) + "}";

    theResponse.setContent(ret);
    theResponse.setHeader("Content-Type", "application/json; charset=UTF-8");
    theResponse.setHeader("Access-Control-Allow-Origin", "*");

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

    auto currentRequests =
        theReactor.getLoggedRequests();  // This is type tuple<bool,LogRange,posix_time>

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
            this, "/admin", boost::bind(&Plugin::callRequestHandler, this, _1, _2, _3)))
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
 * \brief Destructor
 */
// ----------------------------------------------------------------------

Plugin::~Plugin() = default;

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
