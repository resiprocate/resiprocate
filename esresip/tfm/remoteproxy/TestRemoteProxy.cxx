#include "tfm/remoteproxy/TestRemoteProxy.hxx"

TestRemoteProxy::TestRemoteProxy(const resip::Data& name,
                                  const resip::Data& host, 
                                    const std::set<int>& udpPorts, 
                                    const std::set<int>& tcpPorts, 
                                    const std::set<int>& tlsPorts, 
                                    const std::set<int>& dtlsPorts,  
                                  const resip::Data& nwInterface,
                                  resip::Security* security):
TestProxy(name, host, udpPorts,tcpPorts,tlsPorts,dtlsPorts, nwInterface)
{
   std::cerr << "Verify hostname is " << host << std::endl;
//   std::cerr << "Press enter to continue";
}

TestRemoteProxy::~TestRemoteProxy()
{

}

void 
TestRemoteProxy::addUser(const resip::Data& userid, const resip::Uri& aor, const resip::Data& password, bool provideFailover)
{
   std::cerr << "Provision user " << userid << " aor= " << aor << " passwd=" << password << std::endl;
//   std::cerr << "Press enter to continue.";
}

/* Copyright 2007 Estacado Systems */
