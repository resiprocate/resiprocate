/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#include "repro/QDBMDb.hxx"

extern "C"
{
#include <depot.h>
}

#include <fcntl.h>
#include <cassert>

#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"

#include "rutil/EsLogger.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

QDBMDb::QDBMDb( )
{ 
   init("repro_qdbm.db");
}


QDBMDb::QDBMDb( const resip::Data& dbName )
{ 
   init(dbName);
}

void
QDBMDb::init(const resip::Data& fileName)
{
   InfoLog( << "Using QDBM " << fileName );
   sane=true;
   

   mDb=dpopen(fileName.c_str(),DP_OWRITER | DP_OCREAT,0);
    
   if ( !mDb )
   {
      ErrLog( <<"Could not open user database at " << fileName );
      sane = false;
      mDb=NULL;
   }
   else
   {
      dpiterinit(mDb);
   }
      

}

QDBMDb::~QDBMDb()
{  
   if( mDb )
   {
      dpsync(mDb);
      dpclose(mDb);
      mDb=0;
   }
}

void QDBMDb::getKeys(std::vector<resip::Data>& container)
{
    resip::Data key;
      dbNextKey(true,key);
    while(key != resip::Data::Empty)
    {
        container.push_back(key);
        dbNextKey(false,key);
    }
}

void 
QDBMDb::dbWriteRecord(  const resip::Data& key, 
                        const resip::Data& data )
{
   if(key.empty())
   {
      return;
   }

   if( !mDb )
   {
      return;
   }
   
   
   if(!dpput(mDb,key.data(),key.size(),data.data(),data.size(),DP_DOVER))
   {
      
   }

}


bool 
QDBMDb::dbReadRecord(const resip::Data& key, 
                     resip::Data& data )
{ 

   if(key.empty())
   {
      return false;
   }
   
   if( !mDb )
   {
      return false;
   }
   
   int retSize=0;
   char *ret=0;
   ret = dpget(mDb,key.data(),key.size(),0,-1,&retSize);

   if(ret)
   {
      data=Data(ret,retSize);
      free(ret);
      return true;
   }

   return false;

}


void 
QDBMDb::dbEraseRecord(  const resip::Data& key )
{ 
   if(key.empty())
   {
      return;
   }
   
   if( !mDb )
   {
      return;
   }
   
   dpout(mDb,key.data(),key.size());

}


void 
QDBMDb::dbNextKey(bool first,resip::Data& key)
{ 
   key.clear();
   if( !mDb )
   {
      return;
   }
   
   if(first)
   {
      dpiterinit(mDb);
   }

   int keySize=0;
   char* prekey=0;
   prekey = dpiternext(mDb,&keySize);
   
   if(prekey)
   {
      key.append(prekey,keySize);
      free(prekey);
   }

}

bool
QDBMDb::isSane()
{
  return sane;
}

/* Copyright 2007 Estacado Systems */
