/**
   s2c_native.hxx

   Copyright (C) 2008, RTFM, Inc.
   All Rights Reserved.

   ekr@rtfm.com  Fri May  2 16:58:21 2008
*/


#ifndef _s2c_native_h
#define _s2c_native_h

#include <iostream>
#include <vector>
#include <string>

/* Typedefs for integral types */
typedef unsigned char u_char;
typedef unsigned short u_int8;
typedef unsigned short u_int16;
typedef unsigned int u_int32;
typedef unsigned long long u_int64;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;

namespace s2c {

class PDU {
   public:
      std::string mName;

      virtual void print(std::iostream *out) 
      {
        (*out) << mName << "\n";
      }
          
      virtual void encode(std::iostream *out)=0;
      virtual void decode(std::iostream *in)=0;
};

#define PDUMemberFunctions \
  virtual void print(std::iostream *out); virtual void encode(std::iostream *out); virtual void decode(std::iostream *in);


/* Functions for primitive integral types */
void encode_uintX(std::iostream *out, const unsigned int bits, const u_int64 value);


}

#endif
