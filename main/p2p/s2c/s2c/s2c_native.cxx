/**
   s2c_native.cxx

   Copyright (C) 2008, RTFM, Inc.
   All Rights Reserved.

   ekr@rtfm.com  Fri May  2 17:04:55 2008
*/



#include <assert.h>
#include "s2c_native.hxx"

void s2c::encode_uintX(std::ostream *out, const unsigned int bits, const u_int64 value)
  {
    int size;

    assert((bits%8)==0);
    assert(bits<=64);

    size=bits/8;
    
    switch(size){
      case 8:
        out->put(((value>>56)&0xff));
      case 7:
        out->put(((value>>48)&0xff));
      case 6:
        out->put(((value>>40)&0xff));
      case 5:
        out->put(((value>>32)&0xff));
      case 4:
        out->put(((value>>24)&0xff));
      case 3:
        out->put(((value>>16)&0xff));
      case 2:
        out->put(((value>>8)&0xff));
      case 1:
        out->put(value&0xff);
        break;
      default:
        assert(1==0);
    }
  }
  
