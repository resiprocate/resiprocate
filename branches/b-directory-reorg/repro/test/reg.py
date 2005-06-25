#!/usr/bin/python
# program to sent test registrations

from socket import *
from random import *
from time import *

proxyHost = "10.0.1.6"
proxyPort = "5060"
domain = "localhost"

myHost = "10.0.1.3"
myPort = "5074"

print "Will register to ", proxyHost, ":", proxyPort 
randGen = Random()
randGen.seed()

sock = socket( AF_INET, SOCK_DGRAM )
sock.bind(('',int(myPort)))

startTime = time()

n = 0
numUsers = 5000
outstanding = 0;

while ( n < numUsers ):
    n = n+1
    user = "User" + str(n)
    rand = str(randGen.randrange(1,2000000000,1))
    data = "\
REGISTER sip:" +domain+ " SIP/2.0\r\n\
To: <sip:" + user + "@" +domain+ ">\r\n\
From: <sip:" +user+ "@" +domain+ ">;tag=90538639\r\n\
Via: SIP/2.0/UDP " +myHost+ ":" +myPort+ ";branch=z9hG4bK-" + rand + ";rport\r\n\
Call-ID: " + rand + "@bG9jYWxob3N0YWlu\r\n\
CSeq: 1 REGISTER\r\n\
Route: <sip:" +proxyHost+ ":" +proxyPort+ ">\r\n\
Contact: <sip:" +user+ "@" +myHost+ ":" +myPort+ ">;expires=3600\r\n\
Expires: 3600\r\n\
Max-Forwards: 70\r\n\
User-Agent: repro-test-python-reg/0.1 (pyton test register)\r\n\
Content-Length: 0\r\n\
\r\n"
    s = sock.sendto(data,(proxyHost,int(proxyPort)))
    outstanding = outstanding+1
    
    if ( n%100 == 0 ):
        print "Send registration for",user," in ",s,"octets"

    if ( outstanding > 40 ):
        junk = sock.recv(8192)
        outstanding = outstanding - 1
        #print "got packet"

# pick up the remaingin responses 
while ( outstanding > 50 ) :
  print "oustanding=", outstanding
  junk = sock.recv(8192)
  outstanding = outstanding - 1
        
endTime = time();
sock.close()

print "Did",n," registration in ",endTime-startTime,"seconds or",n/(endTime-startTime),"tps"




