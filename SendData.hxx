#ifndef RESIP_SendData_HXX
#define RESIP_SendData_HXX

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Tuple.hxx"

namespace resip
{

class SendData
{
   public:
      SendData(const Tuple& dest, const Data& pdata, const Data& tid): 
         destination(dest),
         data(pdata),
         transactionId(tid) 
      {}
      Tuple destination;
      const Data data;
      const Data transactionId;
};

}

#endif
