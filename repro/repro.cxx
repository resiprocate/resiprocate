#include <signal.h>

#include "repro/ProxyMainException.hxx"

#include "resip/stack/Transport.hxx"

#include "repro/ProxyMain.hxx"
#include "repro/CommandLineParser.hxx"
#include "repro/Store.hxx"
#include "repro/UserStore.hxx"
#include "repro/ConfigStore.hxx"
#include "repro/AclStore.hxx"
#include "repro/RouteStore.hxx"
#include "rutil/FileSystem.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "repro/AbstractDb.hxx"
#include "repro/BerkeleyDb.hxx"

#ifdef WIN32
#include "rutil/Win32EventLog.hxx"
#endif


#include "repro/Parameters.hxx"

#if defined(USE_MYSQL)
#include "repro/MySqlDb.hxx"
#endif
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

using namespace repro;
using namespace resip;
using namespace std;

#ifdef WIN32
static const char* ReproServiceName="ReproService";
#endif


static void
signalHandler(int signo)
{
   std::cerr << "Shutting down" << endl;
   reproFinish = true;
}


#ifdef WIN32

static DWORD WINAPI 
ReproSvcHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
   SvcStat.dwWaitHint = ReproStateNum;
   if ( dwControl == SERVICE_CONTROL_INTERROGATE )
   {
      switch ( ReproState )
      {
      case reproStarting:
         SvcStat.dwCurrentState = SERVICE_START_PENDING;
         break;
      case reproWorking:
         SvcStat.dwCurrentState = SERVICE_RUNNING;
         break;
      case reproFinishing:
         SvcStat.dwCurrentState = SERVICE_STOP_PENDING;
         break;
      case reproEnd:
         SvcStat.dwCurrentState = SERVICE_STOPPED;
         break;
      }
      SetServiceStatus( svcH, &SvcStat);
      return NO_ERROR;
   }
   else if ( dwControl == SERVICE_CONTROL_STOP )
   {
      reproFinish = true;
      while ( ReproState != reproEnd )
         Sleep( 1000 );
//      ReproState = reproFinishing;
      SvcStat.dwCurrentState = SERVICE_STOPPED;
      SetServiceStatus( svcH, &SvcStat);
      return NO_ERROR;
   }
   switch ( ReproState )
   {
   case reproStarting:
      SvcStat.dwCurrentState = SERVICE_START_PENDING;
      break;
   case reproWorking:
      SvcStat.dwCurrentState = SERVICE_RUNNING;
      break;
   case reproFinishing:
      SvcStat.dwCurrentState = SERVICE_STOP_PENDING;
      break;
   case reproEnd:
      SvcStat.dwCurrentState = SERVICE_STOPPED;
      break;
   }
   SetServiceStatus( svcH, &SvcStat);
   return ERROR_CALL_NOT_IMPLEMENTED;
}

int runRepro();

static void 
WINAPI ReproServiceMain(DWORD dwArgc,LPTSTR* lpszArgv)
{
//   DebugBreak();
   ReproState = reproStarting; 
   SetSvcStat();
   svcH = RegisterServiceCtrlHandlerEx( ReproServiceName, &ReproSvcHandlerEx, &SvcStat);
   if ( svcH ) 
   {
      SvcStat.dwWin32ExitCode = NO_ERROR;
      SetServiceStatus(svcH,&SvcStat);
      if ( int ret = runRepro() )
      {
         SvcStat.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
         SvcStat.dwServiceSpecificExitCode = ret;
      }
      SetSvcStat();
   }
}

static bool 
installService(int argc, char** argv)
{
   SC_HANDLE sch = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS );
   if ( !sch )
   {
      return false;
   }
   size_t CmdLineSize = 0;
   for ( int i=0; i < argc; i++)
      CmdLineSize+=strlen( argv[i])+1;
   char *CmdLine = (char *)malloc( CmdLineSize + 1);
   *CmdLine=0;
   if ( strchr(argv[0] , (int) " " ) )
   {
      strcpy(CmdLine,"\"");
      strcat(CmdLine,argv[0]);
      strcat(CmdLine,"\"");
   }
   else
   {
      strcpy(CmdLine,argv[0]);
   }
   for ( int i=1; i < argc; i++)
      if ( stricmp( argv[i], "--install-service") )
      {
         strcat( CmdLine, " " );
         strcat( CmdLine, argv[i] );
      }
   SC_HANDLE svch=CreateService( sch, ReproServiceName, "Repro sip proxy server", SERVICE_ALL_ACCESS, 
           SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_IGNORE, CmdLine,
           NULL, NULL, NULL, NULL, NULL);
   if ( !svch )
   {
      CloseServiceHandle(sch);
      return false;
   }
   CloseServiceHandle(svch);
   CloseServiceHandle(sch);

   HKEY hk; 
   DWORD dwData; 
   char *BaseKey="SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
   char *Key=(char *)malloc( strlen( BaseKey ) + strlen( ReproServiceName ) + 1 );
   strcpy( Key, BaseKey );
   strcat( Key, ReproServiceName );
   RegCreateKey(HKEY_LOCAL_MACHINE, Key, &hk);
   RegSetValueEx(hk, "EventMessageFile", 0, REG_EXPAND_SZ, (LPBYTE) argv[0], strlen( argv[0] ) + 1);
   dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE; 
   RegSetValueEx(hk,"TypesSupported",0,REG_DWORD,(LPBYTE) &dwData,sizeof(DWORD));
   RegCloseKey(hk); 

   return true;
}

