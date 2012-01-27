/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#include "resip/stack/KeyValueTransportDb.hxx"
#include "rutil/DbSerializationHelper.hxx"

namespace resip
{

KeyValueTransportDb::KeyValueTransportDb(std::auto_ptr<resip::KeyValueDbIf> ifdb)
{
    mInterface = ifdb.release();
}

KeyValueTransportDb::~KeyValueTransportDb()
{
    delete mInterface;
}

void 
KeyValueTransportDb::addInterface( const Key& key, const InterfaceRecord& rec )
{
   resip::Data value;
   {
      resip::oDataStream s(value);
      DbSerializationHelper::encodeShort(11,s); //Version number
      DbSerializationHelper::encodeData(rec.ipAddress,s);
      DbSerializationHelper::encodeUShort(rec.port,s);
      DbSerializationHelper::encodeData(resip::toData(rec.protocol),s);
      DbSerializationHelper::encodeDataList(rec.domains,s);
      DbSerializationHelper::encodeBool(rec.enabled,s);
      DbSerializationHelper::encodeBool(rec.rrEnabled,s);
      s.flush();
   }
   
   mInterface->dbWriteRecord(key,value);
}

void 
KeyValueTransportDb::eraseInterface(  const Key& key )
{
   mInterface->dbEraseRecord(key);
}

void
KeyValueTransportDb::getAllInterfaces(KeyValueTransportDb::InterfaceRecordList& container)
{   
   std::vector<KeyValueTransportDb::Key> keys;
   std::vector<KeyValueTransportDb::Key>::iterator iter;

   getInterfaceKeys(keys);
   
   for ( iter=keys.begin(); iter!=keys.end(); iter++)
   {      
      container.push_back(getInterface(*iter));
   }
}

KeyValueTransportDb::InterfaceRecord
KeyValueTransportDb::getInterface( const Key& key) const
{
   InterfaceRecord rec;
   resip::Data value;
   mInterface->dbReadRecord(key,value);
   {
      iDataStream s(value);
      short version=DbSerializationHelper::decodeShort(s);
      if(version==11)
      {
         rec.ipAddress=DbSerializationHelper::decodeData(s);
         rec.port=DbSerializationHelper::decodeUShort(s);
         rec.protocol=resip::toTransportType(DbSerializationHelper::decodeData(s));
         rec.domains=DbSerializationHelper::decodeDataList(s);
         rec.enabled=DbSerializationHelper::decodeBool(s);
         rec.rrEnabled=DbSerializationHelper::decodeBool(s);
      }
   }
   
   return rec;
}

void
KeyValueTransportDb::getInterfaceKeys(std::vector<resip::Data>& container)
{
   mInterface->getKeys(container);
}


}

/* Copyright 2007 Estacado Systems */
