#ifndef RESIP_DTLSMESSAGE_HXX
#define RESIP_DTLSMESSAGE_HXX

#ifdef USE_DTLS

#include "resiprocate/Message.hxx"

#include <openssl/ssl.h>

namespace resip
{

class DtlsMessage : public Message
{
   public:
      RESIP_HeapCount( DtlsMessage ) ;
      DtlsMessage( SSL *ssl )
         : mSsl( ssl ) 
            {}
      ~DtlsMessage() 
            {}
      Data brief() const
            { return Data( mSsl ) ; }
      Message * clone() const
            { return new DtlsMessage( mSsl ) ; }
      std::ostream& encode(std::ostream& strm) const
            { return strm ; }

      SSL *getSsl()
      { return mSsl ; }
      
   private:
      SSL *mSsl ;
} ;

}

#endif /* USE_DTLS */

#endif /* ! RESIP_DTLSMESSAGE_HXX */
