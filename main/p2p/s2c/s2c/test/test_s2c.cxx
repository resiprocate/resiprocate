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
    std::ifstream fin;

    resip::Log::initialize(resip::Data("cerr"),
      resip::Data("DEBUG"),resip::Data("test_s2c"));
    InfoLog(<<"XXXX");


    // ***** Test select  *****
    // Cook up an ns and write it
    s2c::NamedStruct ns1;
    ns1.mType=ns1.tZero;
    ns1.mZero.mZeroArm=9;

    fout.open("test.out");
    ns1.encode(fout);
    fout.close();    

    
    // Now read it back in again
    fin.open("test.out");
    s2c::NamedStruct ns2;
    ns2.mType=ns2.tZero;
    ns2.decode(fin);
    
    std::cout << std::hex << (int)ns2.mZero.mZeroArm << "\n";
  }

