//
// Copyright (C) 2008 Plantronics
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// $$
///////////////////////////////////////////////////////////////////////////////
// Author: Scott Godin (sgodin AT SipSpectrum DOT com)

#ifndef _SdpHelperResip_h_
#define _SdpHelperResip_h_

// SYSTEM INCLUDES
//#include <...>

// APPLICATION INCLUDES
#define BUILD_RESIP_SDP_HELPER

#ifdef BUILD_RESIP_SDP_HELPER
#include <sdp/Sdp.h>
#include <sdp/SdpMediaLine.h>
#include <resip/stack/SdpContents.hxx>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

//: Container for SdpHelperResip specification
// This class holds the information related to an SdpHelperResip.
//
namespace resip
{
   class ParseBuffer;
   class Data;
}

/**
  This class is used to form a sipX representation of SDP from
  a resiprocate representation.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class SdpHelperResip
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */
   static Sdp::SdpAddressType convertResipAddressType(resip::SdpContents::AddrType resipAddrType);
   static SdpMediaLine::SdpEncryptionMethod convertResipEncryptionMethod(resip::SdpContents::Session::Encryption::KeyType resipMethod);
   static Sdp* createSdpFromResipSdp(const resip::SdpContents& resipSdp);

/* ============================ ACCESSORS ================================= */

   static SdpMediaLine::SdpCrypto* parseCryptoLine(const resip::Data& cryptoLine);
   static bool parseFingerPrintLine(const resip::Data& fingerprintLine, SdpMediaLine::SdpFingerPrintHashFuncType& hashType, resip::Data& fingerPrint);
   static bool parseTransportCapabilitiesLine(const resip::Data& tcapLine, std::list<SdpMediaLine::SdpTransportProtocolCapabilities>& tcapList);
   static bool parsePotentialConfigurationLine(const resip::Data& pcfgLine, std::list<SdpMediaLine::SdpPotentialConfiguration>& pcfgList);

/* ============================ INQUIRY =================================== */


/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   static SdpMediaLine* parseMediaLine(const resip::SdpContents::Session::Medium& resipMedia, const resip::SdpContents::Session& resipSession, bool sessionRtcpEnabled);
   static void parseCryptoParams(resip::ParseBuffer& pb, 
                                 SdpMediaLine::SdpCryptoKeyMethod& keyMethod, 
                                 resip::Data& keyValue, 
                                 unsigned int& srtpLifetime, 
                                 unsigned int& srtpMkiValue, 
                                 unsigned int& srtpMkiLength);

};

/* ============================ INLINE METHODS ============================ */
#endif  // BUILD_RESIP_SDP_HELPER
#endif  // _SdpHelperResip_h_
