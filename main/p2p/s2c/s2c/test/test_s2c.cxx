/**
   test_s2c.cxx

   Copyright (C) 2008, RTFM, Inc.
   All Rights Reserved.

   ekr@rtfm.com  Sun May  4 16:13:37 2008
*/



#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"
#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "selectGen.hxx"
#include <iostream>
#include <fstream>
#include <vector>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

  
int main(int argc, char **argv)
  {
    std::ofstream fout;

    resip::Log::initialize(resip::Data("cerr"),
      resip::Data("DEBUG"),resip::Data("test_s2c"));
    InfoLog(<<"XXXX");



    // Test select
    s2c::NamedStruct ns;
    
    ns.mType=ns.tZero;
    ns.mZero.mZeroArm=9;

    fout.open("test.out");
    ns.encode(fout);
    fout.close();    

    
    
  }

