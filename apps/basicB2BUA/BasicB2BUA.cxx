
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>

#include "resip/stack/Helper.hxx"
#include "resip/stack/SERNonceHelper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/InMemoryRegistrationDatabase.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/ServerAuthManager.hxx"
#include "rutil/Log.hxx"

//#include "DummyRegistrationPersistenceManager.hxx"
#include "b2bua/DummyServerRegistrationHandler.hxx"
#include "b2bua/Logging.hxx"
#include "b2bua/RtpProxyRecurringTask.hxx"

#include "MyServerAuthManager.hxx"
#include "BasicConfiguration.hxx"
#include "BasicB2BUA.hxx"
#include "BasicManager.hxx"

using namespace b2bua;
using namespace resip;
using namespace std;

#define PID_FILE "/var/run/basicB2BUA.pid"

static BasicB2BUA *_b2bua = NULL;

BasicB2BUA::BasicB2BUA(char *progname, const char *configfile, char *localIp, int sipPort, char *rtpProxySocket, char *rtpProxyTimeoutSocket, char *challengeDomain, bool log_stderr, bool dummyRegistration, const resip::Data& nonEncodedChars, ostream& cdrStream) : B2BUA(NULL, *(new BasicCDRHandler(cdrStream))), mBasicConfiguration(configfile), basicManager(mBasicConfiguration) {

  RtpProxyUtil::setSocket(rtpProxySocket);
  RtpProxyUtil::setTimeoutSocket(rtpProxyTimeoutSocket);
  RtpProxyUtil::init();

  uasMasterProfile.get()->setFixedTransportInterface(localIp);
  uasMasterProfile.get()->setFixedTransportPort(sipPort);

  setAuthorizationManager(&basicManager);

  // Tell the DUM stack to recognise our domains
  dialogUsageManager->addDomain(challengeDomain);
  dialogUsageManager->addDomain(localIp);

  // Set up authentication when we act as UAS
  SharedPtr<ServerAuthManager> serverAuth(new MyServerAuthManager(*dialogUsageManager, mBasicConfiguration));
  dialogUsageManager->setServerAuthManager(serverAuth);

  // do simulated registration for phones that insist on registering 
  // before making a call
  if(dummyRegistration) {
    DummyServerRegistrationHandler *rh = new DummyServerRegistrationHandler();
    dialogUsageManager->setServerRegistrationHandler(rh);
    //DummyRegistrationPersistenceManager *rpm = new DummyRegistrationPersistenceManager();
    dialogUsageManager->setRegistrationPersistenceManager(new InMemoryRegistrationDatabase());
    uasMasterProfile.get()->addSupportedMethod(REGISTER);
  }

  SERNonceHelper *nonceHelper = new SERNonceHelper(300);
  nonceHelper->setPrivateKey("J7ht7eoPKUangcoDgP73nFa7xM8hxXytbnAYLVqq");
  Helper::setNonceHelper(nonceHelper);

  // Specify the transport we would like to use
  dialogUsageManager->addTransport(UDP, sipPort, V4, localIp);

  MediaManager::setProxyAddress(localIp);
  
  // For compatibility with Cisco and Nextone
  Uri::setUriUserEncoding('#', false);  // FIXME - provide config option

  RtpProxyUtil::init();
  taskManager->addRecurringTask(new RtpProxyRecurringTask());
}

BasicB2BUA::~BasicB2BUA() {
}

void BasicB2BUA::loadDests() {
   mBasicConfiguration.setReloadRequired(true);
}

int run_b2bua(char *progname, char *configfile, char *localIp, int sipPort, char *rtpProxySocket, char *rtpProxyTimeoutSocket, char *challengeDomain, bool log_stderr, bool dummyRegistration, const resip::Data& nonEncodedChars) {
  B2BUA_LOG_NOTICE("initialising...");

  // Install signal handlers
  (void)signal(SIGUSR1, handle_signal);
  (void)signal(SIGUSR2, handle_signal);
  (void)signal(SIGTERM, handle_signal);

  // open CDR stream
  ofstream cdrStream;
  cdrStream.open("/var/log/mycdr.csv", ios::out | ios::app);
  if(!cdrStream.is_open()) {
    B2BUA_LOG_ERR("Failed to open CDR file");
    throw;
  }

  // start B2BUA
  _b2bua = new BasicB2BUA(progname, configfile, localIp, sipPort, rtpProxySocket, rtpProxyTimeoutSocket, challengeDomain, log_stderr, dummyRegistration, nonEncodedChars, cdrStream);
  B2BUA_LOG_NOTICE("starting...");
  _b2bua->run();
  return 0;
}

