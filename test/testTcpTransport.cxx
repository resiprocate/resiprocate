#include "sipstack/TcpTransport.hxx"
#include "sipstack/SipMessage.hxx"



char *onemsg = ("REGISTER sip:registrar.ixolib.com SIP/2.0\r\n"
                "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sfirst\r\n"
                "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=ssecond\r\n"
                "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sthird\r\n"
                "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sfourth\r\n"
                "Max-Forwards: 7\r\n"
                "To: Speedy <sip:speedy@biloxi.com>\r\n"
                "From: Speedy <sip:speedy@biloxi.com>;tag=88888\r\n"
                "Call-ID: 88888@8888\r\n"
                "CSeq: 6281 REGISTER\r\n"
                "Contact: <sip:speedy@192.0.2.4>\r\n"
                "Contact: <sip:qoq@192.0.2.4>\r\n"
                "Expires: 2700\r\n"
                "Content-Length: 0\r\n\r\n");




