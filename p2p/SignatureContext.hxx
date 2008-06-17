#ifndef __P2P_SIGNATURE_CONTEXT_HXX
#define __P2P_SIGNATURE_CONTEXT_HXX 1

namespace p2p
{

#include "ConfigObject.hxx"

class SignatureContext
{
   public:
      SignatureContext(ConfigObject &);
      
      // Compute signatures, one function for each signature type
      // Returns an encoded "Signature" object
      Data computeSignatureWithPeerIdentity(IStream &toBeSigned);
      Data computeSignatureWithUserName(IStream &toBeSigned);
      Data computeSignatureWithCertificate(IStream &toBeSigned);

      // Fetch all the certificates corresponding to this set of
      // signatures, works in terms of encoded "Signature" objects
      void fetchCertificates(const vector<Data &> signatures, Fifo &done);
      // Validate a signature, comparing to the expected NodeId node
      bool validateSignature(IStream &toBeSigned, const Data &signature,
                             NodeId node);
      // Validate a signature, comparing to the expected UserName name
      bool validateSignature(IStream &toBeSigned, const Data &signature,
                             UserName user);
};
}


#endif


