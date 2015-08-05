#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <iostream>

// JSON library includes
#include "cajun/json/reader.h"
#include "cajun/json/writer.h"
#include "cajun/json/elements.h"

#include "repro/AccountingCollector.hxx"
#include "repro/RequestContext.hxx"
#include "repro/ProxyConfig.hxx"
#include "repro/PersistentMessageQueue.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/Logger.hxx"

#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace json;
using namespace std;

const static Data sessionEventQueueName = "sessioneventqueue";
const static Data registrationEventQueueName = "regeventqueue";

AccountingCollector::AccountingCollector(ProxyConfig& config) :
   mDbBaseDir(config.getConfigData("DatabasePath", "./", true)),
   mSessionEventQueue(0),
   mRegistrationEventQueue(0),
   mSessionAccountingAddRoutingHeaders(config.getConfigBool("SessionAccountingAddRoutingHeaders", false)),
   mSessionAccountingAddViaHeaders(config.getConfigBool("SessionAccountingAddViaHeaders", false)),
   mRegistrationAccountingAddRoutingHeaders(config.getConfigBool("RegistrationAccountingAddRoutingHeaders", false)),
   mRegistrationAccountingAddViaHeaders(config.getConfigBool("RegistrationAccountingAddViaHeaders", false)),
   mRegistrationAccountingLogRefreshes(config.getConfigBool("RegistrationAccountingLogRefreshes", false)),
   mFifo(0, 0)  // not limited by time or size
{
   if(config.getConfigBool("SessionAccountingEnabled", false))
   {
      if(!initializeEventQueue(SessionEventType))
      {
         ErrLog(<< "AccountingCollector: cannot initialize session event queue!");
      }
   }
   if(config.getConfigBool("RegistrationAccountingEnabled", false))
   {
      if(!initializeEventQueue(RegistrationEventType))
      {
         ErrLog(<< "AccountingCollector: cannot initialize registration event queue!");
      }
   }

   run();  // Start thread
}

AccountingCollector::~AccountingCollector()
{
   // Shutdown thread and wait for it to complete
   shutdown();
   join();

   delete mSessionEventQueue;
   delete mRegistrationEventQueue;
}

