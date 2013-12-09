
#include "rutil/Logger.hxx"
#include "repro/Plugin.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;

class ExamplePlugin : public Plugin
{
   public:
      ExamplePlugin(){};
      ~ExamplePlugin(){};

      virtual bool init(ProxyConfig *proxyConfig)
      {
          DebugLog(<<"ExamplePlugin: init called");
          return true;
      }

      virtual void onRequestProcessorChainPopulated(ProcessorChain& chain)
      {
         DebugLog(<<"ExamplePlugin: onRequestProcessorChainPopulated called");
      }

      virtual void onResponseProcessorChainPopulated(ProcessorChain& chain)
      {
         DebugLog(<<"ExamplePlugin: onResponseProcessorChainPopulated called");
      }

      virtual void onTargetProcessorChainPopulated(ProcessorChain& chain)
      {
         DebugLog(<<"ExamplePlugin: onTargetProcessorChainPopulated called");
      }
};


extern "C" {

static
Plugin* instantiate()
{
   return new ExamplePlugin();
}

ReproPluginDescriptor reproPluginDesc =
{
   REPRO_DSO_PLUGIN_API_VERSION,
   &instantiate
};

};

