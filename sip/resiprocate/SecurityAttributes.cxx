#include "resiprocate/SecurityAttributes.hxx"

using namespace resip;

SecurityAttributes::SecurityAttributes()  :
   
   mIsEncrypted(false),
   mSigStatus(SignatureNone),
   mStrength(From)
{}

SecurityAttributes::~SecurityAttributes() 
{};