void
AccountingCollector::doRegistrationAccounting(AccountingCollector::RegistrationEvent regevent, const resip::SipMessage& msg)
{
   resip_assert(msg.isRequest());
   resip_assert(msg.method() == REGISTER);

   if(regevent == RegistrationRefreshed && !mRegistrationAccountingLogRefreshes)
   {
      // if mRegistrationAccountingLogRefreshes is false then don't log refreshes
      return;
   }

   DateCategory datetime;
   if(!msg.exists(h_CallId) || !msg.header(h_CallId).isWellFormed())
   {
      ErrLog(<< "AccountingCollector::doRegistrationAccounting: missing proper callId header: " << msg);
      return;
   }
   Object regEvent;
   regEvent["EventId"] = Number(regevent);
   switch(regevent)
   {
   case RegistrationAdded:
      regEvent["EventName"] = String("Registration Added");
      break;
   case RegistrationRefreshed:
      regEvent["EventName"] = String("Registration Refreshed");
      break;
   case RegistrationRemoved:
      regEvent["EventName"] = String("Registration Removed");
      break;
   case RegistrationRemovedAll:
      regEvent["EventName"] = String("Registration Removed All");
      break;
   }
   regEvent["Datetime"] = String(Data::from(datetime).c_str());
   regEvent["CallId"] = String(msg.header(h_CallId).value().c_str());
   if(msg.exists(h_To) && msg.header(h_To).isWellFormed())
   {
      if(!msg.header(h_To).displayName().empty())
      {
         regEvent["User"]["DisplayName"] = String(msg.header(h_To).displayName().c_str());
      }
      regEvent["User"]["Aor"] = String(Data::from(msg.header(h_To).uri().getAorAsUri(msg.getSource().getType())).c_str());
   }
   if(msg.exists(h_From) && msg.header(h_From).isWellFormed())
   {
      if(msg.header(h_From).uri() != msg.header(h_To).uri()) // Only log from is different from To
      {
         if(!msg.header(h_From).displayName().empty())
         {
            regEvent["From"]["DisplayName"] = String(msg.header(h_From).displayName().c_str());
         }
         regEvent["From"]["Uri"] = String(Data::from(msg.header(h_From).uri()).c_str());
      }
   }
   if(msg.exists(h_Contacts) && !msg.header(h_Contacts).empty())
   {
      Array arrayContacts;
      NameAddrs::const_iterator contactIt = msg.header(h_Contacts).begin();
      for(; contactIt != msg.header(h_Contacts).end(); contactIt++)
      {
         if(contactIt->isWellFormed())
         {
            arrayContacts.Insert(String(Data::from(*contactIt).c_str()));
         }
      }
      if(!arrayContacts.Empty())
      {
         regEvent["Contacts"] = arrayContacts;
      }
   }
   if(msg.exists(h_Expires) && msg.header(h_Expires).isWellFormed())
   {
      regEvent["Expires"] = Number(msg.header(h_Expires).value());
   }
   if(mRegistrationAccountingAddViaHeaders &&
      msg.exists(h_Vias) && !msg.header(h_Vias).empty())
   {
      Array arrayVias;
      Vias::const_iterator viaIt = msg.header(h_Vias).begin();
      for(; viaIt != msg.header(h_Vias).end(); viaIt++)
      {
         if(viaIt->isWellFormed())
         {
            arrayVias.Insert(String(Data::from(*viaIt).c_str()));
         }
      }
      if(!arrayVias.Empty())
      {
         regEvent["Vias"] = arrayVias;
      }
   }
   Tuple publicAddress = Helper::getClientPublicAddress(msg);
   if(publicAddress.getType() != UNKNOWN_TRANSPORT)
   {
      regEvent["ClientPublicAddress"]["Transport"] = String(Tuple::toData(publicAddress.getType()).c_str());
      regEvent["ClientPublicAddress"]["IP"] = String(Tuple::inet_ntop(publicAddress).c_str());
      regEvent["ClientPublicAddress"]["Port"] = Number(publicAddress.getPort());
   }
   if(mRegistrationAccountingAddRoutingHeaders &&
      msg.exists(h_Routes) && !msg.header(h_Routes).empty())
   {
      Array arrayRoutes;
      NameAddrs::const_iterator routeIt = msg.header(h_Routes).begin();
      for(; routeIt != msg.header(h_Routes).end(); routeIt++)
      {
         if(routeIt->isWellFormed())
         {
            arrayRoutes.Insert(String(Data::from(*routeIt).c_str()));
         }
      }
      if(!arrayRoutes.Empty())
      {
         regEvent["Routes"] = arrayRoutes;
      }
   }
   if(mRegistrationAccountingAddRoutingHeaders &&
      msg.exists(h_Paths) && !msg.header(h_Paths).empty())
   {
      Array arrayPaths;
      NameAddrs::const_iterator pathIt = msg.header(h_Paths).begin();
      for(; pathIt != msg.header(h_Paths).end(); pathIt++)
      {
         if(pathIt->isWellFormed())
         {
            arrayPaths.Insert(String(Data::from(*pathIt).c_str()));
         }
      }
      if(!arrayPaths.Empty())
      {
         regEvent["Paths"] = arrayPaths;
      }
   }
   if(msg.exists(h_UserAgent) && msg.header(h_UserAgent).isWellFormed())
   {
      regEvent["UserAgent"] = String(msg.header(h_UserAgent).value().c_str());
   }
   pushEventObjectToQueue(regEvent, RegistrationEventType);
}

