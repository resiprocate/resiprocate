#if !defined(RESIP_PUBLICATIONHANDLER_HXX)
#define RESIP_PUBLICATIONHANDLER_HXX

#include "resiprocate/dum/Handles.hxx"

namespace resip
{
class ClientPublication;
class ServerPublication;
class SipMessage;
class SecurityAttributes;

class ClientPublicationHandler
{
   public:
      /// Called when the publication succeeds or each time it is sucessfully
      /// refreshed. 
      virtual void onSuccess(ClientPublicationHandle, const SipMessage& status)=0;

      //publication was successfully removed
      virtual void onRemove(ClientPublicationHandle, const SipMessage& status)=0;

      //call on failure. The usage will be destroyed.  Note that this may not
      //necessarily be 4xx...a malformed 200, etc. could also reach here.      
      virtual void onFailure(ClientPublicationHandle, const SipMessage& status)=0;

      // ?dcm? -- when should this be called
      virtual void onStaleUpdate(ClientPublicationHandle, const SipMessage& status)
      {}
};

class ServerPublicationHandler
{
   public:
      virtual void onInitial(ServerPublicationHandle, 
                             const Data& etag, 
                             const SipMessage& pub, 
                             const Contents* contents,
                             const SecurityAttributes* attrs, 
                             int expires)=0;
      virtual void onExpired(ServerPublicationHandle, const Data& etag)=0;
      virtual void onRefresh(ServerPublicationHandle, const Data& etag, 
                             const SipMessage& pub, 
                             const Contents* contents,
                             const SecurityAttributes* attrs,
                             int expires)=0;
      virtual void onUpdate(ServerPublicationHandle, 
                            const Data& etag, 
                            const SipMessage& pub, 
                            const Contents* contents,
                            const SecurityAttributes* attrs, 
                            int expires)=0;
      virtual void onRemoved(ServerPublicationHandle, 
                             const Data& etag, 
                             const SipMessage& pub,
                             int expires)=0;

      const Mimes& getSupportedMimeTypes() const;      
};
 
}

#endif
