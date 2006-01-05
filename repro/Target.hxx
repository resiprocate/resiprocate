#ifndef TARGET_HXX
#define TARGET_HXX 1

#include "resip/stack/Uri.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/Via.hxx"

namespace repro
{



class Target
{


   public:
   
      typedef enum
      {
         Pending, //Transaction has not started
         Trying, //Transaction has started
         Proceeding, //Transaction has received a provisional response
         WaitingToCancel,
         Terminated, //Transaction has received a final response
         NonExistent //The state of transactions that do not exist
      } Status;
   
      Target();
      Target(const resip::Uri& uri);
      Target(const resip::NameAddr& target);
      Target(const repro::Target& target);

      virtual ~Target();
      
      virtual const resip::Data& tid() const;
      
      virtual Status& status();
      virtual const Status& status() const;
      
      virtual resip::Uri& uri();
      virtual const resip::Uri& uri() const;
      
      virtual resip::Via& via();
      virtual const resip::Via& via() const;
      
      virtual resip::NameAddr& nameAddr();
      virtual const resip::NameAddr& nameAddr() const;
      
   private:
      Status mStatus;
      resip::Via mVia;
      resip::NameAddr mNameAddr;
      
};// class Target

}// namespace repro

#endif