void
AccountingCollector::doSessionAccounting(const resip::SipMessage& msg, bool received, RequestContext& context)
{
   //DebugLog(<< "AccountingCollector::doSessionAccounting: " << msg.brief());
   if(msg.isRequest())
   {
      if(msg.method() == INVITE && !msg.header(h_To).exists(p_tag))
      {
         // Dialog creating INVITE
         if(received)
         {
            // This came from the wire - so it is new session
            DateCategory datetime;
            if(!msg.exists(h_CallId) || !msg.header(h_CallId).isWellFormed())
            {
               ErrLog(<< "AccountingCollector::doSessionAccounting: missing proper callId header: " << msg);
               return;
            }
            Object sessionEvent;
            sessionEvent["EventId"] = Number(SessionCreated);
            sessionEvent["EventName"] = String("Session Created");
            sessionEvent["Datetime"] = String(Data::from(datetime).c_str());
            sessionEvent["CallId"] = String(msg.header(h_CallId).value().c_str());
            sessionEvent["RequestUri"] = String(Data::from(msg.header(h_RequestLine).uri()).c_str());
            if(msg.exists(h_To) && msg.header(h_To).isWellFormed())
            {
               if(!msg.header(h_To).displayName().empty())
               {
                  sessionEvent["To"]["DisplayName"] = String(msg.header(h_To).displayName().c_str());
               }
               sessionEvent["To"]["Uri"] = String(Data::from(msg.header(h_To).uri()).c_str());
            }
            if(msg.exists(h_From) && msg.header(h_From).isWellFormed())
            {
               if(!msg.header(h_From).displayName().empty())
               {
                  sessionEvent["From"]["DisplayName"] = String(msg.header(h_From).displayName().c_str());
               }
               sessionEvent["From"]["Uri"] = String(Data::from(msg.header(h_From).uri()).c_str());
            }
            if(msg.exists(h_Contacts) && !msg.header(h_Contacts).empty() && msg.header(h_Contacts).front().isWellFormed())
            {
               sessionEvent["Contact"] = String(Data::from(msg.header(h_Contacts).front()).c_str());
            }
            if(mSessionAccountingAddViaHeaders &&
               msg.exists(h_Vias) && !msg.header(h_Vias).empty())
            {
               Array arrayVias;
               Vias::const_iterator viaIt = msg.header(h_Vias).begin();
               for(; viaIt != msg.header(h_Vias).end(); viaIt++)
               {
                  if(viaIt->isWellFormed())
                  {
                     arrayVias.Insert(String(Data::from(*viaIt).c_str()));
                  }
               }
               if(!arrayVias.Empty())
               {
                  sessionEvent["Vias"] = arrayVias;
               }
            }
            Tuple publicAddress = Helper::getClientPublicAddress(msg);
            if(publicAddress.getType() != UNKNOWN_TRANSPORT)
            {
               sessionEvent["ClientPublicAddress"]["Transport"] = String(Tuple::toData(publicAddress.getType()).c_str());
               sessionEvent["ClientPublicAddress"]["IP"] = String(Tuple::inet_ntop(publicAddress).c_str());
               sessionEvent["ClientPublicAddress"]["Port"] = Number(publicAddress.getPort());
            }
            if(mSessionAccountingAddRoutingHeaders &&
               msg.exists(h_Routes) && !msg.header(h_Routes).empty())
            {
               Array arrayRoutes;
               NameAddrs::const_iterator routeIt = msg.header(h_Routes).begin();
               for(; routeIt != msg.header(h_Routes).end(); routeIt++)
               {
                  if(routeIt->isWellFormed())
                  {
                     arrayRoutes.Insert(String(Data::from(*routeIt).c_str()));
                  }
               }
               if(!arrayRoutes.Empty())
               {
                  sessionEvent["Routes"] = arrayRoutes;
               }
            }
            if(mSessionAccountingAddRoutingHeaders &&
               msg.exists(h_RecordRoutes) && !msg.header(h_RecordRoutes).empty())
            {
               Array arrayRecordRoutes;
               NameAddrs::const_iterator recordRouteIt = msg.header(h_RecordRoutes).begin();
               for(; recordRouteIt != msg.header(h_RecordRoutes).end(); recordRouteIt++)
               {
                  if(recordRouteIt->isWellFormed())
                  {
                     arrayRecordRoutes.Insert(String(Data::from(*recordRouteIt).c_str()));
                  }
               }
               if(!arrayRecordRoutes.Empty())
               {
                  sessionEvent["RecordRoutes"] = arrayRecordRoutes;
               }
            }
            if(msg.exists(h_UserAgent) && msg.header(h_UserAgent).isWellFormed())
            {
               sessionEvent["UserAgent"] = String(msg.header(h_UserAgent).value().c_str());
            }
            context.setSessionCreatedEventSent();
            pushEventObjectToQueue(sessionEvent, SessionEventType);
         }
         else
         {
            // This is being forwarded to a target
            DateCategory datetime;
            if(!msg.exists(h_CallId) || !msg.header(h_CallId).isWellFormed())
            {
               ErrLog(<< "AccountingCollector::doSessionAccounting: missing proper callId header: " << msg);
               return;
            }
            Object sessionEvent;
            sessionEvent["EventId"] = Number(SessionRouted);
            sessionEvent["EventName"] = String("Session Routed");
            sessionEvent["Datetime"] = String(Data::from(datetime).c_str());
            sessionEvent["CallId"] = String(msg.header(h_CallId).value().c_str());
            sessionEvent["TargetUri"] = String(Data::from(msg.header(h_RequestLine).uri()).c_str());
            if(mSessionAccountingAddRoutingHeaders &&
               msg.exists(h_Routes) && !msg.header(h_Routes).empty())
            {
               Array arrayRoutes;
               NameAddrs::const_iterator routeIt = msg.header(h_Routes).begin();
               for(; routeIt != msg.header(h_Routes).end(); routeIt++)
               {
                  if(routeIt->isWellFormed())
                  {
                     arrayRoutes.Insert(String(Data::from(*routeIt).c_str()));
                  }
               }
               if(!arrayRoutes.Empty())
               {
                  sessionEvent["Routes"] = arrayRoutes;
               }
            }
            pushEventObjectToQueue(sessionEvent, SessionEventType);
         }
      }
      else if(msg.method() == BYE && received)
      {
         // Session Ended
         DateCategory datetime;
         if(!msg.exists(h_CallId) || !msg.header(h_CallId).isWellFormed())
         {
            ErrLog(<< "AccountingCollector::doSessionAccounting: missing proper callId header: " << msg);
            return;
         }
         Object sessionEvent;
         sessionEvent["EventId"] = Number(SessionEnded);
         sessionEvent["EventName"] = String("Session Ended");
         sessionEvent["Datetime"] = String(Data::from(datetime).c_str());
         sessionEvent["CallId"] = String(msg.header(h_CallId).value().c_str());
         if(msg.exists(h_From) && msg.header(h_From).isWellFormed())
         {
            if(!msg.header(h_From).displayName().empty())
            {
               sessionEvent["From"]["DisplayName"] = String(msg.header(h_From).displayName().c_str());
            }
            sessionEvent["From"]["Uri"] = String(Data::from(msg.header(h_From).uri()).c_str());
         }
         if(msg.exists(h_Reasons) && !msg.header(h_Reasons).empty() && msg.header(h_Reasons).front().isWellFormed())
         {
            // Just look at first occurance
            sessionEvent["Reason"]["Value"] = String(msg.header(h_Reasons).front().value().c_str());
            if(msg.header(h_Reasons).front().exists(p_cause))
            {
               sessionEvent["Reason"]["Cause"] = Number(msg.header(h_Reasons).front().param(p_cause));
            }
            if(msg.header(h_Reasons).front().exists(p_text) && !msg.header(h_Reasons).front().param(p_text).empty())
            {
               sessionEvent["Reason"]["Text"] = String(msg.header(h_Reasons).front().param(p_text).c_str());
            }
         }
         pushEventObjectToQueue(sessionEvent, SessionEventType);
      }
      else if(msg.method() == CANCEL && received)
      {
         // Session Cancelled
         DateCategory datetime;
         if(!msg.exists(h_CallId) || !msg.header(h_CallId).isWellFormed())
         {
            ErrLog(<< "AccountingCollector::doSessionAccounting: missing proper callId header: " << msg);
            return;
         }
         Object sessionEvent;
         sessionEvent["EventId"] = Number(SessionCancelled);
         sessionEvent["EventName"] = String("Session Cancelled");
         sessionEvent["Datetime"] = String(Data::from(datetime).c_str());
         sessionEvent["CallId"] = String(msg.header(h_CallId).value().c_str());
         if(msg.exists(h_Reasons) && !msg.header(h_Reasons).empty() && msg.header(h_Reasons).front().isWellFormed())
         {
            // Just look at first occurance
            sessionEvent["Reason"]["Value"] = String(msg.header(h_Reasons).front().value().c_str());
            if(msg.header(h_Reasons).front().exists(p_cause))
            {
               sessionEvent["Reason"]["Cause"] = Number(msg.header(h_Reasons).front().param(p_cause));
            }
            if(msg.header(h_Reasons).front().exists(p_text) && !msg.header(h_Reasons).front().param(p_text).empty())
            {
               sessionEvent["Reason"]["Text"] = String(msg.header(h_Reasons).front().param(p_text).c_str());
            }
         }
         pushEventObjectToQueue(sessionEvent, SessionEventType);
      }
      else if(msg.method() == REFER && received && msg.header(h_To).exists(p_tag))
      {
         // Only handle mid-dialog REFERs for now
         // Session Redirected
         DateCategory datetime;
         if(!msg.exists(h_CallId) || !msg.header(h_CallId).isWellFormed())
         {
            ErrLog(<< "AccountingCollector::doSessionAccounting: missing proper callId header: " << msg);
            return;
         }
         Object sessionEvent;
         sessionEvent["EventId"] = Number(SessionRedirected);
         sessionEvent["EventName"] = String("Session Redirected");
         sessionEvent["Datetime"] = String(Data::from(datetime).c_str());
         sessionEvent["CallId"] = String(msg.header(h_CallId).value().c_str());
         if(msg.exists(h_From) && msg.header(h_From).isWellFormed())
         {
            if(!msg.header(h_From).displayName().empty())
            {
               sessionEvent["ReferredBy"]["DisplayName"] = String(msg.header(h_From).displayName().c_str());
            }
            sessionEvent["ReferredBy"]["Uri"] = String(Data::from(msg.header(h_From).uri()).c_str());
         }
         if(msg.exists(h_ReferTo) && msg.header(h_ReferTo).isWellFormed())
         {
            sessionEvent["TargetUri"] = String(Data::from(msg.header(h_ReferTo).uri()).c_str());
         }
         pushEventObjectToQueue(sessionEvent, SessionEventType);
      }
   }
   // Response
   else 
   {
      if(!received && msg.method() == INVITE)
      {
         if(msg.header(h_StatusLine).statusCode() >= 200 &&
            msg.header(h_StatusLine).statusCode() < 300)
         {
            // Session Answered
            DateCategory datetime;
            if(!msg.exists(h_CallId) || !msg.header(h_CallId).isWellFormed())
            {
               ErrLog(<< "AccountingCollector::doSessionAccounting: missing proper callId header: " << msg);
               return;
            }
            Object sessionEvent;
            sessionEvent["EventId"] = Number(SessionEstablished);
            sessionEvent["EventName"] = String("Session Established");
            sessionEvent["Datetime"] = String(Data::from(datetime).c_str());
            sessionEvent["CallId"] = String(msg.header(h_CallId).value().c_str());
            if(msg.exists(h_Contacts) && !msg.header(h_Contacts).empty() && msg.header(h_Contacts).front().isWellFormed())
            {
               sessionEvent["Contact"] = String(Data::from(msg.header(h_Contacts).front()).c_str());
            }
            if(mSessionAccountingAddRoutingHeaders &&
               msg.exists(h_RecordRoutes) && !msg.header(h_RecordRoutes).empty())
            {
               Array arrayRecordRoutes;
               NameAddrs::const_iterator recordRouteIt = msg.header(h_RecordRoutes).begin();
               for(; recordRouteIt != msg.header(h_RecordRoutes).end(); recordRouteIt++)
               {
                  if(recordRouteIt->isWellFormed())
                  {
                     arrayRecordRoutes.Insert(String(Data::from(*recordRouteIt).c_str()));
                  }
               }
               if(!arrayRecordRoutes.Empty())
               {
                  sessionEvent["RecordRoutes"] = arrayRecordRoutes;
               }
            }
            if(msg.exists(h_UserAgent) && msg.header(h_UserAgent).isWellFormed())
            {
               sessionEvent["UserAgent"] = String(msg.header(h_UserAgent).value().c_str());
            }
            context.setSessionEstablishedEventSent();
            pushEventObjectToQueue(sessionEvent, SessionEventType);
         }
         else if(msg.header(h_StatusLine).statusCode() >= 300 &&
                 msg.header(h_StatusLine).statusCode() < 400)
         {
            // Session Redirected
            // Session Answered
            DateCategory datetime;
            if(!msg.exists(h_CallId) || !msg.header(h_CallId).isWellFormed())
            {
               ErrLog(<< "AccountingCollector::doSessionAccounting: missing proper callId header: " << msg);
               return;
            }
            Object sessionEvent;
            sessionEvent["EventId"] = Number(SessionRedirected);
            sessionEvent["EventName"] = String("Session Redirected");
            sessionEvent["Datetime"] = String(Data::from(datetime).c_str());
            sessionEvent["CallId"] = String(msg.header(h_CallId).value().c_str());
            if(msg.exists(h_Contacts) && !msg.header(h_Contacts).empty())
            {
               Array arrayContacts;
               NameAddrs::const_iterator contactIt = msg.header(h_Contacts).begin();
               for(; contactIt != msg.header(h_Contacts).end(); contactIt++)
               {
                  if(contactIt->isWellFormed())
                  {
                     arrayContacts.Insert(String(Data::from(*contactIt).c_str()));
                  }
               }
               if(!arrayContacts.Empty())
               {
                  sessionEvent["TargetUris"] = arrayContacts;
               }
            }
            if(msg.exists(h_UserAgent) && msg.header(h_UserAgent).isWellFormed())
            {
               sessionEvent["UserAgent"] = String(msg.header(h_UserAgent).value().c_str());
            }
            pushEventObjectToQueue(sessionEvent, SessionEventType);
         }
         else if(msg.header(h_StatusLine).statusCode() >= 400 &&
                 msg.header(h_StatusLine).statusCode() < 700)
         {
            // Session Error
            Object sessionEvent;
            // Session Answered
            DateCategory datetime;
            if(!msg.exists(h_CallId) || !msg.header(h_CallId).isWellFormed())
            {
               ErrLog(<< "AccountingCollector::doSessionAccounting: missing proper callId header: " << msg);
               return;
            }
            sessionEvent["EventId"] = Number(SessionError);
            sessionEvent["EventName"] = String("Session Error");
            sessionEvent["Datetime"] = String(Data::from(datetime).c_str());
            sessionEvent["CallId"] = String(msg.header(h_CallId).value().c_str());
            sessionEvent["Status"]["Code"] = Number(msg.header(h_StatusLine).statusCode());
            if(!msg.header(h_StatusLine).reason().empty())
            {
               sessionEvent["Status"]["Text"] = String(msg.header(h_StatusLine).reason().c_str());
            }
            if(msg.exists(h_Warnings) && !msg.header(h_Warnings).empty() && msg.header(h_Warnings).front().isWellFormed())
            {
               // Just look at first occurance
               sessionEvent["Warning"]["Code"] = Number(msg.header(h_Warnings).front().code());
               if(!msg.header(h_Warnings).front().text().empty())
               {
                  sessionEvent["Warning"]["Text"] = String(msg.header(h_Warnings).front().text().c_str());
               }
            }
            // Note: a reason header is not usually present on a response - but we will use one if it is
            if(msg.exists(h_Reasons) && !msg.header(h_Reasons).empty() && msg.header(h_Reasons).front().isWellFormed())
            {
               // Just look at first occurance
               sessionEvent["Reason"]["Value"] = String(msg.header(h_Reasons).front().value().c_str());
               if(msg.header(h_Reasons).front().exists(p_cause))
               {
                  sessionEvent["Reason"]["Cause"] = Number(msg.header(h_Reasons).front().param(p_cause));
               }
               if(msg.header(h_Reasons).front().exists(p_text) && !msg.header(h_Reasons).front().param(p_text).empty())
               {
                  sessionEvent["Reason"]["Text"] = String(msg.header(h_Reasons).front().param(p_text).c_str());
               }
            }
            if(msg.exists(h_UserAgent) && msg.header(h_UserAgent).isWellFormed())
            {
               sessionEvent["UserAgent"] = String(msg.header(h_UserAgent).value().c_str());
            }
            pushEventObjectToQueue(sessionEvent, SessionEventType);
         }
      }
   }
}

