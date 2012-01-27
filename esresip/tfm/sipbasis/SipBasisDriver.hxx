#ifndef SipBasisDriver_hxx
#define SipBasisDriver_hxx

#include "MsgHandler.hxx"

namespace basis {
   class StackConfiguration;
   class SIPStack;
};


class SipBasisDriver  {
  public:
     SipBasisDriver();
     ~SipBasisDriver();

  private:
     basis::StackConfiguration *stackConfig;
     basis::SIPStack *stack;
     MsgHandler msgHandler;
     
};

//class VoiceCall;
//call VoiceCallManager;
#endif 

/* Copyright 2007 Estacado Systems */
