/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef _ES_STOREHELPER_HXX
#define _ES_STOREHELPER_HXX 1

#include "rutil/compat.hxx"
#include "resip/stack/TransportPersistenceManager.hxx"
#include "resip/stack/SipStack.hxx"

#include <vector>

namespace resip
{
class TransportThread;
}

namespace estacado
{

/** 
   @ingroup resip_config
   @brief Provides a common implementation of the code to add transports and
           domains (from a db-backed netStore) to the stack.
  */
    class EsStoreHelper
    {
    public:
        /** @brief Standardized code for adding transports and domains to stack from db-backed netstore.
        
            This code takes a database-backed resip::TransportPersistenceManager and adds
            each configured resip::TransportPersistenceManager::NetRecord to the stack as a transport.

            @param stack the stack to which transports should be added
            @param netStore the netStore containing resip::TransportPersistenceManager::NetRecord elements that should be added to the stack
            @param loggingCategory allows the caller to place the logging output generated in this method in a given category
         **/
        static std::vector<resip::Transport*> addTransportsAndDomains(resip::SipStack& stack,
                                            resip::TransportPersistenceManager& netStore,
                                            const char* loggingCategory,
                                            bool threadedTransports);

    };

} 
#endif // _ES_STOREHELPER_HXX not defined

/* Copyright 2007 Estacado Systems */
