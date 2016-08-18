/*
 * Copyright (C) 2015 Daniel Pocock http://danielpocock.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/recon/ReconSubsystem.hxx>
#include <rutil/XMLCursor.hxx>

#include <resip/recon/UserAgent.hxx>
#include <resip/recon/ReconSubsystem.hxx>
#include "resip/stack/Pidf.hxx"
#include "resip/stack/GenericPidfContents.hxx"
#include "resip/dum/PublicationHandler.hxx"
#include "resip/dum/ClientPublication.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "rutil/Random.hxx"

#include "MyUserAgent.hxx"

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

tr::MyUserAgent::MyUserAgent(ConversationManager* conversationManager, SharedPtr<UserAgentMasterProfile> profile, tr::Connection& connection, SharedPtr<MyInstantMessage> instantMessage) :
   UserAgent(conversationManager, profile, 0, instantMessage), mConnection(connection)
{
}

void
tr::MyUserAgent::onApplicationTimer(unsigned int id, unsigned int durationMs, unsigned int seq)
{
   InfoLog(<< "onApplicationTimeout: id=" << id << " dur=" << durationMs << " seq=" << seq);
}

void
tr::MyUserAgent::onSubscriptionTerminated(SubscriptionHandle handle, unsigned int statusCode)
{
   InfoLog(<< "onSubscriptionTerminated: handle=" << handle << " statusCode=" << statusCode);
}

std::vector<Pidf::Tuple>
tr::MyUserAgent::getTuplesFromXML(const Data& notifyData)
{
   ParseBuffer pb(notifyData.data(), notifyData.size());
   std::string pidf_namespace;

   XMLCursor xml(pb);
   std::vector<Pidf::Tuple> tuples;
   Uri entity;
   XMLCursor::AttributeMap attr = xml.getAttributes();
   XMLCursor::AttributeMap::const_iterator it =
      std::find_if(attr.begin(), attr.end(), XMLCursor::AttributeValueEqual("urn:ietf:params:xml:ns:pidf"));

   if ( it != attr.end() ) 
   {

      std::string key(it->first.data(), it->first.size());

      size_t pos = key.find(':');

      if ( pos != string::npos) 
      {
         pidf_namespace.assign(key, pos+1, key.size()-pos-1);
         pidf_namespace.append(1, ':');
      }
   }

   const std::string presence = pidf_namespace + "presence";

   if (xml.getTag() == presence.c_str())
   {
      XMLCursor::AttributeMap::const_iterator i = xml.getAttributes().find("entity");
      if (i != xml.getAttributes().end())
      {
         entity = Uri(i->second);
      }
      else
      {
         DebugLog(<< "no entity!");
      }

      if (xml.firstChild())
      {
         do
         {
            const std::string tuple = pidf_namespace + "tuple";
            if (xml.getTag() == tuple.c_str())
            {
	       Pidf::Tuple t;
               t.attributes = xml.getAttributes();
               XMLCursor::AttributeMap::const_iterator i = xml.getAttributes().find("id");
               if (i != xml.getAttributes().end())
               {
                  t.id = i->second;
                  t.attributes.erase("id");
               }

               // look for status, contacts, notes -- take last of each for now
               if (xml.firstChild())
               {
		  const std::string status = pidf_namespace + "status";
		  const std::string contact = pidf_namespace + "contact";
		  const std::string note = pidf_namespace + "note";
		  const std::string timestamp = pidf_namespace + "timestamp";
                  do
                  {
                     if (xml.getTag() == status.c_str())
                     {
                        // look for basic
                        if (xml.firstChild())
                        {
                           do
                           {
                              std::string basic = pidf_namespace + "basic";
                              if (xml.getTag() == basic.c_str())
                              {
                                 if (xml.firstChild())
                                 {
                                    t.status = (xml.getValue() == "open");

				    // If status == closed then the contact is considered Offline
				    if(!t.status)
				    {
				       t.note = "Offline";
				       t.contact = entity.toString();
				       tuples.push_back(t);
				       return tuples;
				    }
				    
                                    xml.parent();
                                 }
                              }
                           } while (xml.nextSibling());
                           xml.parent();
                        }
                     }
                     else if (xml.getTag() == contact.c_str())
                     {
                        XMLCursor::AttributeMap::const_iterator i = xml.getAttributes().find("priority");
                        if (i != xml.getAttributes().end())
                        {
                           t.contactPriority.setValue(i->second);
                        }
                        if (xml.firstChild())
                        {
                           t.contact = xml.getValue();
                           xml.parent();
                        }
                     }
                     else if (xml.getTag() == note.c_str())
                     {
                        if (xml.firstChild())
                        {
                           t.note = xml.getValue();
                           xml.parent();
                        }
                     }
                     else if (xml.getTag() == timestamp.c_str())
                     {
                        if (xml.firstChild())
                        {
                           t.timeStamp = xml.getValue();
                           xml.parent();
                        }
                     }
                  } while (xml.nextSibling());
                  xml.parent();
               }
               
               tuples.push_back(t);
            }
         } while (xml.nextSibling());
         xml.parent();
      }
   }
   else
   {
      DebugLog(<< "no presence tag!");
   }
   
   return tuples;
}

void
tr::MyUserAgent::onSubscriptionNotify(SubscriptionHandle handle, const Data& notifyData)
{
   InfoLog(<< "onSubscriptionNotify: handle=" << handle << " data=" << endl << notifyData);

   std::vector<Pidf::Tuple> tuples = getTuplesFromXML(notifyData);

   // I'm assuming here that the last Tuple will hold the valid information
   QString identifier = QString::fromUtf8(tuples.back().contact.c_str());
   // TODO find a better way to do this...maybe use some resiprocate API
   // removes the string sip: from identifier
   identifier.remove(0,4);
   QString note = QString::fromUtf8(tuples.back().note.c_str());

   // RFC 3863 doesn't seem to have this definitions. Got this from Jitsi and Empathy UI
   QString presence;
   if(note == "Offline")
   {
      presence = "offline";
   }
   else if(note == "Online")
   {
      presence = "available";
   }
   else if(note == "Away")
   {
      presence = "away";
   }
   else if(note == "In a meeting")
   {
      presence = "xa";
   }
   else if(note == "Busy (DND)")
   {
      presence = "dnd";
   }
   else if(note == "Invisible")
   {
      presence = "hidden";
   }
   else
   {
      presence = "unknown";
   }

   emit setContactStatus(identifier, presence);

   // Apparently this is the right way of doing the parse but I'm getting an error when I do this
   // ParseBuffer pb(notifyData.data(), notifyData.size());
   // NameAddr res;
   // res.parse(pb);
}

void
tr::MyUserAgent::onSuccess(ClientRegistrationHandle h, const SipMessage& response)
{
   InfoLog( << "ClientHandler::onSuccess: " << endl );
   QMetaObject::invokeMethod(&mConnection, "onConnected", Qt::QueuedConnection);
}

void
tr::MyUserAgent::onRemoved(ClientRegistrationHandle, const SipMessage& response)
{
   ErrLog ( << "ClientHandler::onRemoved ");
   setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonNetworkError);
}

void
tr::MyUserAgent::onFailure(ClientRegistrationHandle, const SipMessage& response)
{
   ErrLog ( << "ClientHandler::onFailure: " << response );
   setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonNetworkError);
}

int
tr::MyUserAgent::onRequestRetry(ClientRegistrationHandle, int retrySeconds, const SipMessage& response)
{
   WarningLog ( << "ClientHandler:onRequestRetry, want to retry immediately");
   return 0;
}

void
tr::MyUserAgent::thread()
{
   mStopping = false;
   while(!mStopping)
   {
      process(50);
   }
   InfoLog(<<"cleaning up thread");
   UserAgent::shutdown();
   setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonRequested);
   InfoLog(<<"thread done");
}

void
tr::MyUserAgent::stop()
{
   mStopping = true;
}

void
tr::MyUserAgent::setStatus(uint newStatus, uint reason)
{
   QMetaObject::invokeMethod(&mConnection, "setStatusSlot", Qt::QueuedConnection, Q_ARG(uint, newStatus), Q_ARG(uint, reason));
}


