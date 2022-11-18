
#include <memory>

#include "rutil/Logger.hxx"

#include "repro/Plugin.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

// in an autotools build, this is defined using pkglibdir
#ifndef REPRO_DSO_PLUGIN_DIR_DEFAULT
#define REPRO_DSO_PLUGIN_DIR_DEFAULT ""
#endif

#define REPRO_DSO_PLUGIN_SYMBOL "reproPluginDesc"

using namespace repro;

ReproPluginManager::ReproPluginManager(resip::SipStack& sipStack, ProxyConfig *proxyConfig)
 : PluginManager(*proxyConfig, REPRO_DSO_PLUGIN_DIR_DEFAULT, REPRO_DSO_PLUGIN_SYMBOL, REPRO_DSO_PLUGIN_API_VERSION),
   mSipStack(sipStack),
   mProxyConfig(proxyConfig)
{
}

ReproPluginManager::~ReproPluginManager()
{
}

bool
ReproPluginManager::onPluginLoaded(std::shared_ptr<resip::Plugin> plugin)
{
   std::shared_ptr<repro::Plugin> _plugin = std::dynamic_pointer_cast<repro::Plugin>(plugin);
   if(_plugin)
   {
      return _plugin->init(mSipStack, mProxyConfig);
   }
   ErrLog(<<"cast failed");
   return false;
}

void
ReproPluginManager::onRequestProcessorChainPopulated(ProcessorChain& chain)
{
   for(auto plugin : getPlugins())
   {
      std::shared_ptr<repro::Plugin> _plugin = std::dynamic_pointer_cast<repro::Plugin>(plugin);
      if(_plugin)
      {
         _plugin->onRequestProcessorChainPopulated(chain);
      }
   }
}

void
ReproPluginManager::onResponseProcessorChainPopulated(ProcessorChain& chain)
{
   for(auto plugin : getPlugins())
   {
      std::shared_ptr<repro::Plugin> _plugin = std::dynamic_pointer_cast<repro::Plugin>(plugin);
      if(_plugin)
      {
         _plugin->onResponseProcessorChainPopulated(chain);
      }
   }
}


void
ReproPluginManager::onTargetProcessorChainPopulated(ProcessorChain& chain)
{
   for(auto plugin : getPlugins())
   {
      std::shared_ptr<repro::Plugin> _plugin = std::dynamic_pointer_cast<repro::Plugin>(plugin);
      if(_plugin)
      {
         _plugin->onTargetProcessorChainPopulated(chain);
      }
   }
}

/* ====================================================================

 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2022, Daniel Pocock https://danielpocock.com
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of Plantronics nor the names of its contributors
    may be used to endorse or promote products derived from this
    software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