static bool 
removeService()
{
   SC_HANDLE sch = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS );
   if ( !sch )
   {
      return false;
   }
   SC_HANDLE svch = OpenService( sch, ReproServiceName, SERVICE_ALL_ACCESS);
   if ( !svch )
   {
      return false;
   }
   SERVICE_STATUS svcstat;
   if ( ControlService( svch, SERVICE_CONTROL_STOP, &svcstat ) )
   {
      int i=0;
      while ( i<=20 && svcstat.dwCurrentState != SERVICE_STOPPED )
      {
         Sleep(1000);
         QueryServiceStatus( svch, &svcstat );
      }
      if ( svcstat.dwCurrentState != SERVICE_STOPPED )
         cerr << "Unable to stop service, service will be deleted after reboot\n";
   }   
   DeleteService( svch );
   CloseServiceHandle( svch );
   CloseServiceHandle( sch );
   return true;
}

#endif

CommandLineParser *args;
char *argv0;

static int
runRepro()
{
   if(args->mLogType.lowercase() == "file")
   {
      Log::initialize("file", args->mLogLevel, argv0, (args->mLogFilePath+FileSystem::PathSeparator+"repro_log.txt").c_str());
   }
   else
   {
      Log::initialize(args->mLogType, args->mLogLevel, argv0, NULL );
   }
   AbstractDb *db=NULL;
#ifdef USE_MYSQL
   if ( !args->mMySqlServer.empty() )
   {
      db = new MySqlDb(args->mMySqlServer);
   }
#endif
   if (!db)
   {
      try // at least under Windows BerkleyDb throw exception when can't open database
      {      
         db = new BerkeleyDb(args->mDbPath);
      }
      catch(...)
      {
         CritLog( <<"Exception while creating BerkleyDb database" );
         return -1;
      }
      if (!static_cast<BerkeleyDb*>(db)->isSane())
      {
         CritLog( <<"Failed to open configuration database" );
         return -1;
      }
   }
   if ( !db )
   {
      CritLog( <<"Failed to open configuration database" );
      return -1;
   }
   auto_ptr<AbstractDb> dbGuard(db);

   Parameters::SetDb( db );
   Store store(new UserStore(*db), new RouteStore(*db), new AclStore(*db), new ConfigStore(*db) );
   do
   {
      reproRestartServer = false;
      if ( !args->mNoUseParameters )
         Parameters::StoreParametersInArgs( args );
      ExternalLogger *externalLogger = NULL;
#ifdef WIN32
      if ( ReproWin32Service )
         externalLogger = new Win32EventLog( ReproServiceName );
#endif
      if(args->mLogType.lowercase() == "file")
      {
         Log::initialize("file", args->mLogLevel, argv0, (args->mLogFilePath+FileSystem::PathSeparator+"repro_log.txt").c_str(), externalLogger );
      }
      else
      {
         Log::initialize(args->mLogType, args->mLogLevel, argv0, NULL, externalLogger );
      }
#ifdef WIN32
      if ( ReproWin32Service )
         NoticeLog (<< "Starting service " << ReproServiceName );
#endif
      try
      {
         ProxyMain( args, store );
      }
      catch ( ProxyMainException& e)
      {
         CritLog (<< "Caught: " << e);
         return -1;
      }
      catch ( Transport::Exception& e )
      {
         std::cerr << "Likely a port is already in use" << endl;
         CritLog (<< "Caught: " << e);
         return -1;
      }
      catch ( resip::BaseException& e )
      {
         CritLog (<< "Caught: " << e);
         return -1;
      }
      catch ( ... )
      {
         CritLog (<< "Caught unknown exception" );
         return -1;
      }
#ifdef WIN32
      if ( ReproWin32Service )
         NoticeLog (<< "Service " << ReproServiceName << " stopped" );
#endif
      if ( externalLogger )
         delete externalLogger;
   }
   while ( reproRestartServer );

   return 0;
}

int
main(int argc, char** argv)
{
#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }
#endif


   if ( signal( SIGINT, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGINT" << endl;
      exit( -1 );
   }

   if ( signal( SIGTERM, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGTERM" << endl;
      exit( -1 );
   }

   /* Initialize a stack */
   args = new CommandLineParser(argc, argv);
   std::auto_ptr<CommandLineParser> args_ptr(args);

#ifdef WIN32
   if ( args->mInstallService && args->mRemoveService )
   {
      cerr << "You cannot install and remove service together\n" ;
      exit(-1);
   }
   if ( args->mInstallService )
   {
      if ( installService( argc, argv ) )
      {
         cerr << "Service installed\n";
         exit(0);
      }
      else
      {
         cerr << "Failed to install service\n";
         exit(-1);
      }
   }
   if ( args->mRemoveService )
   {
      if ( removeService() )
      {
         cerr << "Service removed\n";
         exit(0);
      }
      else
      {
         cerr << "Failed to remove service\n";
         exit(0);  // We need exit(0) because of Windows uninstall procedure
      }
   }
#endif




#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   { FindMemoryLeaks fml;
#endif

   argv0 = argv[0];
#ifndef WIN32
   return runRepro();
#else
   SERVICE_TABLE_ENTRY SvcTE[]=
   {
      { (LPSTR)ReproServiceName, ReproServiceMain },
      {NULL,                     NULL}
   };
   if ( !StartServiceCtrlDispatcher( SvcTE ) )
   {
      if ( GetLastError() == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT ) //Ok we run repro in console
      {
         ReproWin32Service = false;
         return runRepro();
      }
      else
         return -1;
   }

#endif

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   }
#endif
}
