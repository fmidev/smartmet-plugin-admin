// ======================================================================
/*!
 * \brief SmartMet Admin plugin interface
 */
// ======================================================================

#pragma once
#include <boost/utility.hpp>
#include <engines/sputnik/Engine.h>
#include <spine/HTTP.h>
#include <spine/HTTPAuthentication.h>
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

class Plugin : public SmartMetPlugin,
               private SmartMet::Spine::HTTP::Authentication,
               private boost::noncopyable
{
 public:
  Plugin(SmartMet::Spine::Reactor* theReactor, const char* theConfig);
  virtual ~Plugin();

  const std::string& getPluginName() const override;
  int getRequiredAPIVersion() const override;
  bool queryIsFast(const SmartMet::Spine::HTTP::Request& theRequest) const override;

 protected:
  void init() override;
  void shutdown() override;
  void requestHandler(SmartMet::Spine::Reactor& theReactor,
                      const SmartMet::Spine::HTTP::Request& theRequest,
                      SmartMet::Spine::HTTP::Response& theResponse) override;

  bool isAuthenticationRequired(const Spine::HTTP::Request& theRequest) const override;

  std::string getRealm() const override;

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

  bool requestActiveRequests(SmartMet::Spine::Reactor& theReactor,
                             const SmartMet::Spine::HTTP::Request& theRequest,
                             SmartMet::Spine::HTTP::Response& theResponse);

  bool requestCacheSizes(SmartMet::Spine::Reactor& theReactor,
                         const SmartMet::Spine::HTTP::Request& theRequest,
                         SmartMet::Spine::HTTP::Response& theResponse);

  bool requestProducerInfo(SmartMet::Spine::Reactor& theReactor,
                           const SmartMet::Spine::HTTP::Request& theRequest,
                           SmartMet::Spine::HTTP::Response& theResponse);

  bool requestParameterInfo(SmartMet::Spine::Reactor& theReactor,
                            const SmartMet::Spine::HTTP::Request& theRequest,
                            SmartMet::Spine::HTTP::Response& theResponse);

  bool requestObsParameterInfo(SmartMet::Spine::Reactor& theReactor,
                               const SmartMet::Spine::HTTP::Request& theRequest,
                               SmartMet::Spine::HTTP::Response& theResponse);

  bool requestObsProducerInfo(SmartMet::Spine::Reactor& theReactor,
                              const SmartMet::Spine::HTTP::Request& theRequest,
                              SmartMet::Spine::HTTP::Response& theResponse);

  bool requestGridProducerInfo(Spine::Reactor &theReactor,
                               const Spine::HTTP::Request &theRequest,
                               Spine::HTTP::Response &theResponse);

  bool requestGridParameterInfo(SmartMet::Spine::Reactor& theReactor,
                               const SmartMet::Spine::HTTP::Request& theRequest,
                               SmartMet::Spine::HTTP::Response& theResponse);

  bool requestLoadStations(Spine::Reactor& theReactor,
                           const Spine::HTTP::Request& theRequest,
                           Spine::HTTP::Response& theResponse);

  bool requestObsStationInfo(Spine::Reactor &theReactor,
							 const Spine::HTTP::Request &theRequest,
							 Spine::HTTP::Response &theResponse);

  bool setPause(SmartMet::Spine::Reactor& theReactor,
                const SmartMet::Spine::HTTP::Request& theRequest,
                SmartMet::Spine::HTTP::Response& theResponse);

  bool setContinue(SmartMet::Spine::Reactor& theReactor,
                   const SmartMet::Spine::HTTP::Request& theRequest,
                   SmartMet::Spine::HTTP::Response& theResponse);

  bool setLogging(SmartMet::Spine::Reactor& theReactor,
                  const SmartMet::Spine::HTTP::Request& theRequest,
                  SmartMet::Spine::HTTP::Response& theResponse);

  bool getLogging(SmartMet::Spine::Reactor& theReactor,
                  const SmartMet::Spine::HTTP::Request& theRequest,
                  SmartMet::Spine::HTTP::Response& theResponse);

  const std::string itsModuleName;

  libconfig::Config itsConfig;

  Engine::Sputnik::Engine* itsSputnik = nullptr;

};  // class Plugin

}  // namespace Admin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
