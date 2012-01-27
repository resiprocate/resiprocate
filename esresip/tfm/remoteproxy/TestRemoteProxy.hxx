#ifndef TEST_REMOTE_PROXY_HXX
#define TEST_REMOTE_PROXY_HXX 1

#include "tfm/TestProxy.hxx"
#include "tfm/remoteproxy/CommandLineParser.hxx"

#include "rutil/Data.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/ssl/Security.hxx"

class TestRemoteProxy : public TestProxy
{
   public:
      TestRemoteProxy(const resip::Data& name,
                const resip::Data& host, 
                  const std::set<int>& udpPorts, 
                  const std::set<int>& tcpPorts, 
                  const std::set<int>& tlsPorts, 
                  const std::set<int>& dtlsPorts,  
                const resip::Data& nwInterface = resip::Data::Empty,
                resip::Security* security=0);

      ~TestRemoteProxy();

      virtual void addUser(const resip::Data& userid, const resip::Uri& aor, const resip::Data& password, bool provideFailover=false);
      virtual void deleteUser(const resip::Data& userid, const resip::Uri& aor){};
      virtual void deleteBindings(const resip::Uri& aor){};
      virtual void addRoute(const resip::Data& matchingPattern,
                            const resip::Data& rewriteExpression, 
                            const resip::Data& method,
                            const resip::Data& event,
                            int priority,
                            int weight){};
      virtual void deleteRoute(const resip::Data& matchingPattern, 
                               const resip::Data& method, 
                               const resip::Data& event){};
            
   private:

};

#endif

/* Copyright 2007 Estacado Systems */
