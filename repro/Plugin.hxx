#ifndef PLUGIN_HXX
#define PLUGIN_HXX

#include "rutil/ResipAssert.h"

#include "resip/stack/SipStack.hxx"
#include "repro/ProcessorChain.hxx"
#include "repro/ProxyConfig.hxx"

/* repro plugin DSO support
 *
 * This is a new feature for v1.9.  Developers can create plugins as
 * standalone shared object files and load them at runtime.
 *
 * This makes it easy to create new functionality without recompiling repro
 * itself.  It also makes it easy to distribute third-party plugins as
 * standalone packages.
 *
 * IMPORTANT NOTE:
 * The plugin API is not guaranteed to remain absolutely constant between
 * releases of reSIProcate.  The version check mechanism will simply reject
 * plugins compiled for a newer or older version of the API.
 * In this first release of DSO support, we have provided a very simple
 * set of functions so that if a future version of this API is very
 * different, developers using the current API will not need to change too
 * much to adapt their code.
 *
 * GETTING STARTED
 * See the example plugin in repro/plugins/example
 * It should be fairly easy to copy the Makefile.am and ExamplePlugin.cxx,
 * add your own configure.ac and build your own standalone project outside the
 * reSIProcate source tree.  Naturally, you will need to include the
 * reSIProcate headers on your build path somewhere.
 */

// If the API changes, this number changes too:
#define REPRO_DSO_PLUGIN_API_VERSION 2

namespace repro
{

class Plugin
{
   public:
      Plugin(){};
      virtual ~Plugin(){};

      /* Initialize the plugin, called shortly after loading and
         before repro drops privileges.
         @returns true if successful */
      virtual bool init(resip::SipStack& sipStack, ProxyConfig *proxyConfig) = 0;

      /* called after the request processor chain has been populated */
      virtual void onRequestProcessorChainPopulated(ProcessorChain& chain) = 0;

      /* called after the response processor chain has been populated */
      virtual void onResponseProcessorChainPopulated(ProcessorChain& chain) = 0;

      /* called after the target processor chain has been populated */
      virtual void onTargetProcessorChainPopulated(ProcessorChain& chain) = 0;

      /* called when a reload signal (HUP) is received */
      virtual void onReload() = 0;

};
}

extern "C" {
typedef repro::Plugin* (*PluginCreationFunc)();
typedef struct ReproPluginDescriptor
{
   int mPluginApiVersion;
   PluginCreationFunc creationFunc;
} ReproPluginDescriptor;
};

#endif

/* ====================================================================
 *
 * Copyright 2013 Daniel Pocock.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

