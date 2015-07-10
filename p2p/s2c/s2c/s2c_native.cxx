/**
   s2c_native.cxx

   Copyright (C) 2008, RTFM, Inc.
   All Rights Reserved.

   ekr@rtfm.com  Fri May  2 17:04:55 2008
*/



#include "rutil/ResipAssert.h"
#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/ParseException.hxx"
#include "s2c_native.hxx"

void s2c::encode_uintX(std::ostream& out, const unsigned int bits, const u_int64 value)
  {
    int size;

    resip_assert((bits%8)==0);
    resip_assert(bits<=64);

    size=bits/8;
    
    switch(size){
      case 8:
        out.put(((value>>56)&0xff));
      case 7:
        out.put(((value>>48)&0xff));
      case 6:
        out.put(((value>>40)&0xff));
      case 5:
        out.put(((value>>32)&0xff));
      case 4:
        out.put(((value>>24)&0xff));
      case 3:
        out.put(((value>>16)&0xff));
      case 2:
        out.put(((value>>8)&0xff));
      case 1:
        out.put(value&0xff);
        break;
      default:
        resip_assert(1==0);
    }
  }
  

void s2c::decode_uintX(std::istream& in, const unsigned int bits, u_char &value)
  {
    int size;
    int c;      
    
    resip_assert(bits<=8);

    size=bits/8;
    
    value=0;

    while(size--){
      value <<=8;
      c=in.get();
     
      if(c==EOF)
        throw resip::ParseException("Unexpected end of encoding","",__FILE__,__LINE__);
      value |= c;
    }
  }


/*void s2c::decode_uintX(std::istream& in, const unsigned int bits, u_int8 &value)
  {
    int size;
    int c;      
    
    assert(bits==8);

    size=bits/8;
    
    value=0;

    while(size--){
      value <<=8;
      c=in->get();
      value |= c;
    }
  }
*/
void s2c::decode_uintX(std::istream& in, const unsigned int bits, u_int16 &value)
  {
    int size;
    int c;
    
    resip_assert(bits<=16);

    size=bits/8;
    
    value=0;

    while(size--){
      value <<=8;
      c=in.get();
      if(c==EOF)
        throw resip::ParseException("Unexpected end of encoding","",__FILE__,__LINE__);


      value |= c;
    }
  }

void s2c::decode_uintX(std::istream& in, const unsigned int bits, u_int32 &value)
  {
    int size;
    int c;
    
    resip_assert(bits<=32);

    size=bits/8;
    
    value=0;

    while(size--){
      value <<=8;
      
      c=in.get();
      if(c==EOF)
        throw resip::ParseException("Unexpected end of encoding","",__FILE__,__LINE__);

      value |= c;
    }
  }

void s2c::decode_uintX(std::istream& in, const unsigned int bits, u_int64 &value)
  {
    int size;
    int c;
    
    resip_assert(bits<=64);

    size=bits/8;
    
    value=0;

    while(size--){
      value <<=8;
      c=in.get();
      if(c==EOF)
        throw resip::ParseException("Unexpected end of encoding","",__FILE__,__LINE__);
      value |= c;
    }
  }
    
void s2c::do_indent(std::ostream& out, int indent)
 {
   while(indent--)  out  << ' ';
 }

// This is really clumsy, but I don't understand rutil
// TODO: !ekr! cleanup
void s2c::read_varray1(std::istream& in, unsigned int lenlen, resip::Data &buf)
  {
    u_int64 len=0;
    int c;
    
    // First read the length
    resip_assert(lenlen<=8);
    while(lenlen--){
      len<<=8;
      c=in.get();
      if(c==EOF)
        throw resip::ParseException("Unexpected end of encoding","",__FILE__,__LINE__);
      len|=c;
    }
    
    resip::DataStream out(buf);
    
    while(len--){
      c=in.get();
      if(c==EOF)
        throw resip::ParseException("Unexpected end of encoding","",__FILE__,__LINE__);

      out.put(c);
    }
    
    out.flush();
  }

s2c::PDU::~PDU()
{
}

std::ostream& 
operator<<(std::ostream& strm, const s2c::PDU& pdu)
{
   pdu.print(strm);
   return strm;
}

