


class Profile
{
   public:
      void setOutboundProxy( const Data& uri );
      
      void setAor( const Data& aor );
      void setAorPassword( const Data& password );
      
      void addDigestCredential( const Data& realm, const Data& users, const Data& password);
      
      class DigestCredentialHandler
      {
         public:
            virtual void 
      };
      
      void setDigestHandler( DigestCredentialHandler* handler );
      
         
   private:
};
   

    
