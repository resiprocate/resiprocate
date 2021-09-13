#include "GStreamerUtils.hxx"

#ifdef BUILD_GSTREAMER

#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"

using namespace resip;

extern "C"
{

Log::Level
gst_debug_level_to_severity_level (GstDebugLevel level)
{
   switch (level) {
      case GST_LEVEL_ERROR:   return Log::Err;
      case GST_LEVEL_WARNING: return Log::Warning;
      case GST_LEVEL_FIXME:   return Log::Info;
      case GST_LEVEL_INFO:    return Log::Info;
      case GST_LEVEL_DEBUG:   return Log::Debug;
      case GST_LEVEL_LOG:     return Log::Stack;
      case GST_LEVEL_TRACE:   return Log::Stack;
      default:                return Log::None;
   }
}

void
gst2resip_log_function(GstDebugCategory *category, GstDebugLevel level,
                   const gchar *file,
                   const gchar *function, gint line, GObject *object,
                   GstDebugMessage *message, gpointer user_data)
{
   if (level > gst_debug_category_get_threshold (category) ) {
      return;
   }

   Log::Level level_ = gst_debug_level_to_severity_level (level);

   if (level_ == Log::None) {
      return;
   }

   Subsystem& system_ = Subsystem::APP;
   do
   {
      if (genericLogCheckLevel(level_, system_))
      {
         resip::Log::Guard _resip_log_guard(level_, system_, file, line, function);
         _resip_log_guard.asStream() << "[" << category->name << "]: ";
         // FIXME - include the GObject *object with debug_object (object)
         _resip_log_guard.asStream() << gst_debug_message_get (message);
      }
   }
   while(false);
}

} // extern "C"
#endif

/* ====================================================================
 *
 * Copyright 2021 Daniel Pocock https://danielpocock.com  All rights reserved.
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
