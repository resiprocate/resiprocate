#if !defined(RESIP_SERVERPUBLICATION_HXX)
#define RESIP_SERVERPUBLICATION_HXX

namespace resip
{

class ServerPublication : public BaseUsage 
{
   public:

      class Handle
      {
      };  
      
      // application may have to muck with expires or Etag
      void accept(const SipMessage& ok);
      void reject(int statusCode);
};

}

#endif
