/* ***********************************************************************
   Open SigComp -- Implementation of RFC 3320 Signaling Compression

   Copyright 2005 Estacado Systems, LLC

   Your use of this code is governed by the license under which it
   has been provided to you. Unless you have a written and signed
   document from Estacado Systems, LLC stating otherwise, your license
   is as provided by the GNU General Public License version 2, a copy
   of which is available in this project in the file named "LICENSE."
   Alternately, a copy of the licence is available by writing to
   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02110-1307 USA

   Unless your use of this code is goverened by a written and signed
   contract containing provisions to the contrary, this program is
   distributed WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */

#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <map>

#include "Types.h"
#include "SigcompMessage.h"
#include "DeflateCompressor.h"
#include "StateHandler.h"
#include "StateChanges.h"
#include "Stack.h"

/*
  pcap format:

  Header 
    4 bytes: Magic (0xa1b2c3d4)
    2 bytes: Major Version
    2 bytes: Minor Version
    4 bytes: TZ
    4 bytes: TS
    4 bytes: Max Packet Size Captured
    4 bytes: Link type

  Packet
    4 bytes: time (seconds)
    4 bytes: time (ms)
    4 bytes: Packet size in file
    4 bytes: Packet size on wire
    [packet contents go here]
*/

struct pcap_file_header
{
  osc::u32 magic;
  osc::u16 maxVer;
  osc::u16 minVer;
  osc::u32 tz;
  osc::u32 ts;
  osc::u32 maxPacketSize;
  osc::u32 linkType;
};

struct pcap_packet_header
{
  osc::u32 timeSec;
  osc::u32 timeMs;
  osc::u32 packetFileSize;
  osc::u32 packetWireSize;
};

struct enet_packet_header
{
  /* Ethernet */
  char ethDestination[6];
  char ethSource[6];
  osc::u16 ethProto;  /* 0x0800 = IP */
};

struct ip_packet_header
{
  /* IP */
  osc::u8  ipVersionAndHeaderLength;
  osc::u8  ipTos;
  osc::u16 ipLength;

  osc::u16 ipId;
  osc::u16 ipFlagsAndFragOffset;

  osc::u8  ipTtl;
  osc::u8  ipProto; /* 0x11 = UDP */
  osc::u16 ipHeaderChecksum;

  osc::u32 ipSource;
  osc::u32 ipDestination;

  /* UDP */
  osc::u16 udpSourcePort;
  osc::u16 udpDestinationPort;
  osc::u16 udpLength;
  osc::u16 udpChecksum;

  /* SigComp */
  char sigcompMessage[1];
};

bool networkOrder;

#define HL(x) ((networkOrder)?(x):ntohl(x))
#define HS(x) ((networkOrder)?(x):ntohs(x))

