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
  Plugin(Spine::Reactor* theReactor, const char* theConfig);
  Plugin() = delete;
  ~Plugin() override;

  const std::string& getPluginName() const override;
  int getRequiredAPIVersion() const override;
  bool queryIsFast(const Spine::HTTP::Request& theRequest) const override;

 protected:
  void init() override;
  void shutdown() override;
  void requestHandler(Spine::Reactor& theReactor,
                      const Spine::HTTP::Request& theRequest,
                      Spine::HTTP::Response& theResponse) override;

  bool isAuthenticationRequired(const Spine::HTTP::Request& theRequest) const override;

  std::string getRealm() const override;

 private:
  bool request(Spine::Reactor& theReactor,
               const Spine::HTTP::Request& theRequest,
               Spine::HTTP::Response& theResponse);

  bool requestClusterInfo(Spine::Reactor& theReactor,
                          const Spine::HTTP::Request& theRequest,
                          Spine::HTTP::Response& theResponse);

  bool requestBackendInfo(Spine::Reactor& theReactor,
                          const Spine::HTTP::Request& theRequest,
                          Spine::HTTP::Response& theResponse);

  static bool requestServiceInfo(Spine::Reactor& theReactor,
                                 const Spine::HTTP::Request& theRequest,
                                 Spine::HTTP::Response& theResponse);

  static bool requestReload(Spine::Reactor& theReactor,
                            const Spine::HTTP::Request& theRequest,
                            Spine::HTTP::Response& theResponse);

  static bool requestGeonames(Spine::Reactor& theReactor,
                              const Spine::HTTP::Request& theRequest,
                              Spine::HTTP::Response& theResponse);

  static bool requestQEngineStatus(Spine::Reactor& theReactor,
                                   const Spine::HTTP::Request& theRequest,
                                   Spine::HTTP::Response& theResponse);

  static bool requestServiceStats(Spine::Reactor& theReactor,
                                  const Spine::HTTP::Request& theRequest,
                                  Spine::HTTP::Response& theResponse);

  static bool requestLastRequests(Spine::Reactor& theReactor,
                                  const Spine::HTTP::Request& theRequest,
                                  Spine::HTTP::Response& theResponse);

  static bool requestActiveRequests(Spine::Reactor& theReactor,
                                    const Spine::HTTP::Request& theRequest,
                                    Spine::HTTP::Response& theResponse);

  static bool requestCacheSizes(Spine::Reactor& theReactor,
                                const Spine::HTTP::Request& theRequest,
                                Spine::HTTP::Response& theResponse);

  static bool requestProducerInfo(Spine::Reactor& theReactor,
                                  const Spine::HTTP::Request& theRequest,
                                  Spine::HTTP::Response& theResponse);

  static bool requestParameterInfo(Spine::Reactor& theReactor,
                                   const Spine::HTTP::Request& theRequest,
                                   Spine::HTTP::Response& theResponse);

  static bool requestObsParameterInfo(Spine::Reactor& theReactor,
                                      const Spine::HTTP::Request& theRequest,
                                      Spine::HTTP::Response& theResponse);

  static bool requestObsProducerInfo(Spine::Reactor& theReactor,
                                     const Spine::HTTP::Request& theRequest,
                                     Spine::HTTP::Response& theResponse);

  static bool requestGridProducerInfo(Spine::Reactor& theReactor,
                                      const Spine::HTTP::Request& theRequest,
                                      Spine::HTTP::Response& theResponse);

  static bool requestGridParameterInfo(Spine::Reactor& theReactor,
                                       const Spine::HTTP::Request& theRequest,
                                       Spine::HTTP::Response& theResponse);

  static bool requestLoadStations(Spine::Reactor& theReactor,
                                  const Spine::HTTP::Request& theRequest,
                                  Spine::HTTP::Response& theResponse);

  static bool requestObsStationInfo(Spine::Reactor& theReactor,
                                    const Spine::HTTP::Request& theRequest,
                                    Spine::HTTP::Response& theResponse);

  static bool listRequests(Spine::Reactor& theReactor,
                           const Spine::HTTP::Request& theRequest,
                           Spine::HTTP::Response& theResponse);

  bool setPause(Spine::Reactor& theReactor,
                const Spine::HTTP::Request& theRequest,
                Spine::HTTP::Response& theResponse);

  bool setContinue(Spine::Reactor& theReactor,
                   const Spine::HTTP::Request& theRequest,
                   Spine::HTTP::Response& theResponse);

  bool setLogging(Spine::Reactor& theReactor,
                  const Spine::HTTP::Request& theRequest,
                  Spine::HTTP::Response& theResponse);

  bool getLogging(Spine::Reactor& theReactor,
                  const Spine::HTTP::Request& theRequest,
                  Spine::HTTP::Response& theResponse);

  const std::string itsModuleName;

  libconfig::Config itsConfig;

  Engine::Sputnik::Engine* itsSputnik = nullptr;

};  // class Plugin

}  // namespace Admin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
