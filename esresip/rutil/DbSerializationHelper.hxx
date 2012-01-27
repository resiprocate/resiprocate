/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef DB_SERIALIZATION_HELPER_HXX
#define DB_SERIALIZATION_HELPER_HXX 1

#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include <list>

namespace resip
{

/**
   @brief Collection of static functions to assist in 
      serialization/deserialization of various primitives/objects.
   @ingroup text_proc
   @ingroup database_related
*/
class DbSerializationHelper
{
   public:
      /// Writes the token to the data stream s.
      static void encodeData(const resip::Data& token,resip::oDataStream& s) ;

      /// Writes the token to the data stream s.
      static void encodeInt(const int token,resip::oDataStream& s) ;

      /// Writes the token to the data stream s.
      static void encodeShort(const short token,resip::oDataStream& s) ;

      /// Writes the token to the data stream s.
      static void encodeUShort(const unsigned short token,resip::oDataStream& s) ;

      /// Iterates over the list of tokens and writes each to the data
      /// stream. An additional string token 'dl' is written to the
      /// stream to convey that the following data should be read as a
      /// Data List.
      static void encodeDataList(const std::list<resip::Data>& token,resip::oDataStream& s) ;

      /// Iterates over the list of ints and writes each to the data
      /// stream. An additional string token 'il' is written to the
      /// stream to convey that the following data should be read as an
      /// Int List.
      static void encodeIntList(const std::list<int>& token,resip::oDataStream& s) ;
  
      /// Iterates over the list of unsigned shorts and writes each to
      /// the data stream. An additional string token 'usl' is written to the
      /// stream to convey that the following data should be read as an
      /// Unsigned Short List.
      static void encodeUShortList(const std::list<unsigned short>& token,resip::oDataStream& s) ;

      /// Write the token to the data stream.
      static void encodeBool(const bool& token,resip::oDataStream& s) ;
      
      /// @returns the decoded Data from the input data stream
      static resip::Data decodeData(resip::iDataStream& s) ;

      /// @returns the decoded int from the input data stream
      static int decodeInt(resip::iDataStream& s) ;

      /// @returns the decoded short from the input data stream
      static short decodeShort(resip::iDataStream& s) ;
  
      /// @returns the decoded unsigned short from the input data stream
      static unsigned short decodeUShort(resip::iDataStream& s) ;

      /// @returns the list of Data objects read in from the data stream.
      static std::list<resip::Data> decodeDataList(resip::iDataStream& s) ;

      /// @returns the list of ints read in from the data stream.
      static std::list<int> decodeIntList(resip::iDataStream& s) ;

      /// @returns the list of unsigned shorts read in from the data stream.
      static std::list<unsigned short> decodeUShortList(resip::iDataStream& s) ;

      /// @returns the bool read in from the data stream.
      static bool decodeBool(resip::iDataStream& s) ;

};
}

#endif

/* Copyright 2007 Estacado Systems */
