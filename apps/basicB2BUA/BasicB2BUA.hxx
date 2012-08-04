
#ifndef __CCB2BUA_h
#define __CCB2BUA_h

#include "rutil/Data.hxx"

#include "b2bua/B2BUA.hxx"
#include "BasicManager.hxx"

class BasicCDRHandler : public b2bua::CDRHandler {
public:
  BasicCDRHandler(std::ostream& cdrStream);
  virtual ~BasicCDRHandler() {};

  virtual void handleRecord(const std::string& record);
private:
  std::ostream& mCdrStream;
};

class BasicB2BUA : public b2bua::B2BUA {

protected:
  BasicConfiguration mBasicConfiguration;
  BasicManager basicManager;

public:
  BasicB2BUA(char *progname, const char *configfile, char *localIp, int sipPort, char *rtpProxySocket, char *rtpProxyTimeoutSocket, char *challengeDomain, bool log_stderr, bool dummyRegistration, const resip::Data& nonEncodedChars, std::ostream& cdrStream);

  virtual ~BasicB2BUA();
  
  void loadDests();

};

int run_b2bua(char *progname, char *configfile, char *localIp, int sipPort, char *rtpProxySocket, char *rtpProxyTimeoutSocket, char *challengeDomain, bool log_stderr, bool dummyRegistration, const resip::Data& nonEncodedChars);
void handle_signal(int signum);

#endif
