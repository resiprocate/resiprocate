#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef DSO_PLUGINS

// This is the UNIX way of doing DSO, an alternative implementation
// for Windows needs to include the relevant Windows headers here
// and implement the loader code further below
#include <dlfcn.h>

#endif


#include "rutil/Logger.hxx"

#include "rutil/Plugin.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::NONE

using namespace resip;

PluginManager::PluginManager(const ConfigParse& configParse,
      const Data& defaultDirectory,
      const Data& symbolName,
      const int pluginApiVersion)
 : mConfigParse(configParse),
   mDefaultDirectory(defaultDirectory),
   mSymbolName(symbolName),
   mPluginApiVersion(pluginApiVersion)
{
}

PluginManager::~PluginManager()
{
}

bool
PluginManager::loadPlugins(const std::vector<Data>& pluginNames)
{
#ifdef DSO_PLUGINS
   if(pluginNames.empty())
   {
      DebugLog(<<"LoadPlugins not specified, not attempting to load any plugins");
      return true;
   }

   const Data& pluginDirectory = mConfigParse.getConfigData("PluginDirectory", mDefaultDirectory, true);
   if(pluginDirectory.empty())
   {
      ErrLog(<<"LoadPlugins specified but PluginDirectory not specified, can't load plugins");
      return false;
   }
   for(auto pluginName : pluginNames)
   {
      void *dlib;
      // FIXME:
      // - not all platforms use the .so extension
      // - detect and use correct directory separator charactor
      // - do we need to support relative paths here?
      // - should we use the filename prefix 'lib', 'mod' or something else?
      Data name = pluginDirectory + '/' + "lib" + pluginName + ".so";
      dlib = dlopen(name.c_str(), RTLD_NOW | RTLD_GLOBAL);
      if(!dlib)
      {
         ErrLog(<< "Failed to load plugin " << pluginName << " (" << name << "): " << dlerror());
         return false;
      }
      PluginDescriptor* desc = (PluginDescriptor*)dlsym(dlib, mSymbolName.c_str());
      if(!desc)
      {
         ErrLog(<< "Failed to find " << mSymbolName << " in plugin " << pluginName << " (" << name << "): " << dlerror());
         return false;
      }
      if(!(desc->mPluginApiVersion == mPluginApiVersion))
      {
         ErrLog(<< "Failed to load plugin " << pluginName << " (" << name << "): found version " << desc->mPluginApiVersion << ", expecting version " << mPluginApiVersion);
      }
      DebugLog(<<"Trying to instantiate plugin " << pluginName);
      // Instantiate the plugin object and add it to our runtime environment
      std::shared_ptr<resip::Plugin> plugin = desc->creationFunc();
      if(!plugin)
      {
         ErrLog(<< "Failed to instantiate plugin " << pluginName << " (" << name << ")");
         return false;
      }
      onPluginLoaded(plugin);
      mPlugins.push_back(plugin);
   }
   return true;
#else
   if(!pluginNames.empty())
   {
      ErrLog(<<"LoadPlugins specified but repro not compiled with plugin DSO support");
      return false;
   }
   DebugLog(<<"Not compiled with plugin DSO support");
   return true;
#endif

}

void
PluginManager::onReload()
{
   for(auto plugin : mPlugins)
   {
      plugin->onReload();
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
