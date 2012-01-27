/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef TRANSPORT_PERSISTENCE_MANAGER_HXX
#define TRANSPORT_PERSISTENCE_MANAGER_HXX 1

#include "rutil/Mutex.hxx"
#include "rutil/Data.hxx"
#include "rutil/TransportType.hxx"
#include <list>
#include <set>
#include <vector>

namespace resip
{

/** @brief Class encapsulating transport information.
 **/
class TransportPersistenceManager
{
   public:
      TransportPersistenceManager(){};
      virtual ~TransportPersistenceManager(){};
      
      /** @brief The persisted record that TransportPersistenceManager uses. 
      
          The key for net records is made up of 3 elements: ip address, port, and
          protocol.
       **/
      class NetRecord
      {
         public:
            //These make up the key
            resip::Data ipAddress; ///< IP address of the transport
            unsigned short port; ///< port number of the transport
            resip::TransportType protocol; ///< protocol of the transport
            
            std::list<resip::Data> domains; ///< list of domains corresponding to this transport
            bool enabled; ///< is the transport enabled
            /** @brief is record-route enabled.

                @note doesn't apply to anything other than proxies, but 
                shouldn't be harmful.
             **/
            bool rrEnabled;
            /** @brief is the transport active

                This isn't configuration really, but it is useful state.
				@todo !bwc! TODO
             **/
            bool active;

      };
      
      /** @brief Returns a NetRecord for the given values that make up the key.
      
          Success or failure of the call is indicated by the last bool parameter.
          @param ipAddress IP address for the requested NetRecord
          @param port port number for the requested NetRecord
          @param protocol resip::TransportType for the requested NetRecord
          @param success pointer to a bool where success or failure can be indicated
          @return NetRecord corresponding to key values if success = true
       **/
      virtual NetRecord getNetRecord(const resip::Data& ipAddress,
                              const unsigned short port,
                              const resip::TransportType protocol,bool * success=0)=0;
                              
      /** @brief Returns a NetRecord for the given key.
      
          Success or failure of the call is indicated by the last bool parameter.
          @param key A key, probably built with buildKey(...) method, used to look up the NetRecord
          @param success pointer to a bool where success or failure can be indicated
          @return NetRecord corresponding to key if success = true
       **/
      virtual NetRecord getNetRecord(const resip::Data& key, bool * success=0)=0;

      /** @brief Set the data for a (new) NetRecord.
          @param ipAddress IP address for the NetRecord
          @param port port number for the NetRecord
          @param protocol resip::TransportType for the NetRecord
          @param domains list of domain names for the NetRecord
          @param enabled indicates whether NetRecord should be enabled
          @param rrEnabled indicates whether record-route should be enabled (only applicable for proxies)
          @return bool, true if successful
       **/
      virtual bool setNetRecord(const resip::Data& ipAddress,
                        const unsigned short port,
                        const resip::TransportType protocol,
                        std::list<resip::Data>& domains,
                        const bool enabled,
                        const bool rrEnabled)=0;
                        
      /** @brief Update the data for a NetRecord corresponding to the given key.
          @param key A key, probably built with buildKey(...) method, used to look up the NetRecord
          @param domains list of domain names for the NetRecord
          @param enabled indicates whether NetRecord should be enabled
          @param rrEnabled indicates whether record-route should be enabled (only applicable for proxies)
          @return bool, true if successful
       **/
      virtual bool updateNetRecord(const resip::Data& key,
                           std::list<resip::Data>& domains,
                           const bool enabled,
                           const bool rrEnabled)=0;
                        
      /** @brief Removed the NetRecord corresponding to the given key.
          @param key A key, probably built with buildKey(...) method, used to look up the NetRecord
          @return bool, true if successful
       **/
      virtual bool eraseNetRecord(const resip::Data& key)=0;
                        
      /** @brief Set NetRecord corresponding to the given key values to active.
          @param ipAddress IP address for the NetRecord
          @param port port number for the NetRecord
          @param protocol resip::TransportType for the NetRecord
          @return bool, true if successful
       **/
      virtual bool setActive(const resip::Data& ipAddress,
                     const unsigned short port,
                     const resip::TransportType protocol)=0;
                     
      /** @brief Set NetRecord corresponding to the given key values to active.
          @param key A key, probably built with buildKey(...) method, used to look up the NetRecord
          @return bool, true if successful
       **/
      virtual bool setActive(const resip::Data& key)=0;
    
      /**
        @brief getRecords fills a vector with all the records from the NetStore
        @param recordVector the vector to fill with the records
        @author NaG
      **/
      virtual void getRecords(std::vector<NetRecord> & recordVector)=0;
      
      /**
        @brief getKeys fills a vector with the keys of the records in the NetStore
        @param keyVector the vector to fill with the keys
        @author NaG
      **/
      virtual void getKeys(std::vector<resip::Data> & keyVector)=0;
      

      /** @brief Returns the full set of domains specified by all the NetRecords.
          @return set of domains
		  @todo !bwc! Not sure if this belongs in the base class.
       **/
      virtual std::set<resip::Data> getAllDomainsAllInterfaces()=0;

      /** @brief Builds a key based on the information that makes up the key.
          @param ipAddress IP address for the NetRecord key
          @param port port number for the NetRecord key
          @param protocol resip::TransportType for the NetRecord key
          @return Data representing key
       **/
      virtual resip::Data buildKey(const resip::Data& ipAddress,
                           const unsigned short port,
                           const resip::TransportType protocol)=0;      
};

}

#endif

/* Copyright 2007 Estacado Systems */
