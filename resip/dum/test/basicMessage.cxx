// made by Andrea Ghittino - andrea.ghittino@csp.it

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/Profile.hxx"
#include "resip/dum/UserProfile.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/ClientPagerMessage.hxx"
#include "resip/dum/ServerPagerMessage.hxx"

#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialog.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/PagerMessageHandler.hxx"
#include "resip/stack/PlainContents.hxx"

#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class RegListener : public ClientRegistrationHandler {
public:
	RegListener() : _registered(false) {};
	bool isRegistered() { return _registered; };

	virtual void onSuccess(ClientRegistrationHandle, const SipMessage& response)
    {
    	cout << "client registered\n";
	    _registered = true;
    }
	virtual void onRemoved(ClientRegistrationHandle, const SipMessage& response)
    {
    	cout << "client regListener::onRemoved\n";
	    exit(-1);
    }
	virtual void onFailure(ClientRegistrationHandle, const SipMessage& response)
    {
    	cout << "client regListener::onFailure\n";
	    exit(-1);
    }
    virtual int onRequestRetry(ClientRegistrationHandle, int retrySeconds, const SipMessage& response)
    {
    	cout << "client regListener::onRequestRetry\n";
	    exit(-1);
	return -1;
    }

protected:
	bool _registered;

};

class ClientMessageHandler : public ClientPagerMessageHandler {
public:
	ClientMessageHandler() : _ended(false) {};
	virtual void onSuccess(ClientPagerMessageHandle, const SipMessage& status)
    {
	    InfoLog(<<"ClientMessageHandler::onSuccess\n");
	    _ended = true;
    }
	virtual void onFailure(ClientPagerMessageHandle, const SipMessage& status, std::auto_ptr<Contents> contents)
    {
    	InfoLog(<<"ClientMessageHandler::onFailure\n");
	    _ended = true;
    }
	bool isEnded() { return _ended; };
private:	
	bool _ended;
};

class ServerMessageHandler : public ServerPagerMessageHandler
{
public:
	ServerMessageHandler() : _rcvd(false) {};
	bool isRcvd() { return _rcvd; };
	virtual void onMessageArrived(ServerPagerMessageHandle handle, const SipMessage& message)
    {
    	//cout << "Message rcv: "  << message << "\n";
	
	    SharedPtr<SipMessage> ok = handle->accept();
	    handle->send(ok);

	    Contents *body = message.getContents();
	    cout << "Message rcv: "  << *body << "\n";

    	_rcvd = true;
    }
private:
	bool _rcvd;
};


/*****************************************************************************/

int main(int argc, char *argv[]) {
	if( (argc < 6) || (argc > 7) ) {
		cout << "usage: " << argv[0] << " sip:from user passwd realm sip:to [port]\n";
		return 0;
	}


	Log::initialize(Log::Cout, Log::Info, argv[0]);

	bool first = true;
	string from(argv[1]);
	string user(argv[2]);
	string passwd(argv[3]);
	string realm(argv[4]);
	string to(argv[5]);
	int port = 5060;
	if(argc == 7) {
		string temp(argv[6]);
		istringstream src(temp);
		src >> port;
	}

	InfoLog(<< "log: from: " << from << ", to: " << to << ", port: " << port << "\n");
	InfoLog(<< "user: " << user << ", passwd: " << passwd << ", realm: " << realm << "\n");
	
	// sip logic
	RegListener client;
	SharedPtr<MasterProfile> profile(new MasterProfile);   
	auto_ptr<ClientAuthManager> clientAuth(new ClientAuthManager());   

    SipStack clientStack;
	DialogUsageManager clientDum(clientStack);
	clientStack.addTransport(UDP, port);
	clientDum.setMasterProfile(profile);

	clientDum.setClientRegistrationHandler(&client);
	clientDum.setClientAuthManager(clientAuth);
	clientDum.getMasterProfile()->setDefaultRegistrationTime(70);		
	clientDum.getMasterProfile()->addSupportedMethod(MESSAGE);
	clientDum.getMasterProfile()->addSupportedMimeType(MESSAGE, Mime("text", "plain"));
	ClientMessageHandler *cmh = new ClientMessageHandler();
	ServerMessageHandler *smh = new ServerMessageHandler();
	clientDum.setClientPagerMessageHandler(cmh);
	clientDum.setServerPagerMessageHandler(smh);

	/////
	NameAddr naFrom(from.c_str());
	profile->setDefaultFrom(naFrom);
	profile->setDigestCredential(realm.c_str(), user.c_str(), passwd.c_str());
	
	SharedPtr<SipMessage> regMessage = clientDum.makeRegistration(naFrom);
	
	InfoLog( << *regMessage << "Generated register: " << endl << *regMessage );
	clientDum.send( regMessage );

	while(true) // (!cmh->isEnded() || !smh->isRcvd() )

	{
		clientStack.process(100);
        while(clientDum.process());
		//if (!(n++ % 10)) cerr << "|/-\\"[(n/10)%4] << '\b';
		
		if(first && client.isRegistered()) {
			first = false;
			InfoLog(<<"client registered!!\n");
			InfoLog(<< "Sending MESSAGE\n");
			NameAddr naTo(to.c_str());
			ClientPagerMessageHandle cpmh = clientDum.makePagerMessage(naTo);
			
			auto_ptr<Contents> content(new PlainContents(Data("my first message!")));
			cpmh.get()->page(content); 
		}
	}   

	return 0;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
