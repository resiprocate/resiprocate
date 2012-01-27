/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#include "resip/stack/EsStoreHelper.hxx"

#include "rutil/Log.hxx"
#include "rutil/EsLogger.hxx"
#include "rutil/DnsUtil.hxx"

#include <vector>

using resip::Transport;

std::vector<Transport*>
estacado::EsStoreHelper::addTransportsAndDomains(resip::SipStack& stack,
                                                 resip::TransportPersistenceManager& netStore,
                                                 const char* loggingCategory,
                                                 bool threadedTransports)
{
    std::vector<Transport*> results;
    std::vector<resip::TransportPersistenceManager::NetRecord> interfaces;
    netStore.getRecords(interfaces);
    std::vector<resip::TransportPersistenceManager::NetRecord>::iterator i;
    
    std::string logger(loggingCategory);
    for(i=interfaces.begin() ; i != interfaces.end(); ++i)
    {
        if(!i->enabled)
        {
            continue;
        }

        if(!resip::DnsUtil::isIpAddress(i->ipAddress))
        {
           ES_ERROR_BYNAME(logger,"Malformed IP-address found in the --interfaces (-i) command-line option: " << i->ipAddress);
           continue;
        }

        if (resip::DnsUtil::isIpV4Address(i->ipAddress))
        {
            ES_DEBUG_BYNAME(logger, "Attempting to open " 
                          << i->protocol << "/" << resip::V4
                          << " port " << i->port 
                          << " On interface: " << i->ipAddress);
            try
            {
                std::list<resip::Data>::iterator dom;
                for(dom=i->domains.begin();dom!=i->domains.end();dom++)
                {
                    stack.addAlias(*dom,i->port);                  
                }
                
                if(i->protocol == resip::TLS)
                {
                    Transport* t=stack.addTransport(i->protocol, i->port, resip::V4, resip::StunEnabled,i->ipAddress,
                                       *(i->domains.begin()),resip::Data::Empty,
                                       resip::SecurityTypes::SSLv23, 0,
                                       threadedTransports);
                    if(t)
                    {
                        results.push_back(t);
                    }
                }
                else
                {
                    Transport* t = stack.addTransport(i->protocol, 
                                                      i->port, 
                                                      resip::V4, 
                                                      resip::StunEnabled,
                                                      i->ipAddress,
                                                      resip::Data::Empty,
                                                      resip::Data::Empty,
                                                      resip::SecurityTypes::SSLv23,
                                                      0,
                                                      threadedTransports);
                    if(t)
                    {
                        results.push_back(t);
                    }
                }
                
                netStore.setActive(i->ipAddress,i->port,i->protocol);
            }
            catch(resip::Transport::Exception& e)
            {
                ES_ERROR_BYNAME(logger, "Could not open " 
                              << i->protocol << "/" << resip::V4
                              << " port " << i->port 
                              << " On interface: " << i->ipAddress
                              << "Exception was: " << e << " Proceeding.");
            }
        }
 #ifdef USE_IPV6
        else
        { 
            ES_DEBUG_BYNAME(logger, "Attempting to open "
                          << i->protocol << "/" << resip::V6 << " port: " 
                          << i->port << " On interface: " << i->ipAddress);
            try
            {
                std::list<resip::Data>::iterator dom;
                for(dom=i->domains.begin();dom!=i->domains.end();dom++)
                {
                    stack.addAlias(*dom,i->port);                  
                }
                
                if(i->protocol == resip::TLS)
                {
                    Transport* t=stack.addTransport(i->protocol, i->port, resip::V6, resip::StunEnabled,i->ipAddress,
                                       *(i->domains.begin()),resip::Data::Empty,
                                       resip::SecurityTypes::SSLv23,
                                       0,
                                       threadedTransports);
                    if(t)
                    {
                        results.push_back(t);
                    }
                }
                else
                {
                    Transport* t = stack.addTransport(i->protocol, 
                                                      i->port, 
                                                      resip::V6, 
                                                      resip::StunEnabled,
                                                      i->ipAddress,
                                                      resip::Data::Empty,
                                                      resip::Data::Empty,
                                                      resip::SecurityTypes::SSLv23,
                                                      0,
                                                      threadedTransports);
                    if(t)
                    {
                        results.push_back(t);
                    }
                }
                
                netStore.setActive(i->ipAddress,i->port,i->protocol);                 
            }
            catch(resip::Transport::Exception& e)
            {
                ES_ERROR_BYNAME(logger,"Could not open "
                              << i->protocol << "/" << resip::V6
                              << " port " << i->port 
                              << " On interface: " << i->ipAddress
                              << " Exception was: " << e << " Proceeding.");
            }
        }
 #endif       
    } // of for
    return results;
} // of addTransportsAndDomains

/* Copyright 2007 Estacado Systems */
