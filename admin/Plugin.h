// ======================================================================
/*!
 * \brief SmartMet Admin plugin interface
 */
// ======================================================================

#pragma once
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

class Plugin : public SmartMetPlugin, private SmartMet::Spine::HTTP::Authentication
{
 public:
  Plugin(Spine::Reactor* theReactor, const char* theConfig);
  ~Plugin() override = default;

  Plugin() = delete;
  Plugin(const Plugin& other) = delete;
  Plugin& operator=(const Plugin& other) = delete;
  Plugin(Plugin&& other) = delete;
  Plugin& operator=(Plugin&& other) = delete;

  const std::string& getPluginName() const override;
  int getRequiredAPIVersion() const override;
  bool isAdminQuery(const Spine::HTTP::Request& theRequest) const override;

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

  bool setPause(Spine::Reactor& theReactor,
                const Spine::HTTP::Request& theRequest,
                Spine::HTTP::Response& theResponse);

  bool setContinue(Spine::Reactor& theReactor,
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
