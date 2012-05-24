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
typedef unsigned char u_int8;
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
      virtual ~PDU()=0;
      
      std::string mName;

      virtual void print(std::ostream& out) const
      {
        print(out, 0);
      }
    
      virtual void print(std::ostream& out, int indent)  const
      {
         out << mName << "(empty)" << "\n";
      }

      virtual void encode(std::ostream& out) const=0;
      virtual void decode(std::istream& in)=0;
};

#define PDUMemberFunctions                                  \
virtual void print(std::ostream& out, int indent) const;    \
virtual void encode(std::ostream& out) const;                     \
virtual void decode(std::istream& in);


/* Functions for primitive integral types */
void encode_uintX(std::ostream& out, const unsigned int bits, const u_int64 value);
void decode_uintX(std::istream& in, const unsigned int bits, u_char &value);
//void decode_uintX(std::istream& in, const unsigned int bits, u_int8 &value);
void decode_uintX(std::istream& in, const unsigned int bits, u_int16 &value);
void decode_uintX(std::istream& in, const unsigned int bits, u_int32 &value);
void decode_uintX(std::istream& in, const unsigned int bits, u_int64 &value);

void do_indent(std::ostream& out, int indent);
void read_varray1(std::istream& in, unsigned int lenlen, resip::Data &buf);

} /* Close of namespace */

std::ostream& operator<<(std::ostream& strm, const s2c::PDU& pdu);

#endif