PersistentMessageEnqueue* 
AccountingCollector::initializeEventQueue(FifoEventType type, bool destroyFirst)
{
   switch(type)
   {
   case SessionEventType:
      if(destroyFirst)
      {
         delete mSessionEventQueue;
         mSessionEventQueue = 0;
      }
      if(!mSessionEventQueue)
      {
         mSessionEventQueue = new PersistentMessageEnqueue(mDbBaseDir);
         if(!mSessionEventQueue->init(true, sessionEventQueueName))
         {
            delete mSessionEventQueue;
            mSessionEventQueue = 0;
         }
      }
      return mSessionEventQueue;
   case RegistrationEventType:
      if(destroyFirst)
      {
         delete mRegistrationEventQueue;
         mRegistrationEventQueue = 0;
      }
      if(!mRegistrationEventQueue)
      {
         mRegistrationEventQueue = new PersistentMessageEnqueue(mDbBaseDir);
         if(!mRegistrationEventQueue->init(true, registrationEventQueueName))
         {
            delete mRegistrationEventQueue;
            mRegistrationEventQueue = 0;
         }
      }
      return mRegistrationEventQueue;
   default:
      resip_assert(false);
      break;
   }
   return 0;
}

void 
AccountingCollector::pushEventObjectToQueue(Object& eventObject, AccountingCollector::FifoEventType type)
{
   FifoEvent* eventData = new FifoEvent;
   eventData->mType = type;
   {
      DataStream ds(eventData->mData);
      Writer::Write(eventObject, ds);
   }

   // Note:  BerkeleyDb calls can block (ie. deaklock after consumer crash), so we use a 
   //        Fifo and thread to ensure we don't block the core proxy processing
   mFifo.add(eventData, TimeLimitFifo<FifoEvent>::InternalElement);
}

