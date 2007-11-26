#ifdef HAVE_CONFIG_H
#include <config.hxx>
#endif

#if HAVE_POPT_H
#include <popt.h>
#endif

#include "CommandLineParser.hxx"
#include "repro/ReproVersion.hxx"
#include "rutil/Logger.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/ParseException.hxx"
#include "rutil/FileSystem.hxx"
#include "Parameters.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

CommandLineParser::CommandLineParser(int argc, char** argv)
{
   char* logType = "cout";
   char* logLevel = "INFO";
#ifndef WIN32
   char logFilePath[256]={'.',0};
#else
   char logFilePathBuf[MAX_PATH]={0};
#endif
   char *logFilePath=logFilePathBuf;
#ifdef WIN32
   static const Data AllUsersLogFilePath(Data( getenv( "ALLUSERSPROFILE" ) ) + "\\Application Data\\Resiprocate\\repro\\");
//when we run as restricted user
   static const Data LocalUserLogFilePath(Data( getenv( "USERPROFILE" ) ) + "\\Local Settings\\Application Data\\Resiprocate\\repro\\");

   if ( FileSystem::DirectoryExists( AllUsersLogFilePath ) && FileSystem::IsReadWriteAccess ( AllUsersLogFilePath.c_str() ) )
   {
      strcpy( logFilePath, AllUsersLogFilePath.c_str() );
   }
   else if ( FileSystem::IsReadWriteAccess ( getenv( "ALLUSERSPROFILE" ) ) )
   {
      assert ( FileSystem::ForceDirectories( AllUsersLogFilePath ) );
      FileSystem::SetAccessAs( AllUsersLogFilePath.c_str() , getenv( "ALLUSERSPROFILE" ) );
      strcpy( logFilePath, AllUsersLogFilePath.c_str() );
   }
   else if ( FileSystem::DirectoryExists( LocalUserLogFilePath ) && FileSystem::IsReadWriteAccess ( LocalUserLogFilePath.c_str() ) )
   {
      strcpy( logFilePath, LocalUserLogFilePath.c_str() );
   }
   else if ( FileSystem::IsReadWriteAccess ( getenv( "USERPROFILE" ) ) )
   {
      assert ( FileSystem::ForceDirectories( LocalUserLogFilePath ) );
      FileSystem::SetAccessAs( LocalUserLogFilePath.c_str(), getenv( "USERPROFILE" ) );
      strcpy( logFilePath, LocalUserLogFilePath.c_str() );
   }
#endif
   char* tlsDomain = 0;
   char* recordRoute = 0;
   int udpPort = 5060;
   int tcpPort = 5060;
#if defined(USE_SSL)
   int tlsPort = 5061;
#else
   int tlsPort = 0;
#endif
   int dtlsPort = 0;
   int disableV4 = false;
   int enableV6 = false;
   char* domains = 0;
   char* interfaces = 0;
   char* routeSet = 0;
#ifdef WIN32
   char certPathBuf[MAX_PATH];
#else
   char certPathBuf[256];
#endif
   char* certPath = certPathBuf;
#ifdef WIN32
   char dbPathBuf[MAX_PATH]={0};
   char* dbPath = dbPathBuf;
#else
   char* dbPath = 0;
#endif
   int noChallenge = false;
   int noAuthIntChallenge = false;
   int noWebChallenge = false;
   
   int noRegistrar = false;
   int noIdentityHeaders = false;
   int certServer = false;

   char* reqChainName = "default";
   char* mySqlServer = 0;
   int httpPort = 5080;
   int recursiveRedirect = 0;
   int doQValue=0;
   char* forkBehavior="EQUAL_Q_PARALLEL";
   bool cancelBetweenForkGroups=true;
   bool waitForTerminate=true;
   int msBetweenForkGroups=3000;//Moot by default
   int msBeforeCancel=3000;
   
   char* enumSuffix = 0;
   int allowBadReg = 0;
   int parallelForkStaticRoutes = 0;
   int NoLoadWebAdmin = 0;
   int showVersion = 0;
#ifdef WIN32
   int installService = 0;
   int removeService = 0;
#endif
   int timerC=180;
   int NoUseParameters = 0;
   char* adminPassword = "";


#ifdef WIN32
#ifndef HAVE_POPT_H
   noChallenge = 1;  // If no POPT, then default to no digest challenges
#endif
// when we have administrative right or run as LocalSystem for example as service
   static const Data AllUsersCerts(Data( getenv( "ALLUSERSPROFILE" ) ) + "\\Application Data\\Resiprocate\\repro\\sipCerts");
   static const Data AllUsersDb(Data( getenv( "ALLUSERSPROFILE" ) ) + "\\Application Data\\Resiprocate\\repro\\Db");
//when we run as restricted user
   static const Data LocalUserCerts(Data( getenv( "USERPROFILE" ) ) + "\\Local Settings\\Application Data\\Resiprocate\\repro\\sipCerts");
   static const Data LocalUserDb(Data( getenv( "USERPROFILE" ) ) + "\\Local Settings\\Application Data\\Resiprocate\\repro\\Db");

   if ( FileSystem::DirectoryExists( AllUsersCerts ) && FileSystem::IsReadWriteAccess ( AllUsersCerts.c_str() ) )
   {
      strcpy( certPath, AllUsersCerts.c_str() );
   }
   else if ( FileSystem::IsReadWriteAccess ( getenv( "ALLUSERSPROFILE" ) ) )
   {
      FileSystem::ForceDirectories( AllUsersCerts );
      FileSystem::SetAccessAs( AllUsersCerts.c_str() , getenv( "ALLUSERSPROFILE" ) );
      strcpy( certPath, AllUsersCerts.c_str() );
   }
   else if ( FileSystem::DirectoryExists( LocalUserCerts ) && FileSystem::IsReadWriteAccess ( LocalUserCerts.c_str() ) )
   {
      strcpy( certPath, LocalUserCerts.c_str() );
   }
   else if ( FileSystem::IsReadWriteAccess ( getenv( "USERPROFILE" ) ) )
   {
      FileSystem::ForceDirectories( LocalUserCerts );
      FileSystem::SetAccessAs( LocalUserCerts.c_str(), getenv( "USERPROFILE" ) );
      strcpy( certPath, LocalUserCerts.c_str() );
   }
   else // we must never get it
      strcpy(certPath,"C:\\sipCerts");   
   if ( FileSystem::DirectoryExists( AllUsersDb ) && FileSystem::IsReadWriteAccess ( AllUsersDb.c_str() ) )
   {
      strcpy( dbPath, AllUsersDb.c_str() );
   }
   else if ( FileSystem::IsReadWriteAccess ( getenv( "ALLUSERSPROFILE" ) ) )
   {
      FileSystem::ForceDirectories( AllUsersDb );
      FileSystem::SetAccessAs( AllUsersDb.c_str(), getenv( "ALLUSERSPROFILE" ) );
      strcpy( dbPath, AllUsersDb.c_str() );
   }
   else if ( FileSystem::DirectoryExists( LocalUserDb ) && FileSystem::IsReadWriteAccess ( LocalUserDb.c_str() ) )
   {
      strcpy( dbPath, LocalUserDb.c_str() );
   }
   else if ( FileSystem::IsReadWriteAccess ( getenv( "USERPROFILE" ) ) )
   {
      FileSystem::ForceDirectories( LocalUserDb );
      FileSystem::SetAccessAs( LocalUserDb.c_str(), getenv( "USERPROFILE" ) );
      strcpy( dbPath, LocalUserDb.c_str() );
   }
#else
   strcpy(certPath, getenv("HOME"));
   strcat(certPath, "/.sipCerts");
#endif

#ifdef HAVE_POPT_H
   struct poptOption table[] = {
      {"log-type",         'l',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &logType,        Parameters::prmLogType, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",        'v',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &logLevel,       Parameters::prmLogLevel, "specify the default log level", "STACK|DEBUG|INFO|WARNING|ALERT"},
      {"log-path",         0,  POPT_ARG_STRING,                            &logFilePath,      Parameters::prmLogPath, "specify the path for log file", 0},
      {"db-path",           0,   POPT_ARG_STRING,                            &dbPath,         Parameters::prmMax, "path to databases", 0},
      {"record-route",     'r',  POPT_ARG_STRING,                            &recordRoute,    Parameters::prmRecordRoute, "specify uri to use as Record-Route", "sip:example.com"},
#if defined(USE_MYSQL)
      {"mysqlServer",      'x',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &mySqlServer,    Parameters::prmMax, "enable MySQL and provide name of server", "localhost"},
#endif
      {"udp",                0,  POPT_ARG_INT| POPT_ARGFLAG_SHOW_DEFAULT,    &udpPort,        Parameters::prmUdp, "listen on UDP port", "5060"},
      {"tcp",                0,  POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT,   &tcpPort,        Parameters::prmTcp, "listen on TCP port", "5060"},
#if defined(USE_SSL)
      {"tls-domain",       't',  POPT_ARG_STRING,                            &tlsDomain,      Parameters::prmTlsDomain, "act as a TLS server for specified domain", "example.com"},
      {"tls",                0,  POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT,   &tlsPort,        Parameters::prmTls, "add TLS transport on specified port", "5061"},
      {"dtls",               0,  POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT,   &dtlsPort,       Parameters::prmDtls, "add DTLS transport on specified port", "0"},
      {"enable-cert-server", 0,  POPT_ARG_NONE,                              &certServer,     Parameters::prmEnableCertServer, "run a cert server", 0},
#ifdef WIN32
      {"cert-path",        'c',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &certPath,       Parameters::prmMax, "path to certificates (default: c:\\sipCerts)", 0},
#else
      {"cert-path",        'c',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &certPath,       Parameters::prmMax, "path to certificates (default: ~/.sipCerts)", 0},
#endif
#endif
      {"enable-v6",         0,   POPT_ARG_NONE,                              &enableV6,       Parameters::prmEnableV6, "enable IPV6", 0},
      {"disable-v4",        0,   POPT_ARG_NONE,                              &disableV4,      Parameters::prmDisableV4, "disable IPV4", 0},
      {"disable-auth",      0,   POPT_ARG_NONE,                              &noChallenge,    Parameters::prmDisableAuth, "disable DIGEST challenges", 0},
      {"disable-auth-int",  0,   POPT_ARG_NONE,                              &noAuthIntChallenge, Parameters::prmDisableAuthInt, "disable auth-int DIGEST challenges", 0},
      {"disable-web-auth",  0,   POPT_ARG_NONE,                              &noWebChallenge, Parameters::prmDisableWebAuth, "disable HTTP challenges", 0},
      {"disable-reg",       0,   POPT_ARG_NONE,                              &noRegistrar,    Parameters::prmDisableReg, "disable registrar", 0},
      {"disable-identity",  0,   POPT_ARG_NONE,                              &noIdentityHeaders, Parameters::prmDisableIdentity, "disable adding identity headers", 0},
      {"interfaces",      'i',   POPT_ARG_STRING,                            &interfaces,     Parameters::prmIinterfaces, "specify interfaces to add transports to", "sip:10.1.1.1:5065;transport=tls"},
      {"domains",         'd',   POPT_ARG_STRING,                            &domains,        Parameters::prmDomains, "specify domains that this proxy is authorative", "example.com,foo.com"},
      {"route",           'R',   POPT_ARG_STRING,                            &routeSet,       Parameters::prmRoute, "specify where to route requests that are in this proxy's domain", "sip:p1.example.com,sip:p2.example.com"},
      {"reqChainName",      0,   POPT_ARG_STRING,                            &reqChainName,   Parameters::prmReqChainName, "name of request chain (default: default)", 0},
      {"http",              0,   POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT,   &httpPort,       Parameters::prmHttp, "run HTTP server on specified port", "5080"},
      {"recursive-redirect",0,   POPT_ARG_NONE,                              &recursiveRedirect, Parameters::prmRecursiveRedirect, "Handle 3xx responses in the proxy", 0},
      {"q-value",           0,   POPT_ARG_NONE,                              &doQValue,       Parameters::prmQValue, "Enable sequential q-value processing", 0},
      {"q-value-behavior",  0,   POPT_ARG_STRING,                              &forkBehavior,   Parameters::prmQValueBehavior, "Specify forking behavior for q-value targets: FULL_SEQUENTIAL, EQUAL_Q_PARALLEL, or FULL_PARALLEL", 0},
      {"q-value-cancel-btw-fork-groups",0,POPT_ARG_NONE,                     &cancelBetweenForkGroups, Parameters::prmQValueCancelBtwForkGroups, "Whether to cancel groups of parallel forks after the period specified by the --q-value-ms-before-cancel parameter.", 0},
      {"q-value-wait-for-terminate-btw-fork-groups",0,POPT_ARG_NONE,         &waitForTerminate, Parameters::prmQValueWaitForTerminateBtwForkGroups, "Whether to wait for parallel fork groups to terminate before starting new fork-groups.", 0},
      {"q-value-ms-between-fork-groups",0,POPT_ARG_INT,                      &msBetweenForkGroups, Parameters::prmQValueMsBetweenForkGroups, "msec to wait before starting new groups of parallel forks", 0},
      {"q-value-ms-before-cancel",0,   POPT_ARG_INT,                         &msBeforeCancel, Parameters::prmQValueMsBeforeCancel, "msec to wait before cancelling parallel fork groups", 0},
      {"enum-suffix",     'e',   POPT_ARG_STRING,                            &enumSuffix,     Parameters::prmEnumSuffix, "specify enum suffix to search", "e164.arpa"},
      {"allow-bad-reg",   'b',   POPT_ARG_NONE,                              &allowBadReg,    Parameters::prmAllowBadReg, "allow To tag in registrations", 0},
      {"parallel-fork-static-routes",'p',POPT_ARG_NONE,                      &parallelForkStaticRoutes, Parameters::prmParallelForkStaticRoutes, "paralled fork to all matching static routes and (first batch) registrations", 0},
      {"timer-C",         0,     POPT_ARG_INT,                               &timerC,         Parameters::prmTimerC, "specify length of timer C in sec (0 or negative will disable timer C)", "180"},
      {"admin-password",  'a',   POPT_ARG_STRING,                            &adminPassword,  Parameters::prmAdminPassword, "set web administrator password", ""},
      {"no-use-parameters",  0,   POPT_ARG_NONE,                            &NoUseParameters,  Parameters::prmMax, "set web administrator password", ""},
      {"no-load-we-admin",  0,   POPT_ARG_NONE,                              &NoLoadWebAdmin,  Parameters::prmMax, "do not load web admin server", ""},
      {"version",         'V',   POPT_ARG_NONE,                              &showVersion,    Parameters::prmMax, "show the version number and exit", 0},
#ifdef WIN32
      {"install-service", 0,   POPT_ARG_NONE,                                &installService,    Parameters::prmMax, "install program as WinNT service", 0},
      {"remove-service",  0,   POPT_ARG_NONE,                                &removeService,     Parameters::prmMax, "remove program from WinNT service list", 0},
#endif
      POPT_AUTOHELP 
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   int prm;
   while ( (prm = poptGetNextOpt(context) ) != -1)
   {
      if ( prm < -1 )
      {
         cerr << "Bad command line argument entered" << endl;
         poptPrintHelp(context, stderr, 0);
         exit(-1);
      }
      if ( prm != Parameters::prmMax )
         Parameters::DisableParam( (Parameters::Param)prm );
   }
#endif

   mHttpPort = httpPort;
   mLogType = logType;
   mLogLevel = logLevel;
   mLogFilePath = logFilePath;

   if (showVersion)
   {
     cout << repro::VersionUtils::instance().displayVersion() << endl;
     exit(0);
   }
   
   if (tlsDomain) 
   {
      mTlsDomain = tlsDomain;
   }

   mShouldRecordRoute = false;
   if (recordRoute) 
   {
      mShouldRecordRoute = true;
      mRecordRoute = toUri(recordRoute, "Record-Route");
   }
   
   mUdpPort = udpPort;
   mTcpPort = tcpPort;
   mTlsPort = tlsPort;
   mDtlsPort = dtlsPort;
   mUseV4 = !disableV4;
   mUseV6 = enableV6?true:false;
   mInterfaces = toVector(interfaces, "interfaces"); 
   mDomains = toVector(domains, "domains"); 
   mRouteSet = toVector(routeSet, "routeSet"); 
   mCertPath = certPath;
   mNoChallenge = noChallenge != 0;
   mNoAuthIntChallenge = noAuthIntChallenge != 0;
   mNoWebChallenge = noWebChallenge != 0;
   mNoRegistrar = noRegistrar != 0 ;
   mNoIdentityHeaders = noIdentityHeaders != 0;
   mCertServer = certServer !=0 ;
   mRequestProcessorChainName=reqChainName;
   mRecursiveRedirect = recursiveRedirect?true:false;
   mDoQValue = doQValue?true:false;
   mForkBehavior=forkBehavior;
   mCancelBetweenForkGroups=cancelBetweenForkGroups?true:false;
   mWaitForTerminate=waitForTerminate?true:false;
   mMsBetweenForkGroups=msBetweenForkGroups;
   mMsBeforeCancel=msBeforeCancel;
   mAllowBadReg = allowBadReg?true:false;
   mParallelForkStaticRoutes = parallelForkStaticRoutes?true:false;
   mNoUseParameters = NoUseParameters != 0;
   mNoLoadWebAdmin = NoLoadWebAdmin != 0;

   if (enumSuffix) mEnumSuffix = enumSuffix;
   
   if (mySqlServer) 
   {
      mMySqlServer = Data(mySqlServer);
   }

   if (dbPath)
   {
      mDbPath = Data(dbPath);
   }
   
   if(timerC >0)
   {
      mTimerC=timerC;
   }
   else
   {
      mTimerC=0;
   }

   mAdminPassword = adminPassword;
#ifdef WIN32
   mInstallService=installService != 0;
   mRemoveService=removeService != 0;
#endif

#ifdef HAVE_POPT_H
   poptFreeContext(context);
#endif
}

resip::Uri 
CommandLineParser::toUri(const char* input, const char* description)
{
   resip::Uri uri;
   try
   {
      if (input)
      {
         uri = Uri(input);
      }
      else
      {
         std::cerr << "No " << description << " specified" << std::endl;
      }
   } 
   catch (ParseException& e)
   {
      std::cerr << "Caught: " << e << std::endl;
      std::cerr << "Can't parse " << description << " : " << input << std::endl;
      exit(-1);
   }
   return uri;
}

std::vector<resip::Data> 
CommandLineParser::toVector(const char* input, const char* description)
{
   std::vector<Data> domains; 

   if (input)
   {
      Data buffer = input;
      if (input)
      {
         for (char* token = strtok(const_cast<char*>(buffer.c_str()), ","); token != 0; token = strtok(0, ","))
         {
            try
            {
               domains.push_back(token);
            } 
            catch (ParseException& e)
            {
               std::cout << "Caught: " << e << std::endl;
               std::cerr << "Can't parse " << description << " : " << token << std::endl;
               exit(-1);
            }
            catch (...)
            {
               std::cout << "Caught some exception" <<std::endl;
               std::cerr << "Some problem parsing " << description << " : " << token << std::endl;
            }
         }
      }
   }
   return domains;
}
   
