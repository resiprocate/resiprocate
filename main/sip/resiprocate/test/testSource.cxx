#ifdef HAVE_CONFIG_H
# include "resiprocate/config.hxx"
#endif
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


   return 0;
}
