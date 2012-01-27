/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef KEY_VALUE_DB_IF_HXX
#define KEY_VALUE_DB_IF_HXX 1

#include "rutil/Data.hxx"
#include <vector>

namespace resip
{

/**
   @brief Interface class for simple databases that hold key-value pairs.
   @ingroup database_related
*/
class KeyValueDbIf
{
   public:
      KeyValueDbIf(){};
      virtual ~KeyValueDbIf(){};

      /**
        @brief Returns a vector of the keys in the DB
        @return a vector of the keys in the DB
      **/
      virtual void getKeys(std::vector<resip::Data>& container) = 0;
      // Db manipulation routines
      virtual void dbWriteRecord(const resip::Data& key, 
                                  const resip::Data& data ) =0;
      /// return false if not found     
      virtual bool dbReadRecord( const resip::Data& key, 
                                 resip::Data& data ) =0;
      virtual void dbEraseRecord(const resip::Data& key ) =0;
      

};

}

#endif

/* Copyright 2007 Estacado Systems */
