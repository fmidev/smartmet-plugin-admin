// ======================================================================
/*!
 * \brief SmartMet Admin plugin interface
 */
// ======================================================================

#pragma once
#include <boost/utility.hpp>
#include <spine/HTTP.h>
#include <spine/Reactor.h>
#include <spine/SmartMetPlugin.h>
#include <libconfig.h++>
#include <map>
#include <string>
#include <utility>

namespace SmartMet
{
namespace Plugin
{
namespace Admin
{
class PluginImpl;

class Plugin : public SmartMetPlugin, private boost::noncopyable
{
 public:
  Plugin(SmartMet::Spine::Reactor* theReactor, const char* theConfig);
  virtual ~Plugin();

  const std::string& getPluginName() const;
  int getRequiredAPIVersion() const;
  bool queryIsFast(const SmartMet::Spine::HTTP::Request& theRequest) const;

 protected:
  void init();
  void shutdown();
  void requestHandler(SmartMet::Spine::Reactor& theReactor,
                      const SmartMet::Spine::HTTP::Request& theRequest,
                      SmartMet::Spine::HTTP::Response& theResponse);

 private:
  Plugin();

  bool request(SmartMet::Spine::Reactor& theReactor,
               const SmartMet::Spine::HTTP::Request& theRequest,
               SmartMet::Spine::HTTP::Response& theResponse);

  bool requestClusterInfo(SmartMet::Spine::Reactor& theReactor,
                          const SmartMet::Spine::HTTP::Request& theRequest,
                          SmartMet::Spine::HTTP::Response& theResponse);

  bool requestServiceInfo(SmartMet::Spine::Reactor& theReactor,
                          const SmartMet::Spine::HTTP::Request& theRequest,
                          SmartMet::Spine::HTTP::Response& theResponse);

  bool requestBackendInfo(SmartMet::Spine::Reactor& theReactor,
                          const SmartMet::Spine::HTTP::Request& theRequest,
                          SmartMet::Spine::HTTP::Response& theResponse);

  bool requestReload(SmartMet::Spine::Reactor& theReactor,
                     const SmartMet::Spine::HTTP::Request& theRequest,
                     SmartMet::Spine::HTTP::Response& theResponse);

  bool requestGeonames(SmartMet::Spine::Reactor& theReactor,
                       const SmartMet::Spine::HTTP::Request& theRequest,
                       SmartMet::Spine::HTTP::Response& theResponse);

  bool requestQEngineStatus(SmartMet::Spine::Reactor& theReactor,
                            const SmartMet::Spine::HTTP::Request& theRequest,
                            SmartMet::Spine::HTTP::Response& theResponse);

  bool requestServiceStats(SmartMet::Spine::Reactor& theReactor,
                           const SmartMet::Spine::HTTP::Request& theRequest,
                           SmartMet::Spine::HTTP::Response& theResponse);

  bool requestLastRequests(SmartMet::Spine::Reactor& theReactor,
                           const SmartMet::Spine::HTTP::Request& theRequest,
                           SmartMet::Spine::HTTP::Response& theResponse);

  bool requestCacheSizes(SmartMet::Spine::Reactor& theReactor,
                         const SmartMet::Spine::HTTP::Request& theRequest,
                         SmartMet::Spine::HTTP::Response& theResponse);

  bool setLogging(SmartMet::Spine::Reactor& theReactor,
                  const SmartMet::Spine::HTTP::Request& theRequest,
                  SmartMet::Spine::HTTP::Response& theResponse);

  bool getLogging(SmartMet::Spine::Reactor& theReactor,
                  const SmartMet::Spine::HTTP::Request& theRequest,
                  SmartMet::Spine::HTTP::Response& theResponse);

  bool authenticateRequest(const SmartMet::Spine::HTTP::Request& theRequest,
                           SmartMet::Spine::HTTP::Response& theResponse);

  const std::string itsModuleName;

  libconfig::Config itsConfig;

};  // class Plugin

}  // namespace Admin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
