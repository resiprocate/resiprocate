#if !defined(DUM_CommandLineParser_hxx)
#define DUM_CommandLineParser_hxx

#include <vector>
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/Data.hxx"

namespace resip
{

class CommandLineParser
{
   public:
      CommandLineParser(int argc, char** argv);
      static resip::Uri toUri(const char* input, const char* description);
      static std::vector<resip::Uri> toUriVector(const char* input, const char* description);


      Data mLogType;
      Data mLogLevel;
      bool mEncrypt;
      bool mSign;
      bool mGenUserCert;
      Data mTlsDomain;
      
      int mUdpPort;
      int mTcpPort;
      int mTlsPort;
      int mDtlsPort;
      
      bool mRegisterDuration;
      bool mNoV4;
      bool mNoV6;
      
      Uri mAor;
      Data mPassword;
      
      Uri mOutboundProxy;
      Uri mContact;
      std::vector<Uri> mBuddies;
      Uri mTarget;
      Data mPassPhrase;
      Data mCertPath;
};
 
}

#endif

