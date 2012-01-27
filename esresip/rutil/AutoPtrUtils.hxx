/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef ES_AUTO_PTR_UTILS
#define ES_AUTO_PTR_UTILS

namespace resip
{
template<typename _Tp1, typename _Tp2>
std::auto_ptr<_Tp1> auto_dynamic_cast(std::auto_ptr<_Tp2>& orig)
{
   std::auto_ptr<_Tp1> result(0);
   _Tp1* ptr = dynamic_cast<_Tp1*>(orig.get());
   if(ptr)
   {
      orig.release();
      result.reset(ptr);
   }
   
   return result;
}

}

#endif

/* Copyright 2007 Estacado Systems */
