#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"

#include "repro/AsyncProcessorMessage.hxx"
#include "repro/monkeys/RequestFilter.hxx"
#include "repro/RequestContext.hxx"
#include "repro/FilterStore.hxx"
#include "repro/Proxy.hxx"
#ifdef USE_MYSQL
#include "repro/MySqlDb.hxx"
#endif
#include "resip/stack/SipStack.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

class RequestFilterAsyncMessage : public AsyncProcessorMessage 
{
public:
   RequestFilterAsyncMessage(AsyncProcessor& proc,
                             const resip::Data& tid,
                             TransactionUser* passedtu,
                             Data& query) :
      AsyncProcessorMessage(proc,tid,passedtu),
      mQuery(query)
   {
   }

   RequestFilterAsyncMessage(const RequestFilterAsyncMessage& orig):
      AsyncProcessorMessage(orig)
   {
   }

   virtual Message* clone() const { return new RequestFilterAsyncMessage(*this); }
   virtual EncodeStream& encode(EncodeStream& strm) const { strm << "RequestFilterAsyncMessage(tid="<<mTid<<")"; return strm; }

   Data mQuery;
   int mQueryResult;
   Data mQueryResultData;
};

RequestFilter::RequestFilter(ProxyConfig& config,
                     Dispatcher* asyncDispatcher) :
   AsyncProcessor("RequestFilter", asyncDispatcher),
   mFilterStore(config.getDataStore()->mFilterStore),
   mMySqlDb(0),
   mDefaultNoMatchBehavior(config.getConfigData("RequestFilterDefaultNoMatchBehavior", "")),  // Default: empty string = Continue Processing
   mDefaultDBErrorBehavior(config.getConfigData("RequestFilterDefaultDBErrorBehavior", "500, Server Internal DB Error"))
{
#ifdef USE_MYSQL
   Data mySQLServer = config.getConfigData("RequestFilterMySQLServer", "");
   if(mySQLServer.empty())
   {
      // If RequestFilterMySQLServer setting is blank, then fallback to global
      // MySql settings - if set
      mySQLServer = config.getConfigData("MySQLServer", "");
      if(!mySQLServer.empty())
      {
         // Initialize My SQL using Global settings
         mMySqlDb = new MySqlDb(mySQLServer, 
                       config.getConfigData("MySQLUser", ""), 
                       config.getConfigData("MySQLPassword", ""),
                       config.getConfigData("MySQLDatabaseName", ""),
                       config.getConfigUnsignedLong("MySQLPort", 0),
                       config.getConfigData("MySQLCustomUserAuthQuery", ""));
      }
   }
   else // RequestFilterMySQLServer is set
   {
      // Initialize My SQL settings using RequestFilter specific settings
         mMySqlDb = new MySqlDb(mySQLServer, 
                       config.getConfigData("RequestFilterMySQLUser", ""), 
                       config.getConfigData("RequestFilterMySQLPassword", ""),
                       config.getConfigData("RequestFilterMySQLDatabaseName", ""),
                       config.getConfigUnsignedLong("RequestFilterMySQLPort", 0),
                       config.getConfigData("RequestFilterMySQLCustomUserAuthQuery", ""));
   }
#endif
}

RequestFilter::~RequestFilter()
{
}

short
RequestFilter::parseActionResult(const Data& result, Data& rejectReason)
{
   Data rejectionStatusCode;
   ParseBuffer pb(result);
   const char* anchor = pb.position();
   pb.skipToChar(',');
   pb.data(rejectionStatusCode, anchor);
   if(pb.position()[0] == ',')
   {
      pb.skipChar();
      pb.skipWhitespace();
      anchor = pb.position();
      pb.skipToEnd();
      pb.data(rejectReason, anchor);
   }
   return (short)rejectionStatusCode.convertInt();
}


Processor::processor_action_t
RequestFilter::applyActionResult(RequestContext &rc, const Data& actionResult)
{
   if(!actionResult.empty())
   {
      Data rejectionReason; 
      short rejectionStatusCode = parseActionResult(actionResult, rejectionReason);

      if(rejectionStatusCode >= 400 && rejectionStatusCode < 600)
      {
         // Blocked - reject request
         SipMessage response;
         InfoLog(<<"Request is blocked - responding with a " << rejectionStatusCode << ", customReason=" << rejectionReason);
         Helper::makeResponse(response, rc.getOriginalRequest(), rejectionStatusCode, rejectionReason);
         rc.sendResponse(response);
         return SkipThisChain;
      }
   }

   // Not blocked - continue processing
   DebugLog(<< "Request is accepted");
   return Continue;
}

Processor::processor_action_t
RequestFilter::process(RequestContext &rc)
{
   DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << rc);
   
   Message *message = rc.getCurrentEvent();
   
   RequestFilterAsyncMessage *async = dynamic_cast<RequestFilterAsyncMessage*>(message);

   if (async)
   {
      InfoLog(<< "RequestFilter query complete: queryResult=" << async->mQueryResult << ", resultData=" << async->mQueryResultData);

      if(async->mQueryResult == 0)  // If query was successful, then get query result
      {
         return applyActionResult(rc, async->mQueryResultData);
      }
      else
      {
         return applyActionResult(rc, mDefaultDBErrorBehavior);
      }
   }
   else
   {
      short action;
      Data actionData;
      if(mFilterStore.process(rc.getOriginalRequest(), action, actionData))
      {
         // Match found
         switch(action)
         {
         case FilterStore::Reject:
            return applyActionResult(rc, actionData);
         case FilterStore::SQLQuery:
            if(mMySqlDb)
            {
               // Dispatch async
               std::auto_ptr<ApplicationMessage> async(new RequestFilterAsyncMessage(*this, rc.getTransactionId(), &rc.getProxy(), actionData));
               mAsyncDispatcher->post(async);
               return WaitingForEvent;
            }
            else
            {
               // No DB - use default DB Error behavior
               WarningLog(<< "Request filter with action type SQL Query exists, however there is no MySQL support compiled in, using DefaultDBErrorBehavior");
               return applyActionResult(rc, mDefaultDBErrorBehavior);
            }
         case FilterStore::Accept:
         default:
            DebugLog(<< "Request is accepted");
            return Continue;
         }
      }
      else
      {
         // No Match found - apply default behavior
         return applyActionResult(rc, mDefaultNoMatchBehavior);
      }
   }
}

bool
RequestFilter::asyncProcess(AsyncProcessorMessage* msg)
{
   RequestFilterAsyncMessage* async = dynamic_cast<RequestFilterAsyncMessage*>(msg);
   assert(async);

#ifdef USE_MYSQL
   if(mMySqlDb)
   {
      async->mQueryResult = mMySqlDb->singleResultQuery(async->mQuery, async->mQueryResultData);
      return true;
   }
#else
   assert(false);
   async->mQueryResult = -1;
#endif
   return false;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
