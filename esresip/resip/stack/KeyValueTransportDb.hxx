/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef KEY_VALUE_TRANSPORT_DB_HXX
#define KEY_VALUE_TRANSPORT_DB_HXX 1

#include "rutil/KeyValueDbIf.hxx"
#include "rutil/Data.hxx"
#include <memory>
#include <vector>
#include "resip/stack/TransportPersistenceManager.hxx"

namespace resip
{

/**
   @brief Simple database to read and write Transports.

   This class uses DbSerializationHelper to serialize into a value or
   deserialize from a value in a simple key-value database.

   @see KeyValueDbIf
   @see TransportPersistenceManager::NetRecord
   @see DbSerializationHelper
   
*/
class KeyValueTransportDb
{
   public:
      KeyValueTransportDb(std::auto_ptr<resip::KeyValueDbIf> ifdb);
      virtual ~KeyValueTransportDb();


      typedef TransportPersistenceManager::NetRecord InterfaceRecord;
      
      typedef std::vector<InterfaceRecord> InterfaceRecordList;
      typedef resip::Data Key;
      
      // functions for Interface Records
  
      /**@brief Adds an interface to the transport database.
	  
	     @param key Key for the key-value database.
		 @param rec the InterfaceRecord that will be serialized and written.
		 */
      virtual void addInterface( const Key& key, const InterfaceRecord& rec );

      /**
         @param key Key for the value that will be erased from the DB.
      */
      virtual void eraseInterface(  const Key& key );

      /**
         @param container Output parameter into which the results of a
         dump of the entire contents of the Transport database will be
         placed.
      */
      virtual void getAllInterfaces(InterfaceRecordList& container);

      /**
         @param container Output parameter into which the keys for
         every entry in the DB will be placed.
      */
      virtual void getInterfaceKeys(std::vector<Key>& container);

      /**
         @param key The key which will be used to look up a Transport
         record in the DB.

         @return The value associated with key.
      */
      virtual InterfaceRecord getInterface( const Key& key) const;

   protected:
      KeyValueDbIf* mInterface;
};

}

#endif

/* Copyright 2007 Estacado Systems */