int
main(int argc, char **argv)
{
  pcap_file_header fileHeader;
  pcap_packet_header packetHeader;
  int bytesRead;
  int compid;

  std::map<int,osc::Stack*> stackMap;

  networkOrder = true;
  
  if (argc != 2)
  {
    std::cout << "Usage: " << argv[0] << " <pcap_file>" << std::endl;
    exit(__LINE__);
  }

  int pcap_file = open (argv[1], O_RDONLY);
  if (pcap_file == -1)
  {
    std::cout << "Could not open file: " << argv[0] << std::endl;
    exit(__LINE__);
  }

  bytesRead = read(pcap_file, &fileHeader, sizeof(fileHeader));
  if (bytesRead != sizeof(fileHeader))
  {
    std::cout << "Could not read header from file: " << argv[0] << std::endl;
    exit(__LINE__);
  }

  if (ntohl(fileHeader.magic) != 0xa1b2c3d4)
  {
    if (fileHeader.magic == 0xa1b2c3d4)
    {
      networkOrder = true;
    }
    else
    {
      printf ("First four bytes don't match pcap magic: %8.8lX\n",
              ntohl(fileHeader.magic));
      exit(__LINE__);
    }
  }

  printf ("Reading pcap file, version %d.%d, linktype = %ld\n",
           HS(fileHeader.maxVer),
           HS(fileHeader.minVer),
           HL(fileHeader.linkType) );

  if(HL(fileHeader.linkType) != 1)
  {
    std::cout << "We only handle ethernet links" << argv[0] << std::endl;
  }

  char *buffer = new char[HL(fileHeader.maxPacketSize)];
  enet_packet_header *enet = 
    reinterpret_cast<enet_packet_header *>(buffer);
  ip_packet_header *wire = 
    reinterpret_cast<ip_packet_header *>(buffer + sizeof(enet_packet_header));

  int packnum = 0;

  while (read(pcap_file, &packetHeader, sizeof(packetHeader)) 
         == sizeof(packetHeader))
  {
    struct in_addr src;
    struct in_addr dst;
    packnum ++;

    bytesRead = read(pcap_file, buffer, HL(packetHeader.packetFileSize));

    if (bytesRead != (signed int) HL(packetHeader.packetFileSize))
    {
      printf ("Truncated packet at end of file\n");
      exit(__LINE__);
    }

    std::cout << "-----------------------------------"
                 "-----------------------------------" << std::endl;
    printf ("%3d: ", packnum);

    if (ntohs(enet->ethProto) != 0x0800)
    {
      printf ("Skipping non-IP packet: %4.4X\n", ntohs(enet->ethProto));
      continue;
    }
    else if (wire->ipProto != 0x11)
    {
      printf ("Skipping non-UDP packet: %2.2X\n", wire->ipProto);
      continue;
    }
    else if ((wire->sigcompMessage[0] & 0xf8) != 0xf8)
    {
      printf ("Skipping non-sigcomp packet: %2.2X\n", wire->sigcompMessage[0]);
      continue;
    }

    src.s_addr = wire->ipSource;
    dst.s_addr = wire->ipDestination;
    printf ("%ld.%6.6ld: ", 
                     HL(packetHeader.timeSec),
                     HL(packetHeader.timeMs));
    printf ("%s:%d -> ", inet_ntoa(src), ntohs(wire->udpSourcePort));
    printf ("%s:%d (%d bytes)\n", 
            inet_ntoa(dst), ntohs(wire->udpDestinationPort),
            ntohs(wire->udpLength)-8);

    /* Create and display the SigComp message */
    osc::SigcompMessage *sm = new osc::SigcompMessage(
      (osc::byte_t*) wire->sigcompMessage,
      ntohs(wire->udpLength)-8);

    std::cout << *sm << std::endl;
    delete sm;

    /* Run the message through a SigComp stack that represents the
       destination host */

    osc::Stack *stack = stackMap[wire->ipDestination];
    if (!stack)
    {
      std::cout << "Creating new stack for " << inet_ntoa(dst) << std::endl;
      osc::StateHandler *sh = new osc::StateHandler(8192,16,8192);
      sh->useSipDictionary();
      osc::Stack *ss = new osc::Stack(*sh);
      ss->addCompressor(new osc::DeflateCompressor(*sh));
      stackMap[wire->ipDestination] = ss;
      stack = ss;
    }

    char output[65536];
    osc::StateChanges *sc;
 
    int outputSize = 
      stack->uncompressMessage(wire->sigcompMessage, ntohs(wire->udpLength)-8,
                               output, sizeof(output), sc);
    if (outputSize > 0)
    {
      std::cout << "*** OUTPUT, size = " << outputSize << " ***" << std::endl;
      for (int k = 0; k < outputSize; k++)
      {
        std::cout << output[k];
      }
      // output[outputSize] = 0;
      std::cout << std::endl << std::endl;
      compid = wire->ipSource;
      std::cout << *sc << std::endl;
      stack->provideCompartmentId (sc, compid);
    }
    else
    {
      std::cout << "*** NO OUTPUT GENERATED ***" << std::endl;
      osc::SigcompMessage *nack = stack->getNack();
      if (nack)
      {
        std::cout << *nack << std::endl << std::endl;
      }
    }
  }

  close (pcap_file);

  return 0;
}
