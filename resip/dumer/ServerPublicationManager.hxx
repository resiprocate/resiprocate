#if !defined(DUM_ServerPublicationManager_hxx)
#define DUM_ServerPublicationManager_hxx

namespace resip
{

class ServerPublicationManager
{
   public:

   private:
      // from ETag -> ServerPublication
      typedef std::map<Data, ServerPublication*> ServerPublications;
      ServerPublications mServerPublications;
};

}

#endif
