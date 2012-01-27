/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef QDBM_DB_HXX
#define QDBM_DB_HXX 1
#include "rutil/Data.hxx"
#include "rutil/KeyValueDbIf.hxx"
#include <string>

extern "C"
{
#include <depot.h>
}

namespace resip
{
  class TransactionUser;
}

namespace resip
{
/**
@internal
*/
class QDBMDb: public KeyValueDbIf
{
   public:
      QDBMDb();
      QDBMDb(const resip::Data& dbName);
      
      virtual ~QDBMDb();

      bool isSane();
      
      
   private:
   
      void init(const resip::Data& dbName);
      //DbEnv mEnv; // !cj! TODO - move to using envoronments
      DEPOT*   mDb;

      bool sane;
      
      virtual void getKeys(std::vector<resip::Data>& container);
      // Db manipulation routines
      virtual void dbWriteRecord( const resip::Data& key, 
                                  const resip::Data& data );
      virtual bool dbReadRecord( const resip::Data& key, 
                                 resip::Data& data ); // return false if not found
      virtual void dbEraseRecord( const resip::Data& key );
      virtual void dbNextKey( bool first,resip::Data& key); // return empty if no more  
};

}


#endif
/* Copyright 2007 Estacado Systems */
