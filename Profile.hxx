#if !defined(RESIP_PROFILE_HXX)
#define RESIP_PROFILE_HXX

namespace resip
{

class Profile
{
   public:
      void setOutboundProxy( const Data& uri );

      /// The following functions deal with getting digest credentals 
      //@{ 

      void addDigestCredential( const Data& realm, const Data& users, const Data& password);
      
      /** This class is used as a callback to get digest crednetials. The
       * derived class must define one of computeA1 or getPaswword. computeA1 is
       * tried first and it it returns an empty string, then getPassword is
       * tried. */
      class DigestCredentialHandler
      {
         public:
            virtual Data computeA1( const Data& realm, const Data& users );
            virtual Data getPassword( const Data& realm, const Data& users );
      };
      
      void setDigestHandler( DigestCredentialHandler* handler );
      //@}
         
   private:
};
   

 
}

#endif
