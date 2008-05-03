/**
   s2c_native.hxx

   Copyright (C) 2008, RTFM, Inc.
   All Rights Reserved.

   ekr@rtfm.com  Fri May  2 16:58:21 2008
*/


#ifndef _s2c_native_h
#define _s2c_native_h

#include <iostream>
#include <string>

namespace s2c {


class PDU {
   public:
      std::string mName;

      virtual void print(std::iostream out);
      virtual void encode(std::iostream out)=0;
      virtual void decode(std::iostream in)=0;
};

#endif


}