void 
AccountingCollector::internalProcess(std::auto_ptr<FifoEvent> eventData)
{
   InfoLog(<< "AccountingCollector::internalProcess: JSON=" << endl << eventData->mData);

   PersistentMessageEnqueue* queue = initializeEventQueue(eventData->mType);

   if(!queue)
   {
      ErrLog(<< "AccountingCollector: cannot initialize PersistentMessageQueue - dropping event!");
      return;
   }

   if(!queue->push(eventData->mData))
   {
      // Error pushing - see if db recovery is needed
      if(queue->isRecoveryNeeded())
      {
         if((queue = initializeEventQueue(eventData->mType, true /* destoryFirst */)) == 0)
         {
            ErrLog(<< "AccountingCollector: cannot initialize PersistentMessageQueue - dropping event!");
            return;
         }
         else
         {
            if(!queue->push(eventData->mData))
            {
               ErrLog(<< "AccountingCollector: error pushing event to queue - dropping event!");
            }
         }
      }
      else
      {
         ErrLog(<< "AccountingCollector: error pushing event to queue - dropping event!");
      }
   }
}

void 
AccountingCollector::thread()
{
   while (!isShutdown() || !mFifo.empty())  // Ensure we drain the queue before shutting down
   {
      try
      {
         std::auto_ptr<FifoEvent> eventData(mFifo.getNext(1000));  // Only need to wake up to see if we are shutdown
         if (eventData.get())
         {
            internalProcess(eventData);
         }
      }
      catch (BaseException& e)
      {
         WarningLog (<< "Unhandled exception: " << e);
      }
   }
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
