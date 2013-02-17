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

#include <iostream>

/*
  This file does a wide variety of truly evil things. Don't
  learn from the contents of this file. Avert your eyes.
  Leave! Now! Before you become tainted!
*/

#define private public
#define protected public
#include "BitBuffer.h"
#include "Buffer.h"
#include "Compartment.h"
#include "CompartmentMap.h"
#include "CompressorData.h"
#include "Compressor.h"
#include "CrcComputer.h"
#include "DeflateBytecodes.h"
#include "DeflateCompressor.h"
#include "DeflateData.h"
#include "DeflateDictionary.h"
#include "MultiBuffer.h"
#include "MutexLockable.h"
#include "NackCodes.h"
#include "NackMap.h"
#include "ReadWriteLockable.h"
#include "Sha1Hasher.h"
#include "SigcompMessage.h"
#include "SipDictionary.h"
#include "Stack.h"
#include "StateChanges.h"
#include "State.h"
#include "StateHandler.h"
#include "StateList.h"
#include "StateMap.h"
#include "TcpStream.h"
#include "Types.h"
#include "Udvm.h"
#include "UdvmOpcodes.h"

#include "DeflateBytecodes.cpp"
#include "DeflateCompressor.cpp"
#include "SipDictionary.cpp"

#define PRINT_SIZE(x) \
  printf("  %-40.40s: %6d bytes\n",#x,sizeof(x));

int
main(int argc, char **argv)
{
  std::cout << "Dynamically allocated objects:" << std::endl;
  PRINT_SIZE(osc::BitBuffer);
  PRINT_SIZE(osc::Buffer);
  PRINT_SIZE(osc::Compartment);
  PRINT_SIZE(osc::CompartmentMap);
  PRINT_SIZE(osc::CompartmentMap::CompartmentBucket);
//  PRINT_SIZE(osc::Compressor);
//  PRINT_SIZE(osc::CompressorData);
  PRINT_SIZE(osc::CrcComputer);
  PRINT_SIZE(osc::DeflateBytecodes);
  PRINT_SIZE(osc::DeflateCompressor);
  PRINT_SIZE(osc::DeflateData);
  PRINT_SIZE(osc::DeflateDictionary);
  PRINT_SIZE(osc::MultiBuffer);
  PRINT_SIZE(osc::NackMap);
  PRINT_SIZE(osc::NackMap::NackNode);
//  PRINT_SIZE(osc::ReadWriteLockable);
  PRINT_SIZE(osc::Sha1Hasher);
  PRINT_SIZE(osc::SigcompMessage);
  PRINT_SIZE(osc::SipDictionary);
  PRINT_SIZE(osc::Stack);
  PRINT_SIZE(osc::State);
  PRINT_SIZE(osc::StateChanges);
  PRINT_SIZE(osc::StateHandler);
  PRINT_SIZE(osc::StateList);
  PRINT_SIZE(osc::StateList::StateNode);
  PRINT_SIZE(osc::StateMap);
  PRINT_SIZE(osc::StateMap::StateNode);
  PRINT_SIZE(osc::TcpStream);
  PRINT_SIZE(osc::Udvm);
  std::cout << std::endl;

  std::cout << "Statically allocated objects:" << std::endl;
  PRINT_SIZE(osc::CrcComputer::s_table);
  PRINT_SIZE(osc::DeflateBytecodes::bytecodes);
#ifndef OPTIMIZE_SIZE
  PRINT_SIZE(osc::DeflateCompressor::c_lengthTable);
  PRINT_SIZE(osc::DeflateCompressor::c_literalTable);
#endif
//  PRINT_SIZE(osc::s_nackCode);
  PRINT_SIZE(osc::SipDictionary::s_stateValue);
}
