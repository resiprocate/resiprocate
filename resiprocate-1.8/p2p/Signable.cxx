#include "p2p/Signable.hxx"

using namespace p2p;
using namespace s2c;

s2c::SignatureStruct *
Signable::sign(const std::vector<resip::Data> &signableData)
{
	SignatureStruct *sig = new SignatureStruct;
	sig->mAlgorithm = new SignatureAndHashAlgorithmStruct();
	sig->mIdentity = new SignerIdentityStruct();
	sig->mIdentity->mIdentityType = signer_identity_peer;

	return sig;
}

