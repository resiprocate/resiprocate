
#include "KurentoManager.hxx"
#include "KurentoConnection.hxx"

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> client;

using namespace kurento;

KurentoManager::KurentoManager(unsigned int timeout)
   : mTimeout(timeout)
{
   mWSClient.init_asio(&mAsio);
}

KurentoManager::~KurentoManager()
{

}

void
KurentoManager::process()
{
   mAsio.poll();
}

KurentoConnection::ptr
KurentoManager::getKurentoConnection(const std::string& uri)
{
   // Register our message handler
   //mWSClient.set_message_handler(bind(&on_message,&c,::_1,::_2));

   websocketpp::lib::error_code ec;
   client::connection_ptr con = mWSClient.get_connection(uri, ec);
   if (ec) {
       std::cout << "could not create connection because: " << ec.message() << std::endl;
       throw std::runtime_error(ec.message()); // FIXME
   }

   KurentoConnection::ptr kConnection = websocketpp::lib::make_shared<KurentoConnection>(mWSClient, con->get_handle(), uri, mTimeout);

   con->set_open_handler(websocketpp::lib::bind(
            &KurentoConnection::onOpen,
            kConnection,
            &mWSClient,
            websocketpp::lib::placeholders::_1
   ));
   con->set_fail_handler(websocketpp::lib::bind(
            &KurentoConnection::onFail,
            kConnection,
            &mWSClient,
            websocketpp::lib::placeholders::_1
   ));
   con->set_close_handler(websocketpp::lib::bind(
            &KurentoConnection::onClose,
            kConnection,
            &mWSClient,
            websocketpp::lib::placeholders::_1
   ));
   con->set_message_handler(websocketpp::lib::bind(
            &KurentoConnection::onMessage,
            kConnection,
            &mWSClient,
            websocketpp::lib::placeholders::_1,
            websocketpp::lib::placeholders::_2
    ));

   // Note that connect here only requests a connection. No network messages are
   // exchanged until the event loop starts running in the next line.
   mWSClient.connect(con);

   return kConnection;

}


/* ====================================================================

 Copyright (c) 2021, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2021, Daniel Pocock https://danielpocock.com
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of Plantronics nor the names of its contributors
    may be used to endorse or promote products derived from this
    software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
