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
#include "test.hxx"
#include <iostream>
#include <fstream>
#include <vector>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST


  
int main(int argc, char **argv)
  {


    /* TESTS */
    std::vector<int> v(2);
    v[1]=0;



    s2c::test::FooPdu *f=new s2c::test::FooPdu();

    
    std::ofstream fout;

    resip::Log::initialize(resip::Data("cerr"),
      resip::Data("DEBUG"),resip::Data("test_s2c"));
    InfoLog(<<"XXXX");
    
    fout.open("test.out");


    f->mBar='X';
    f->mMumble=new s2c::test::BazTypePdu();
    f->mMumble->mZzz=0x9988;
    f->mId=new s2c::test::PeerIdPdu();
    memset(f->mId->mId, 0x65, 16);
    f->mZulu=0xffff;
    f->mVariable.push_back(0x11);
    f->mVariable.push_back(0x22);

    f->print(&std::cout, 0);

    f->encode(&fout);
    fout.close();


    std::ifstream fin;
    fin.open("test.out");
    
    s2c::test::FooPdu *f2=new s2c::test::FooPdu();
    
    f2->decode(&fin);

    f2->print(&std::cout, 0);
  }

