#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Tuple.hxx"

int
main(int argc, char *argv[])
{

   int sockfd;
   typedef int socklen_t;
   char buffer[1024];

   socklen_t len;
   struct sockaddr_in cliaddr, servaddr;

   if (argc != 2)
   {
      exit(0);
   }
   using namespace resip;

   sockfd = socket(AF_INET, SOCK_DGRAM, 0);

   bzero(&servaddr, sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   const int serv_port = 9999;
   servaddr.sin_port = htons(9999);

   inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
   inet_ntop(AF_INET, &servaddr.sin_addr, buffer,1024);
   
   std::cout << "Sending to : " << buffer << std::endl;
   
   connect(sockfd, (const sockaddr*)&servaddr, sizeof(servaddr));

   len = sizeof(cliaddr);
   getsockname(sockfd, (sockaddr*)&cliaddr, &len);
   inet_ntop(AF_INET, &cliaddr.sin_addr, buffer, 1024);
   std::cout << "new local source is: " << buffer << std::endl;
   std::cout << "DnsUtil::inet_ntop = " 
             << DnsUtil::inet_ntop(cliaddr.sin_addr)
             << std::endl;

   Data interfaceHost;
   Tuple destination;
   destination.port = 5060;
   DnsUtil::inet_pton(argv[1],destination.ipv4);

   sockaddr_in ephemeral;
   memset(&ephemeral,0,sizeof(ephemeral));
   inet_pton(AF_INET, argv[1], &ephemeral.sin_addr);
   ephemeral.sin_family = AF_INET;
   ephemeral.sin_port = htons(destination.port);
   
   
   int mSocket = socket(AF_INET, SOCK_DGRAM, 0);

   std::cout << " socket = " << mSocket
             << ":  for tx to "  << DnsUtil::inet_ntop(destination.ipv4)
             << std::endl;

   {
      connect(sockfd,
              (const sockaddr*)&destination.ipv4,
              sizeof(destination.ipv4));

      sockaddr_in cli;
      int len = sizeof(cli);

      getsockname(mSocket,(sockaddr*)&cli,
                  &len);

      char newbuf[1024];
      int xlen=sizeof(newbuf)/sizeof(*newbuf);
      inet_ntop(AF_INET, &cli.sin_addr, buffer, xlen);
      std::cout << " old inet_ntop returned " << buffer << std::endl;
      interfaceHost = DnsUtil::inet_ntop(cli.sin_addr);//.sin_addr);
         
   }
   std::cout << " new address is " << interfaceHost << std::endl;

   return 0;
}
