/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

/* Copyright 2007 Estacado Systems */
#ifndef Command_Include_Guard
#define Command_Include_Guard

namespace resip
{

/**
   Dirt-simple Command pattern. Nothing fancy here. (It is assumed that these
   do not delete themselves after invocation.)
*/
class Command
{
   public:
      virtual ~Command(){};
      virtual void operator()()=0;
};
}

#endif
