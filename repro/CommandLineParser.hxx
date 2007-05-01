#if !defined(DUM_CommandLineParser_hxx)
#define DUM_CommandLineParser_hxx

#include <vector>
#include "resip/stack/Uri.hxx"
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
      Data mEnumSuffix;
      bool mShouldRecordRoute;
      resip::Uri mRecordRoute;
      int mUdpPort;
      int mTcpPort;
      int mTlsPort;
      int mDtlsPort;
      bool mUseV4;
      bool mUseV6;
      std::vector<Data> mDomains;
      std::vector<Data> mInterfaces;
      std::vector<Data> mRouteSet;
      Data mCertPath;
      bool mNoChallenge;
      bool mNoWebChallenge;
      bool mNoRegistrar;
      bool mNoIdentityHeaders;
      bool mCertServer;
      Data mRequestProcessorChainName;
      Data mMySqlServer;
      int mHttpPort;
      bool mRecursiveRedirect;
      bool mDoQValue;
      Data mForkBehavior;
      bool mCancelBetweenForkGroups;
      bool mWaitForTerminate;
      int mMsBetweenForkGroups;
      int mMsBeforeCancel;
      bool mAllowBadReg;
      bool mParallelForkStaticRoutes;
      int mTimerC;
      Data mAdminPassword;
};
 
}

#endif