int main(int argc, char *argv[]) {
  char *progname = argv[0];
  char *configfile = argv[1];
  char *localIp = argv[2];
  char *sipPort = argv[3];
  char *challengeDomain = argv[4];
  char *rtpProxySocket = argv[5];
  char *rtpProxyTimeoutSocket = argv[6];

  // Parse command line
  bool do_fork = false;
  //bool do_fork = true;
  bool log_stderr = false;
  bool dummyRegistration = true;
  Data nonEncodedChars("#");

  char *t;
  if ((t = strrchr(argv[0], '/')))
    progname = t+1;

  // Setup logging

  /* int log_opts = LOG_NDELAY | LOG_PID;
  if(log_stderr)
    log_opts |= LOG_PERROR;
  int log_facility = LOG_LOCAL0;
  openlog(progname, log_opts, log_facility); */
  // resip logging
  Log::initialize(Log::Syslog, Log::Debug, progname, NULL, NULL);
  B2BUA_LOG_INIT(progname);

  if(argc != 7) {
    B2BUA_LOG_CRIT("wrong number of command line arguments, aborting.");
    cerr << "Usage:" << endl;
    cerr << " " << progname << " <config file> <local IP> <sip port> <challenge domain> <rtpproxy socket> <rtpproxy timeout socket>" << endl;
    return -1;
  }

  if(!do_fork)
    return run_b2bua(progname, configfile, localIp, atoi(sipPort), rtpProxySocket, rtpProxyTimeoutSocket, challengeDomain, log_stderr, dummyRegistration, Data(nonEncodedChars));

  if(daemon(1, 1) == -1) {
    B2BUA_LOG_ERR("daemon failed");
    return -1;
  }

  // Redirect stdin, stdout and stderr
  char *my_stdout = "/var/log/basicb2bua.out";
  char *my_stderr = "/var/log/basicb2bua.out";
  if((freopen("/dev/null", "wt", stdin) == NULL) ||
    (freopen(my_stdout, "wt", stdout) == NULL) ||
    (freopen(my_stderr, "wt", stderr) == NULL)) {
    B2BUA_LOG_ERR("error calling freopen");
    return -1;
  }

  while(true) {
    pid_t child_pid = 0;
    child_pid = fork();
    if(child_pid == 0) {
      return run_b2bua(progname, configfile, localIp, atoi(sipPort), rtpProxySocket, rtpProxyTimeoutSocket, challengeDomain, log_stderr, dummyRegistration, Data(nonEncodedChars));
    } else {
      // Create PID file
      FILE *f;
      if((f = fopen(PID_FILE, "wt")) != NULL) {
        // FIXME - should we use child or parent PID here?
        //fprintf(f, "%d", getpid());
        fprintf(f, "%d", child_pid);
        fclose(f);
      } else {
        B2BUA_LOG_ERR("error creating PID file %s", PID_FILE);
      }
      int status;
      wait(&status);
      unlink(PID_FILE);
      if(WIFEXITED(status)) {
        B2BUA_LOG_INFO("exited normally, finishing");
        return 0;
      } 
      B2BUA_LOG_WARNING("exited abnormally, starting again"); 
      // rotate core files
      char buf[200];
      time_t t;
      time(&t);
      sprintf(buf, "core.%ld", t);
      rename("core", buf);
      sleep(1);
    }
  }
}

/* handles signals */
void handle_signal(int signum) {
  B2BUA_LOG_INFO("received signal %d", signum);
  switch(signum) {
  case SIGUSR1: // log stats
    _b2bua->logStats(); 
    break;
  case SIGUSR2: // reload dests
    _b2bua->loadDests();
    break;
  case SIGTERM:
    _b2bua->stop();
    break;
  default:
    B2BUA_LOG_INFO("signal %d not handled", signum);
  }
}


BasicCDRHandler::BasicCDRHandler(std::ostream& cdrStream) :
  mCdrStream(cdrStream)   {
}

void BasicCDRHandler::handleRecord(const std::string& record) {
  // Now write to file
  mCdrStream << record << std::endl;
  mCdrStream.flush();
}

