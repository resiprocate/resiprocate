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

    // Set up both variants
    ns1.mZero.mZeroArm=9;
    ns1.mOne.mOneArm1=99;
    memset(ns1.mOne.mOneArm2,0xff,10);
    unsigned char blahblah=0x11;
    ns1.mOne.mOneArm3.push_back(blahblah);
    
    // Now choose one
//    ns1.mSwitchtype=s2c::zero;
    ns1.mSwitchtype=s2c::one;

    fout.open("test.out");
    ns1.encode(fout);
    fout.close();    

    
    // Now read it back in again
    fin.open("test.out");
    s2c::NamedStruct ns2;
    ns2.decode(fin);
    
    std::cout << "Type is" << std::hex << (int)ns2.mSwitchtype << "\n";
//    std::cout << std::hex << (int)ns2.mZero.mZeroArm << "\n";
  }

