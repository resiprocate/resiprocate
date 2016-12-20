#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdexcept>

#include "rutil/hep/ResipHep.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

#include "reflow/FlowManagerSubsystem.hxx"
#include "reflow/HEPRTCPEventLoggingHandler.hxx"

#include "reflow/rtcp/re_rtp.h"

using namespace flowmanager;
using namespace resip;
using namespace reTurn;
using namespace std;

#define RESIPROCATE_SUBSYSTEM FlowManagerSubsystem::FLOWMANAGER

HEPRTCPEventLoggingHandler::HEPRTCPEventLoggingHandler(SharedPtr<HepAgent> agent)
   : mHepAgent(agent)
{
   if(!agent.get())
   {
      ErrLog(<<"agent must not be NULL");
      throw std::runtime_error("agent must not be NULL");
   }
}

HEPRTCPEventLoggingHandler::~HEPRTCPEventLoggingHandler()
{
}

void
HEPRTCPEventLoggingHandler::outboundEvent(resip::SharedPtr<FlowContext> context, const reTurn::StunTuple& source, const reTurn::StunTuple& destination, const resip::Data& event)
{
   sendToHOMER(context, source, destination, event);
}

void
HEPRTCPEventLoggingHandler::inboundEvent(resip::SharedPtr<FlowContext> context, const reTurn::StunTuple& source, const reTurn::StunTuple& destination, const resip::Data& event)
{
   sendToHOMER(context, source, destination, event);
}

int32_t
HEPRTCPEventLoggingHandler::ntoh_cpl(const void *x)
{
   unsigned char c[4];
   uint32_t *v = reinterpret_cast<uint32_t *>(c);

   memcpy(c, x, 4);

   // replace the fraction lost (8 bits) by sign of the 24 bit value
   c[0] = (c[1] & 0x80) ? 0xff : 0;

   return (int32_t)(ntohl(*v));
}

void
HEPRTCPEventLoggingHandler::sendToHOMER(resip::SharedPtr<FlowContext> context, const StunTuple& source, const StunTuple& destination, const Data& event)
{
   GenericIPAddress _source, _destination;

   source.toSockaddr(&_source.address);
   destination.toSockaddr(&_destination.address);

   const struct rtcp_msg* msg = reinterpret_cast<const struct rtcp_msg*>(event.data());

   Data json;
   DataStream stream(json);

   StackLog(<<"RTCP packet type: " << msg->hdr.pt << " len " << (ntohs(msg->hdr.length)*2) << " bytes");
   
   stream << "{";

   switch (msg->hdr.pt)
   {
      case RTCP_SR:
         stream << "\"sender_information\":{"
                << "\"ntp_timestamp_sec\":" << ntohl(msg->r.sr.ntp_sec) << ","
                << "\"ntp_timestamp_usec\":" << ntohl(msg->r.sr.ntp_frac) << ","
                << "\"octets\":" << ntohl(msg->r.sr.osent) << ","
                << "\"rtp_timestamp\":" << ntohl(msg->r.sr.rtp_ts) << ","
                << "\"packets\":" << ntohl(msg->r.sr.psent)
                << "},";
         if(msg->hdr.count > 0)
         {
            const struct rtcp_rr *rr = reinterpret_cast<const struct rtcp_rr*>(&msg->r.sr.rrv);
            stream << "\"ssrc\":" << ntohl(msg->r.sr.ssrc) << ","
                   << "\"type\":" << msg->hdr.pt << ","
                   << "\"report_blocks\":["
                   << "{"
                      << "\"source_ssrc\":" << ntohl(rr->ssrc) << ","
                      << "\"highest_seq_no\":" << ntohl(rr->last_seq) << ","
                      << "\"fraction_lost\":" << +(reinterpret_cast<const uint8_t *>(&rr->fraction_lost_32))[0] << ","  // 8 bits
                      << "\"ia_jitter\":" << ntohl(rr->jitter) << ","
                      << "\"packets_lost\":" << ntoh_cpl(&rr->fraction_lost_32) << ","
                      << "\"lsr\":" << ntohl(rr->lsr) << ","
                      << "\"dlsr\":" << ntohl(rr->dlsr)
                   << "}"
                   << "],\"report_count\":1";
         }
         break;

      case RTCP_RR:
         if(msg->hdr.count > 0)
         {
            const struct rtcp_rr *rr = reinterpret_cast<const struct rtcp_rr*>(&msg->r.rr.rrv);
            stream << "\"ssrc\":" << ntohl(msg->r.rr.ssrc) << ","
                   << "\"type\":" << msg->hdr.pt << ","
                   << "\"report_blocks\":["
                   << "{"
                      << "\"source_ssrc\":" << ntohl(rr->ssrc) << ","
                      << "\"highest_seq_no\":" << ntohl(rr->last_seq) << ","
                      << "\"fraction_lost\":" << +(reinterpret_cast<const uint8_t *>(&rr->fraction_lost_32))[0] << "," // 8 bits
                      << "\"ia_jitter\":" << ntohl(rr->jitter) << ","
                      << "\"packets_lost\":" << ntoh_cpl(&rr->fraction_lost_32) << ","
                      << "\"lsr\":" << ntohl(rr->lsr) << ","
                      << "\"dlsr\":" << ntohl(rr->dlsr)
                   << "}"
                   << "],\"report_count\":1";
         }
         break;

      default:
         DebugLog(<<"unhandled RTCP packet type: " << msg->hdr.pt);
   }

   stream << "}";
   stream.flush();
   StackLog(<<"constructed RTCP JSON: " << json);

   Data correlationId;
   if(context.get())
   {
      correlationId = context->getSipCallId();
   }

   mHepAgent->sendToHOMER<Data>(resip::UDP,
      _source, _destination,
      HepAgent::RTCP_JSON, json,
      correlationId);
}

/* ====================================================================
 *
 * Copyright 2016 Daniel Pocock http://danielpocock.com  All rights reserved.
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
