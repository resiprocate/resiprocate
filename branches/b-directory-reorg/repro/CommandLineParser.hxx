#if !defined(DUM_CommandLineParser_hxx)
#define DUM_CommandLineParser_hxx

#include <vector>
#include "resiprocate/Uri.hxx"
#include "rutil/Data.hxx"

namespace resip
{

class CommandLineParser
{
   public:
      CommandLineParser(int argc, char** argv);
      static resip::Uri toUri(const char* input, const char* description);
      static std::vector<resip::Data> toVector(const char* input, const char* description);
      
      Data mLogType;
      Data mLogLevel;
      Data mTlsDomain;
      int mUdpPort;
      int mTcpPort;
      int mTlsPort;
      int mDtlsPort;
      bool mUseV4;
      bool mUseV6;
      std::vector<Data> mDomains;
      Data mCertPath;
      bool mNoChallenge;
      bool mNoWebChallenge;
      bool mNoRegistrar;
      bool mCertServer;
      Data mRequestProcessorChainName;
      Data mMySqlServer;
      int mHttpPort;
};
 
}

#endif

