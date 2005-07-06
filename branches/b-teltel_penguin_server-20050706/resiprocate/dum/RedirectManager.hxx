#if !defined(RESIP_REDIRECTMANAGER_HXX)
#define RESIP_REDIRECTMANAGER_HXX


#include <set>
#include <queue>
#include <functional>
#include <vector>

#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/NameAddr.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/DialogSetId.hxx"
#include "resiprocate/os/HashMap.hxx"

//8.1.3.4
//19.1.5

namespace resip
{

class DialogSet;

class RedirectManager
{
   public:   
      class Ordering : public std::binary_function<const NameAddr&, const NameAddr&, bool>
      {
         public:
            virtual bool operator()(const NameAddr& lhs, const NameAddr& rhs) const;
      };

      bool handle(DialogSet& dSet, SipMessage& origRequest, const SipMessage& response);
      
      //deafult is by q-value, no q-value last
      void setOrdering(const Ordering& order);      
      
      void removeDialogSet(DialogSetId id);
      
      //based on follows 8.1.3.4 of 3261. Overload to interject user decisions
      //or to process 301, 305 or 380 reponses.
//      virtual void onRedirect(AppDialogSetHandle, const SipMessage& response);
      //use a priority queue by q-value, and a list(linear, set too heavy?) of
      //values that have been tried. Do q-values really have any meaning across
      //different 3xx values? Or should the first 3xx always win.
   protected:      
      class TargetSet
      {
         public:
            TargetSet(const SipMessage& request, const Ordering& order) :
               mTargetQueue(order),
               mRequest(request)
            {}
            
            void addTargets(const SipMessage& msg);
            //pass in the message stored in the creator
            bool makeNextRequest(SipMessage& request);
         protected:
            typedef std::set<NameAddr> EncounteredTargetSet;      
            typedef std::priority_queue<NameAddr, std::vector<NameAddr>, Ordering> TargetQueue;

            EncounteredTargetSet mTargetSet;
            TargetQueue mTargetQueue;            
            //mLastRequest in creator is kept in sync with this, this is needed
            //so that embedded information in a target uri does not migrate to
            //the wrong attempt
            SipMessage mRequest;                        
      };      

      typedef HashMap<DialogSetId, TargetSet*> RedirectedRequestMap;
      RedirectedRequestMap mRedirectedRequestMap;
      Ordering mOrdering;      
};

 
}

#endif
