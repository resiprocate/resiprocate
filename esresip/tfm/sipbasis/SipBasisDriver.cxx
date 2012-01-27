#include "SipBasisDriver.hxx"

#include "cria.h"
#include "SIPStack.h"

SipBasisDriver::SipBasisDriver() 
{
  
  stackConfig = new basis::StackConfiguration(6060);

  stack = new basis::SIPStack;
  {
    int ok = stack->init(stackConfig);
    assert (ok);
  }


  stack->registerTransactionLifecycleObserver(&msgHandler);
  
  stack->start();

}

SipBasisDriver::~SipBasisDriver()
{
  stack->join();
  delete stack;
  delete stackConfig;
}

/* Copyright 2007 Estacado Systems */
