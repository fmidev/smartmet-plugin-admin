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
#include <engines/querydata/Engine.h>
#include <engines/sputnik/Engine.h>
#include <macgyver/Base64.h>
#include <macgyver/StringConversion.h>
#include <spine/Convenience.h>
#include <spine/SmartMet.h>
#include <spine/Table.h>
#include <spine/TableFormatterFactory.h>
#include <spine/TableFormatterOptions.h>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>

using namespace std;
using namespace boost::posix_time;
using namespace SmartMet::Spine;

namespace
{
bool isNotOld(const boost::posix_time::ptime &target, const LoggedRequest &compare)
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
string average_and_format(double total_microsecs, unsigned long requests)
{
  try
  {
    // Average global request time
    double average_time = total_microsecs / (1000 * requests);
    string result;
    if (std::isnan(average_time))
    {
      result = "Not available";
    }
    else
    {
      stringstream ss;
      ss << setprecision(4) << average_time;
      result = ss.str();
    }

    return result;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}
}  // namespace

// ----------------------------------------------------------------------
/*!
 * \brief Perform an Admin query
 */
// ----------------------------------------------------------------------
bool Plugin::request(SmartMet::Spine::Reactor &theReactor,
                     const HTTP::Request &theRequest,
                     HTTP::Response &theResponse)
{
  try
  {
    string what = SmartMet::Spine::optional_string(theRequest.getParameter("what"), "");

    if (what.empty())
    {
      string ret("No request specified");
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

    // Unknown request,build response
    // Make MIME header
    std::string mime("text/html; charset=UTF-8");
    theResponse.setHeader("Content-Type", mime.c_str());

    string ret("Unknown admin request: '" + what + "'");
    theResponse.setContent(ret);
    return false;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform a clusterinfo query
 */
// ----------------------------------------------------------------------

bool Plugin::requestClusterInfo(SmartMet::Spine::Reactor &theReactor,
                                const HTTP::Request & /* theRequest */,
                                HTTP::Response &theResponse)
{
  try
  {
    ostringstream out;

    auto engine = theReactor.getSingleton("Sputnik", nullptr);
    if (!engine)
    {
      out << "Sputnik engine is not available" << endl;
      string response = out.str();
      theResponse.setContent(response);
      return false;
    }

    auto *sputnik = reinterpret_cast<SmartMet::Engine::Sputnik::Engine *>(engine);
    sputnik->status(out);

    // Make MIME header
    std::string mime("text/html; charset=UTF-8");
    theResponse.setHeader("Content-Type", mime.c_str());

    // Set content
    string ret = "<html><head><title>SmartMet Admin</title></head><body>";
    ret += out.str();
    ret += "</body></html>";
    theResponse.setContent(ret);

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform a serviceinfo query
 */
// ----------------------------------------------------------------------

bool Plugin::requestServiceInfo(SmartMet::Spine::Reactor &theReactor,
                                const HTTP::Request & /* theRequest */,
                                HTTP::Response &theResponse)
{
  try
  {
    auto handlers = theReactor.getURIMap();

    ostringstream out;
    out << "<h3>Services currently provided by this server</h3>" << std::endl;
    out << "<ol>" << std::endl;
    for (const auto &handler : handlers)
      out << "<li>" << handler.first << "</li>" << std::endl;
    out << "</ol>" << std::endl;

    // Make MIME header
    std::string mime("text/html; charset=UTF-8");
    theResponse.setHeader("Content-Type", mime.c_str());

    // Set content
    string ret = "<html><head><title>SmartMet Admin</title></head><body>";
    ret += out.str();
    ret += "</body></html>";
    theResponse.setContent(ret);

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}
// ----------------------------------------------------------------------
/*!
 * \brief Perform a reload query
 */
// ----------------------------------------------------------------------

bool Plugin::requestReload(SmartMet::Spine::Reactor &theReactor,
                           const HTTP::Request & /* theRequest */,
                           HTTP::Response &theResponse)
{
  try
  {
    ostringstream out;

    auto engine = theReactor.getSingleton("Geonames", nullptr);
    if (!engine)
    {
      out << "Geonames engine is not available" << endl;
      string response = out.str();
      theResponse.setContent(response);
      return false;
    }

    auto *geoengine = reinterpret_cast<SmartMet::Engine::Geonames::Engine *>(engine);
    time_t starttime = time(nullptr);
    if (!geoengine->reload())
    {
      out << "GeoEngine reload failed: " << geoengine->errorMessage() << endl;
      theResponse.setContent(out.str());
      return false;
    }

    time_t endtime = time(nullptr);
    long secs = endtime - starttime;
    out << "GeoEngine reloaded in " << secs << " seconds" << endl;

    // Make MIME header
    std::string mime("text/html; charset=UTF-8");
    theResponse.setHeader("Content-Type", mime);

    // Set content
    string ret = "<html><head><title>SmartMet Admin</title></head><body>";
    ret += out.str();
    ret += "</body></html>";
    theResponse.setContent(ret);

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform a geonames status query
 */
// ----------------------------------------------------------------------

bool Plugin::requestGeonames(SmartMet::Spine::Reactor &theReactor,
                             const HTTP::Request &theRequest,
                             HTTP::Response &theResponse)
{
  try
  {
    ostringstream out;
    string tableFormat =
        SmartMet::Spine::optional_string(theRequest.getParameter("format"), "html");

    string dataType = SmartMet::Spine::optional_string(theRequest.getParameter("type"), "meta");

    std::unique_ptr<TableFormatter> tableFormatter(TableFormatterFactory::create(tableFormat));

    auto engine = theReactor.getSingleton("Geonames", nullptr);
    if (!engine)
    {
      out << "Geonames engine is not available" << endl;
      string response = out.str();
      theResponse.setContent(response);
      return false;
    }

    auto *geoengine = reinterpret_cast<SmartMet::Engine::Geonames::Engine *>(engine);

    SmartMet::Engine::Geonames::StatusReturnType status;

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
      throw SmartMet::Spine::Exception(BCP, "Invalid value for request parameter \"type\".");
    }

    // Make MIME header
    std::string mime(tableFormatter->mimetype() + "; charset=UTF-8");
    theResponse.setHeader("Content-Type", mime);

    tableFormatter->format(out, *status.first, status.second, theRequest, TableFormatterOptions());

    // Set content
    std::string ret = out.str();
    theResponse.setContent(ret);

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform qengine status query
 */
// ----------------------------------------------------------------------

bool Plugin::requestQEngineStatus(SmartMet::Spine::Reactor &theReactor,
                                  const HTTP::Request &theRequest,
                                  HTTP::Response &theResponse)
{
  try
  {
    ostringstream out;

    // Get the Qengine
    auto engine = theReactor.getSingleton("Querydata", nullptr);
    if (!engine)
    {
      out << "Querydata engine not available" << endl;
      string response = out.str();
      theResponse.setContent(response);
      return false;
    }

    auto *qengine = reinterpret_cast<SmartMet::Engine::Querydata::Engine *>(engine);

    // Parse formatting options
    string tableFormat =
        SmartMet::Spine::optional_string(theRequest.getParameter("format"), "debug");
    string projectionFormat =
        SmartMet::Spine::optional_string(theRequest.getParameter("projformat"), "newbase");

    if (tableFormat == "wxml")
    {
      // Wxml not available
      out << "Wxml formatting not supported" << endl;
      string response = out.str();
      theResponse.setContent(response);
      return false;
    }

    string timeFormat =
        SmartMet::Spine::optional_string(theRequest.getParameter("timeformat"), "sql");

    std::pair<boost::shared_ptr<Table>, TableFormatter::Names> statusResult =
        qengine->getEngineContents(timeFormat, projectionFormat);

    std::unique_ptr<TableFormatter> tableFormatter(TableFormatterFactory::create(tableFormat));

    tableFormatter->format(
        out, *statusResult.first, statusResult.second, theRequest, TableFormatterOptions());

    std::string ret;
    if (tableFormat == "html")
    {
      // Only insert tags if using human readable mode
      ret =
          "<html><head>"
          "<title>SmartMet Admin</title>"
          "<style>"
          "table { border: 1px solid black; }"
          "td { border: 1px solid black; text-align:right;}"
          "</style>"
          "</head><body>"
          "<h1>QEngine Memory Mapped Files</h1>";
      ret += out.str();
      ret += "</body></html>";
    }
    else
    {
      ret = out.str();
    }

    // Make MIME header and content
    std::string mime = tableFormatter->mimetype() + "; charset=UTF-8";

    theResponse.setHeader("Content-Type", mime.c_str());
    theResponse.setContent(ret);

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform a backends query
 */
// ----------------------------------------------------------------------

bool Plugin::requestBackendInfo(SmartMet::Spine::Reactor &theReactor,
                                const HTTP::Request &theRequest,
                                HTTP::Response &theResponse)
{
  try
  {
    ostringstream out;

    string service = SmartMet::Spine::optional_string(theRequest.getParameter("service"), "");
    string format = SmartMet::Spine::optional_string(theRequest.getParameter("format"), "debug");

    if (format == "wxml")
    {
      // Wxml not available
      out << "Wxml formatting not supported" << endl;
      string response = out.str();
      theResponse.setContent(response);
      return false;
    }

    auto engine = theReactor.getSingleton("Sputnik", nullptr);
    if (!engine)
    {
      out << "Sputnik engine is not available" << endl;
      string response = out.str();
      theResponse.setContent(response);
      return false;
    }

    auto *sputnik = reinterpret_cast<SmartMet::Engine::Sputnik::Engine *>(engine);

    boost::shared_ptr<Table> table = sputnik->backends(service);

    boost::shared_ptr<TableFormatter> formatter(TableFormatterFactory::create(format));
    TableFormatter::Names names;
    names.push_back("Backend");
    names.push_back("IP");
    names.push_back("Port");

    formatter->format(out, *table, names, theRequest, TableFormatterOptions());

    sputnik->status(out);

    // Make MIME header

    std::string mime = formatter->mimetype() + "; charset=UTF-8";
    theResponse.setHeader("Content-Type", mime);

    // Set content
    string ret;
    if (format == "html")
    {
      // Add html tags only when using human readable format
      ret = "<html><head><title>SmartMet Admin</title></head><body>";
      ret += out.str();
      ret += "</body></html>";
    }
    else
    {
      ret = out.str();
    }

    theResponse.setContent(ret);

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Set server (ContentEngine) logging activity
 */
// ----------------------------------------------------------------------

bool Plugin::setLogging(SmartMet::Spine::Reactor &theReactor,
                        const HTTP::Request &theRequest,
                        HTTP::Response & /* theResponse */)
{
  try
  {
    // First parse if logging status change is requested
    auto loggingFlag = theRequest.getParameter("status");
    if (loggingFlag)
    {
      string flag = *loggingFlag;
      // Logging status change requested
      if (flag == "enable")
      {
        theReactor.setLogging(true);
        return true;
      }
      else if (flag == "disable")
      {
        theReactor.setLogging(false);
        return true;
      }
      else
      {
        throw SmartMet::Spine::Exception(BCP, "Invalid logging parameter value: " + flag);
      }
    }
    else
    {
      throw SmartMet::Spine::Exception(BCP, "Logging parameter value not set.");
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Set server (ContentEngine) logging activity status
 */
// ----------------------------------------------------------------------

bool Plugin::getLogging(SmartMet::Spine::Reactor &theReactor,
                        const HTTP::Request &theRequest,
                        HTTP::Response &theResponse)
{
  try
  {
    ostringstream out;
    Table reqTable;
    string format = SmartMet::Spine::optional_string(theRequest.getParameter("format"), "json");
    std::unique_ptr<TableFormatter> formatter(TableFormatterFactory::create(format));

    bool isCurrentlyLogging = theReactor.getLogging();

    if (isCurrentlyLogging)
    {
      reqTable.set(0, 0, "Enabled");
    }
    else
    {
      reqTable.set(0, 0, "Disabled");
    }

    vector<string> headers = {"LoggingStatus"};
    formatter->format(out, reqTable, headers, theRequest, TableFormatterOptions());

    std::string mime = formatter->mimetype() + "; charset=UTF-8";
    theResponse.setHeader("Content-Type", mime);

    // Set content
    string ret = out.str();
    theResponse.setContent(ret);

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get server (ContentEngine) logged requests (last N minutes)
 */
// ----------------------------------------------------------------------

bool Plugin::requestLastRequests(SmartMet::Spine::Reactor &theReactor,
                                 const HTTP::Request &theRequest,
                                 HTTP::Response &theResponse)
{
  try
  {
    ostringstream out;
    Table reqTable;
    string format = SmartMet::Spine::optional_string(theRequest.getParameter("format"), "json");
    std::unique_ptr<TableFormatter> formatter(TableFormatterFactory::create(format));

    auto givenMinutes = theRequest.getParameter("minutes");
    unsigned int minutes;
    if (!givenMinutes)
    {
      minutes = 1;
    }
    else
    {
      minutes = boost::lexical_cast<unsigned int>(*givenMinutes);
    }

    // Obtain logging information
    auto currentRequests =
        theReactor.getLoggedRequests();  // This is type tuple<bool,RequestMap,posix_time>

    boost::posix_time::ptime firstValidTime =
        boost::posix_time::second_clock::local_time() - boost::posix_time::minutes(minutes);

    std::size_t row = 0;
    for (auto it = std::get<1>(currentRequests).begin(); it != std::get<1>(currentRequests).end();
         ++it)
    {
      auto firstConsidered = std::find_if(
          it->second.begin(), it->second.end(), boost::bind(::isNotOld, firstValidTime, _1));

      for (auto reqIt = firstConsidered; reqIt != it->second.end(); ++reqIt)
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

    vector<string> headers = {"Time", "Duration", "RequestString"};
    formatter->format(out, reqTable, headers, theRequest, TableFormatterOptions());

    // Set MIME
    std::string mime = formatter->mimetype() + "; charset=UTF-8";
    theResponse.setHeader("Content-Type", mime);

    // Set content
    string ret = out.str();
    theResponse.setContent(ret);

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get server active requests
 */
// ----------------------------------------------------------------------

bool Plugin::requestActiveRequests(SmartMet::Spine::Reactor &theReactor,
                                   const HTTP::Request &theRequest,
                                   HTTP::Response &theResponse)
{
  try
  {
    ostringstream out;
    Table reqTable;
    string format = SmartMet::Spine::optional_string(theRequest.getParameter("format"), "json");
    std::unique_ptr<TableFormatter> formatter(TableFormatterFactory::create(format));

    // Obtain logging information
    auto requests = theReactor.getActiveRequests();

    auto now = boost::posix_time::microsec_clock::universal_time();

    std::size_t row = 0;
    for (const auto &id_request : requests)
    {
      const auto id = id_request.first;
      const auto &request = id_request.second;

      auto duration = now - request.time;

      std::size_t column = 0;
      reqTable.set(column++, row, Fmi::to_string(id));
      reqTable.set(column++, row, Fmi::to_iso_extended_string(request.time.time_of_day()));
      reqTable.set(column++, row, Fmi::to_string(duration.total_milliseconds() / 1000.0));
      reqTable.set(column++, row, request.uri);
      ++row;
    }

    vector<string> headers = {"Id", "Time", "Duration", "RequestString"};
    formatter->format(out, reqTable, headers, theRequest, TableFormatterOptions());

    // Set MIME
    std::string mime = formatter->mimetype() + "; charset=UTF-8";
    theResponse.setHeader("Content-Type", mime);

    // Set content
    string ret = out.str();
    theResponse.setContent(ret);

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get server (ContentEngine) logged requests (last N minutes)
 */
// ----------------------------------------------------------------------

bool Plugin::requestCacheSizes(SmartMet::Spine::Reactor &theReactor,
                               const HTTP::Request &theRequest,
                               HTTP::Response &theResponse)
{
  try
  {
    auto engine = theReactor.getSingleton("Contour", nullptr);
    if (!engine)
    {
      theResponse.setContent("Contour engine is not available");
      return false;
    }

    auto *cengine = static_cast<SmartMet::Engine::Contour::Engine *>(engine);

    engine = theReactor.getSingleton("Querydata", nullptr);
    if (!engine)
    {
      theResponse.setContent("Querydata engine is not available");
      return false;
    }

    auto *qengine = static_cast<SmartMet::Engine::Querydata::Engine *>(engine);

    ostringstream out;
    Table reqTable;
    string format = SmartMet::Spine::optional_string(theRequest.getParameter("format"), "json");
    std::unique_ptr<TableFormatter> formatter(TableFormatterFactory::create(format));

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
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform a backends query
 */
// ----------------------------------------------------------------------

bool Plugin::requestServiceStats(SmartMet::Spine::Reactor &theReactor,
                                 const HTTP::Request &theRequest,
                                 HTTP::Response &theResponse)
{
  try
  {
    static auto &Headers =
        *new vector<string>{"Handler", "LastMinute", "LastHour", "Last24Hours", "AverageDuration"};

    ostringstream out;

    stringstream formatstream;

    Table statsTable;

    string format = SmartMet::Spine::optional_string(theRequest.getParameter("format"), "json");

    std::unique_ptr<TableFormatter> formatter(TableFormatterFactory::create(format));

    // Use system locale for numbers formatting
    locale system_locale("");
    formatstream.imbue(system_locale);

    auto currentRequests =
        theReactor.getLoggedRequests();  // This is type tuple<bool,RequestMap,posix_time>

    auto currentTime = boost::posix_time::microsec_clock::local_time();

    std::size_t row = 0;
    unsigned long total_minute = 0;
    unsigned long total_hour = 0;
    unsigned long total_day = 0;
    long global_microsecs = 0;

    for (auto requestPair = std::get<1>(currentRequests).begin();
         requestPair != std::get<1>(currentRequests).end();
         ++requestPair)
    {
      // Lets calculate how many hits we have in minute,hour and day and since start
      unsigned long inMinute = 0;
      unsigned long inHour = 0;
      unsigned long inDay = 0;
      long total_microsecs = 0;
      // We go from newest to oldest
      for (auto listIter = requestPair->second.rbegin(); listIter != requestPair->second.rend();
           ++listIter)
      {
        auto sinceDuration = currentTime - listIter->getRequestEndTime();
        auto accessDuration = listIter->getAccessDuration();

        total_microsecs += accessDuration.total_microseconds();

        global_microsecs += accessDuration.total_microseconds();

        if (sinceDuration < boost::posix_time::minutes(1))
        {
          ++inMinute;
          ++total_minute;
        }
        if (sinceDuration < boost::posix_time::hours(1))
        {
          ++inHour;
          ++total_hour;
        }
        if (sinceDuration < boost::posix_time::hours(24))
        {
          ++inDay;
          ++total_day;
        }
      }

      std::size_t column = 0;

      statsTable.set(column, row, requestPair->first);
      ++column;

      formatstream << inMinute;
      statsTable.set(column, row, formatstream.str());
      ++column;
      formatstream.str("");

      formatstream << inHour;
      statsTable.set(column, row, formatstream.str());
      ++column;
      formatstream.str("");

      formatstream << inDay;
      statsTable.set(column, row, formatstream.str());
      ++column;
      formatstream.str("");

      string msecs = average_and_format(total_microsecs, inDay);
      statsTable.set(column, row, msecs);

      ++row;
    }

    // Finally insert totals
    std::size_t column = 0;

    statsTable.set(column, row, "Total requests");
    ++column;

    formatstream << total_minute;
    statsTable.set(column, row, formatstream.str());
    ++column;
    formatstream.str("");

    formatstream << total_hour;
    statsTable.set(column, row, formatstream.str());
    ++column;
    formatstream.str("");

    formatstream << total_day;
    statsTable.set(column, row, formatstream.str());
    ++column;
    formatstream.str("");

    string msecs = average_and_format(global_microsecs, total_day);
    statsTable.set(column, row, msecs);

    formatter->format(out, statsTable, Headers, theRequest, TableFormatterOptions());

    // Set MIME
    std::string mime = formatter->mimetype() + "; charset=UTF-8";
    theResponse.setHeader("Content-Type", mime);

    // Set content
    string ret = out.str();
    theResponse.setContent(ret);

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Main content handler
 */
// ----------------------------------------------------------------------

void Plugin::requestHandler(SmartMet::Spine::Reactor &theReactor,
                            const HTTP::Request &theRequest,
                            HTTP::Response &theResponse)
{
  try
  {
    // Check authentication first
    bool hasValidAuthentication = authenticateRequest(theRequest, theResponse);

    if (!hasValidAuthentication)
    {
      // Auhentication failure
      return;
    }

    try
    {
      // We return JSON, hence we should enable CORS
      theResponse.setHeader("Access-Control-Allow-Origin", "*");

      const int expires_seconds = 1;
      ptime t_now = second_clock::universal_time();

      bool response = request(theReactor, theRequest, theResponse);

      if (response)
      {
        theResponse.setStatus(HTTP::Status::ok);
      }
      else
      {
        theResponse.setStatus(HTTP::Status::not_implemented);
      }

      // Adding response headers

      ptime t_expires = t_now + seconds(expires_seconds);
      boost::shared_ptr<Fmi::TimeFormatter> tformat(Fmi::TimeFormatter::create("http"));
      std::string cachecontrol =
          "public, max-age=" + boost::lexical_cast<std::string>(expires_seconds);
      std::string expiration = tformat->format(t_expires);
      std::string modification = tformat->format(t_now);

      theResponse.setHeader("Cache-Control", cachecontrol.c_str());
      theResponse.setHeader("Expires", expiration.c_str());
      theResponse.setHeader("Last-Modified", modification.c_str());

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

      SmartMet::Spine::Exception exception(BCP, "Request processing exception!", nullptr);
      exception.addParameter("URI", theRequest.getURI());
      exception.printError();

      theResponse.setStatus(HTTP::Status::bad_request);

      // Adding the first exception information into the response header

      std::string firstMessage = exception.what();
      boost::algorithm::replace_all(firstMessage, "\n", " ");
      firstMessage = firstMessage.substr(0, 300);
      theResponse.setHeader("X-Admin-Error", firstMessage.c_str());
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Plugin constructor
 */
// ----------------------------------------------------------------------

Plugin::Plugin(SmartMet::Spine::Reactor *theReactor, const char *theConfig)
    : SmartMetPlugin(), itsModuleName("Admin")
{
  try
  {
    if (theReactor->getRequiredAPIVersion() != SMARTMET_API_VERSION)
      throw SmartMet::Spine::Exception(BCP, "Admin plugin and Server API version mismatch");

    // Register the handler
    if (!theReactor->addContentHandler(
            this, "/admin", boost::bind(&Plugin::callRequestHandler, this, _1, _2, _3)))
      throw SmartMet::Spine::Exception(BCP, "Failed to register admin content handler");

    itsConfig.readFile(theConfig);

    // Password must be specified
    if (!itsConfig.exists("password"))
      throw SmartMet::Spine::Exception(BCP, "Password not specified in the config file");

    // User must be specified
    if (!itsConfig.exists("user"))
      throw SmartMet::Spine::Exception(BCP, "User not specified in the config file");
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
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

bool isAuthenticationRequired(const HTTP::Request &theRequest)
{
  try
  {
    string what = SmartMet::Spine::optional_string(theRequest.getParameter("what"), "");

    if (what == "reload")
      return true;

    return false;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
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
 * \brief Authenticates the request
 */
// ----------------------------------------------------------------------

bool Plugin::authenticateRequest(const HTTP::Request &theRequest, HTTP::Response &theResponse)
{
  try
  {
    auto credentials = theRequest.getHeader("Authorization");

    if (!credentials)
    {
      // Does not have authentication, lets ask for it if necessary
      if (!isAuthenticationRequired(theRequest))
        return true;

      theResponse.setStatus(HTTP::Status::unauthorized);
      theResponse.setHeader("WWW-Authenticate", "Basic realm=\"SmartMet Admin\"");
      theResponse.setHeader("Content-Type", "text/html; charset=UTF-8");

      string content = "<html><body><h1>401 Unauthorized </h1></body></html>";
      theResponse.setContent(content);

      return false;
    }

    else
    {
      // Parse user and password

      vector<string> splitHeader;
      string truePassword, trueUser, trueDigest, givenDigest;

      boost::algorithm::split(
          splitHeader, *credentials, boost::is_any_of(" "), boost::token_compress_on);

      givenDigest = splitHeader[1];  // Second field in the header: ( Basic aHR0cHdhdGNoOmY= )

      itsConfig.lookupValue("user", trueUser);          // user exists in config at this point
      itsConfig.lookupValue("password", truePassword);  // password exists in config at this point
      trueDigest = Fmi::Base64::encode(trueUser + ":" + truePassword);

      // Passwords match
      if (trueDigest == givenDigest)
      {
        // Main handler can proceed
        return true;
      }

      // Wrong password, ask it again
      else
      {
        theResponse.setStatus(HTTP::Status::unauthorized);
        theResponse.setHeader("WWW-Authenticate", "Basic realm=\"SmartMet Admin\"");
        theResponse.setHeader("Content-Type", "text/html; charset=UTF-8");

        string content = "<html><body><h1>401 Unauthorized </h1></body></html>";
        theResponse.setContent(content);
        return false;
      }
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

Plugin::~Plugin() {}
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

bool Plugin::queryIsFast(const SmartMet::Spine::HTTP::Request & /* theRequest */) const
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
  delete us;
}

// ======================================================================
