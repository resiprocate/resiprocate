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

/**
  @file SipDictionary.cpp
  @brief Implementation of osc::SipDictionary class.
*/



#include "ProfileStack.h"
#include <assert.h>
#include "SipDictionary.h"

/**
  The state value for the SIP dictionary, as specified by RFC 3485
*/

osc::byte_t osc::SipDictionary::s_stateValue[] =
  "\r\nReject-Contact: \r\nError-Info: \r\nTimestamp: \r\nCall-Info: \r\n"
  "Reply-To: \r\nWarning: \r\nSubject: ;handling=image;purpose=;cause=;t"
  "ext=card300 Multiple Choicesmimessage/sipfrag407 Proxy Authentication"
  " Requiredigest-integrity484 Address Incompletelephone-events494 Secur"
  "ity Agreement Requiredeactivated481 Call/Transaction Does Not Existal"
  "e=500 Server Internal Errorobust-sorting=416 Unsupported URI Schemerg"
  "ency415 Unsupported Media Typending488 Not Acceptable Herejected423 I"
  "nterval Too Briefrom-tagQ.8505 Version Not Supported403 Forbiddenon-u"
  "rgent429 Provide Referror Identity420 Bad Extensionoresource\r\na=key"
  "-mgmt:mikeyOPTIONS Language: 504 Server Time-outo-tag\r\nAuthenticati"
  "on-Info: Dec 380 Alternative Service503 Service Unavailable421 Extens"
  "ion Required405 Method Not Allowed487 Request Terminatedauth-interlea"
  "ving=\r\nm=application Aug 513 Message Too Large687 Dialog Terminated"
  "302 Moved Temporarily301 Moved Permanentlymultipart/signed\r\nRetry-A"
  "fter: GMThu, 402 Payment Required\r\na=orient:landscape400 Bad Reques"
  "true491 Request Pending501 Not Implemented406 Not Acceptable606 Not A"
  "cceptable\r\na=type:broadcastone493 Undecipherable\r\nMIME-Version: M"
  "ay 482 Loop Detected\r\nOrganization: Jun mode-change-neighbor=critic"
  "alertcp-fb489 Bad Eventls\r\nUnsupported: Jan 502 Bad Gatewaymode-cha"
  "nge-period=\r\na=orient:seascape\r\na=type:moderated404 Not Found305 "
  "Use Proxy\r\na=type:recvonly\r\na=type:meeting\r\nk=prompt:\r\nReferr"
  "ed-By: \r\nIn-Reply-To: TRUEncoding: 182 QueuedAuthenticate: \r\nUser"
  "-Agent: \r\na=framerate:\r\nAlert-Info: CANCEL \r\na=maxptime:;retry-"
  "after=uachannels=410 Gone\r\nRefer-To: \r\nPriority: \r\nm=control \r"
  "\na=quality:\r\na=sdplang:\r\na=charset:\r\nReplaces: REFER ipsec-ike"
  ";transport=\r\na=keywds:\r\nk=base64:;refresher=\r\na=ptime:\r\nk=cle"
  "ar:;received=;duration=\r\nAccept: \r\na=group:FALSE: INFO \r\nAccept"
  "-\r\na=lang:\r\nm=data mode-set=\r\na=tool:TLSun, \r\nDate: \r\na=cat"
  ":\r\nk=uri:\r\nProxy-;reason=;method=\r\na=mid:;maddr=opaque=\r\nMin-"
  ";alg=Mon, Tue, Wed, Fri, Sat, ;ttl=auts=\r\nr=\r\nz=\r\ne=;id=\r\ni=c"
  "rc=\r\nu=;q=uas414 Request-URI Too Longiveuprivacyudprefer600 Busy Ev"
  "erywherequired480 Temporarily Unavailable\r\na=type:H.33202 Accepted\r"
  "\nSession-Expires: \r\nSubscription-State: Nov \r\nService-Route: Sep"
  " \r\nAllow-Events: Feb \r\na=inactiveRTP/SAVP RTP/AVPF Anonymousips:\r"
  "\na=type:testel:MESSAGE \r\na=recvonly\r\na=sendonly\r\nc=IN IP4 \r\n"
  "Reason: \r\nAllow: \r\nEvent: \r\nPath: ;user=\r\nb=AS CT \r\nWWW-Aut"
  "henticate: Digest \r\na=sendrecvideoctet-align=application/sdpatheade"
  "rspauth=\r\na=orient:portraitimeouttr-inticonc=483 Too Many Hopslinfo"
  "ptionalgorithm=604 Does Not Exist Anywheresponse=\r\n\r\nRequest-Disp"
  "osition: MD580 Precondition Failureplaces422 Session Interval Too Sma"
  "llocal181 Call Is Being Forwardedomain=failurenderealm=SUBSCRIBE prec"
  "onditionormalipsec-mandatory413 Request Entity Too Large2e183 Session"
  " Progressctp486 Busy HeremoterminatedAKAv1-MD5-sessionone\r\nAuthoriz"
  "ation: 603 Declinextnonce=485 Ambiguousername=audio\r\nContent-Type: "
  "Mar \r\nRecord-Route: Jul 401 Unauthorized\r\nRequire: \r\nt=0 0.0.0."
  "0\r\nServer: REGISTER \r\nc=IN IP6 180 Ringing100 Tryingv=0\r\no=UPDA"
  "TE NOTIFY \r\nSupported: unknownAMRTP/AVP \r\nPrivacy: \r\nSecurity-\r"
  "\nExpires: \r\na=rtpmap:\r\nm=video \r\nm=audio \r\ns= false\r\na=con"
  "f:;expires=\r\nRoute: \r\na=fmtp:\r\na=curr:Client: Verify: \r\na=des"
  ":\r\nRAck: \r\nRSeq: BYE cnonce=100reluri=qop=TCPUDPqosxml;lr\r\nVia:"
  " SIP/2.0/TCP 408 Request Timeoutimerpsip:\r\nContent-Length: Oct \r\n"
  "Via: SIP/2.0/UDP ;comp=sigcomprobationack;branch=z9hG4bK\r\nMax-Forwa"
  "rds: Apr SCTPRACK INVITE \r\nCall-ID: \r\nContact: 200 OK\r\nFrom: \r"
  "\nCSeq: \r\nTo: ;tag=\x04\x10\xdd\x10\x11""1\r\x11\n\x07\x10\xb9\x0c\x10"
  "\xfe\x12\x10\xe1\x06\x11N\x07\x11N\x03\x11J\x04\x11J\x07\x10\xb2\x08\x11"
  "y\x06\x11\x81\x0f\x11\"\x0b\x11U\x06\x11k\x0b\x11`\x13\x10\xb2\x08\x11"
  "q\x05\x11\x87\x13\x10\xf7\x09\x0e\x8d\x08\r\xae\x0c\x10\xb9\x07\x10\x8e"
  "\x03\r\x96\x03\x10\x8a\x04\x10\x8a\x09\r\xd7\n\x0f\x12\x08\x0f\x8f\x09"
  "\x0f\x8f\x08\rl\x06\x0e""f\x09\x0el\n\x0el\x06\x0f\xc6\x07\x0f\xc6\x05"
  "\x11H\x06\x11H\x06\x0f\xbf\x07\x0f\xbf\x07\x0eU\x06\x0f\x16\x04\x0e\xf4"
  "\x03\x0e\xb1\x03\x10\xa6\x09\x10P\x03\x10\xa3\n\r\xb4\x05\x0e""6\x06\x0e"
  "\xd6\x03\r\xf9\x11\x0e\xf8\x04\x0c\xd9\x08\x0e\xea\x04\x09S\x03\nK\x04"
  "\x0e\xe4\x10\x0f""5\x09\x0e\xe4\x08\r?\x03\x0f\xe1\x0b\x10\x01\x03\x10"
  "\xac\x06\x10\x95\x0c\x0ev\x0b\x0f\xeb\n\x0f\xae\x05\x10+\x04\x10+\x08"
  "\x10z\x10\x0fI\x07\x0f\xb8\x09\x10>\x0b\x10\x0c\x07\x0fx\x0b\x0fm\x09"
  "\x10G\x08\x10\x82\x0b\x0f\xf6\x08\x10""b\x08\x0f\x87\x08\x10j\x04\x0f"
  "x\r\x0f\xcd\x08\r\xae\x10\x0f]\x0b\x0f\x98\x14\r \x1b\r \x04\r\xe0\x14"
  "\x0e\xb4\x0b\x0f\xa3\x0b\x07""4\x0f\rV\x04\x0e\xf4\x03\x10\xaf\x07\r4"
  "\x09\x0f'\x04\x10\x9b\x04\x10\x9f\x09\x10Y\x08\x10r\x09\x10""5\n\x10!"
  "\n\x10\x17\x08\x0f\xe3\x03\x10\xa9\x05\x0c\xac\x04\x0c\xbd\x07\x0c\xc1"
  "\x08\x0c\xc1\x09\x0c\xf6\x10\x0cr\x0c\x0c\x86\x04\rd\x0c\x0c\xd5\x09\x0c"
  "\xff\x1b\x0b\xfc\x11\x0c]\x13\x0c""0\x09\x0c\xa4\x0c\x0c$\x0c\r;\x03\r"
  "\x1a\x03\r\x1d\x16\x0c""C\x09\x0c\x92\x09\x0c\x9b\r\x0e\xcb\x04\r\x16"
  "\x06\r\x10\x05\x04\xf2\x0b\x0c\xe1\x05\x0b\xde\n\x0c\xec\x13\x0b\xe3\x07"
  "\x0b\xd4\x08\r\x08\x0c\x0c\xc9\x09\x0c:\x04\n\xe5\x0c\n#\x08\x0b:\x0e"
  "\x09\xab\x0f\x0e\xfa\x09\x0fo\x0c\n\x17\x0f\x09v\x0c\n_\x17\r\xe2\x0f"
  "\x07\xa8\n\x0f\x85\x0f\x08\xd6\x0e\x09\xb9\x0b\nz\x03\x0b\xdb\x03\x08"
  "\xc1\x04\x0e\xc7\x03\x08\xd3\x02\x04\x8d\x08\x0bJ\x05\x0b\x8c\x07\x0b"
  "a\x06\x05H\x04\x07\xf4\x05\x10""0\x04\x07\x1e\x08\x07\x1e\x05\x0b\x91"
  "\x10\x04\xca\x09\nq\x09\x0e\x87\x05\x04\x98\x05\x0bn\x0b\x04\x9b\x0f\x04"
  "\x9b\x07\x04\x9b\x03\x04\xa3\x07\x04\xa3\x10\x07\x98\x09\x07\x98\x05\x0b"
  "s\x05\x0bx\x05\x0b}\x05\x07\xb9\x05\x0b\x82\x05\x0b\x87\x05\x0b\x1d\x05"
  "\x08\xe4\x05\x0c\x81\x05\x0f""D\x05\x11@\x05\x08x\x05\x08\x9d\x05\x0f"
  "X\x05\x07?\x05\x0cm\x05\x10\xf2\x05\x0cX\x05\x06\xa9\x04\x07\xb6\x09\x05"
  "\x8c\x06\x06\x1a\x06\x0e\x81\n\x06\x16\n\n\xc4\x07\x0bZ\n\n\xba\x03\x0b"
  "\x1b\x04\x11""E\x06\x0c\x8c\x07\x05\xad\n\x0e\xda\x08\x0b""B\r\x09\xf7"
  "\x0b\x05\x1c\x09\x11\x16\x08\x05\xc9\x07\r\x86\x06\x0b\xcf\n\x06M\x04"
  "\x0b\xa2\x06\x06\x8d\x08\x05\xe6\x08\x0e\x11\x0b\n\x9b\x03\n\x04\x03\x0b"
  "\xb5\x05\x10\xd7\x04\x09\x94\x05\n\xe2\x03\x0b\xb2\x06\rg\x04\r\x11\x08"
  "\x08\xb7\x1b\x0e;\n\x09\xa1\x14\x04\x85\x15\x07\x83\x15\x07n\r\x09=\x17"
  "\x06\xae\x0f\x07\xe6\x14\x07\xbe\r\x06\n\r\x09""0\x16\x06\xf2\x12\x08"
  "\x1e!\x04\xaa\x13\x10\xc5\x08\n\x0f\x1c\x0e\x96\x18\x0b\xb8\x1a\x05\x95"
  "\x1a\x05u\x11\x06=\x16\x06\xdc\x1e\x0e\x19\x16\x05\xd1\x1d\x06 #\x05'"
  "\x11\x08}\x11\r\x99\x16\x04\xda\r\x0f\x1c\x16\x07\x08\x17\x05\xb4\r\x08"
  "\xc7\x13\x07\xf8\x12\x08W\x1f\x04\xfe\x19\x05N\x13\x08\x0b\x0f\x08\xe9"
  "\x17\x06\xc5\x13\x06{\x19\x05\xf1\x15\x07""D\x18\r\xfb\x0b\x0f\x09\x1b"
  "\r\xbe\x12\x08""0\x15\x07Y\x04\x0b\xa6\x04\x0b\xae\x04\x0b\x9e\x04\x0b"
  "\x96\x04\x0b\x9a\n\n\xb0\x0b\n\x90\x08\x0b""2\x0b\x09k\x08\x0b*\x0b\n"
  "\x85\x09\x0b\x12\n\n\xa6\r\x09\xea\x13\rt\x14\x07\xd2\x13\x09\x0b\x12"
  "\x08""B\x10\x09[\x12\x09\x1e\r\x0c\xb1\x0e\x0c\x17\x11\x09J\x0c\nS\x0c"
  "\nG\x09\n\xf7\x0e\x09\xc7\x0c\n;\x07\x06i\x08\x06i\x06\x09\xe3\x08\x0b"
  "R\n\n\xd8\x12\x06W\r\x06W\x07\x09\xe3\x04\n\xe9\x10\x07""0\x09\x0b\x00"
  "\x0c\n/\x05\n\xe9\x05\nk\x06\nk\n\n\xce\x09\n\xee\x03\x0b\xdb\x07\x0f"
  "~\n\x09\x97\n\x06q\x0e\x09\xd5\x17\x06\x93\x07\x0e\\\x07\x0f\xda\n\x0f"
  "5\r\r\xec\n\x09\x97\n\x06q\x08\x0b\"\x0f\x09\x85\x06\x0bh\x0c\rJ\x09\x0b"
  "\x09\x13\x08\xf8\x15\x08\xa2\x04\x0b\xaa\x0f\x05""f\r\x07#\x09\n\x06\x0b"
  "\rJ\x0f\x04\xee\x06\x04\xf8\x04\x09+\x04\x08S\x07\x08\xc0\x03\x11\x1f"
  "\x04\x11\x1e\x07\r\x8c\x03\x07""4\x04\x10\xdb\x03\x07""6\x03\r\xa9\r\x04"
  " \x0b\x04Q\x0c\x04:\x04\x0b\xb8\x04\x0c$\x04\x05\x95\x04\x04|\x04\x05"
  "u\x04\x04\x85\x04\x09k\x04\x06=\x06\x04{\x04\x06\xdc\x04\x07\x83\x04\x0e"
  "\x19\x12\x04\x00\x10\x08\x8e\x10\x08i\x0e\x04\x12\r\x04-\x03\x10\xb9\x04"
  "\x05\xd1\x04\x07n\x04\x06 \x07\x04t\x04\x0b\xfc\n\x04\\\x04\x05'\x04\x09"
  "=\x04\x08}\x04\x0f\xae\x04\r\x99\x04\x06\xae\x04\x04\xda\x09\x04\x09\x08"
  "\x11\"\x04\x0f\x1c\x04\x07\xe6\x04\x0e\xcb\x05\x08\xbd\x04\x07\x08\x04"
  "\x0f\xa3\x04\x06W\x04\x05\xb4\x04\x0f]\x04\x08\xc7\x08\x0b\xf4\x04\x07"
  "\xf8\x04\x07""0\x04\x07\xbe\x04\x08W\x05\rF\x04\x04\xfe\x04\x06\n\x04"
  "\x05N\x04\x0e;\x04\x08\x0b\x04\x09""0\x04\x08\xe9\x05\x05\xee\x04\x06"
  "\xc5\x04\x06\xf2\x04\x06{\x04\x09\xa1\x04\x05\xf1\x04\x08\x1e\x04\x07"
  "D\x04\x0b\xdd\x04\r\xfb\x04\x04\xaa\x04\x0b\xe3\x07\x0e\xee\x04\x0f\x09"
  "\x04\x0e\xb4\x04\r\xbe\x04\x10\xc5\x04\x08""0\x05\x0f""0\x04\x07Y\x04"
  "\n\x0f\x06\x0e""a\x04\x04\x81\x04\r\xab\x04\r\x93\x04\x11k\x04\x0e\x96"
  "\x05\x04""f\x09\x04k\x0b\x04""F\x04\x0c\xe1";

/**
  Constructor for osc::SipDictionary.
 */
osc::SipDictionary::SipDictionary()
  : State (s_stateValue, sizeof(s_stateValue)-1, 0, 0, 6),
    m_indexTableHead(0),
    m_indexTable(0),
    m_offsetTableHead(0),
    m_offsetTableNext(0)
{
  DEBUG_STACK_FRAME;
  retain();
}

/**
  Copy constructor for osc::SipDictionary.
 */
osc::SipDictionary::SipDictionary(SipDictionary const &r)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::SipDictionary.
 */
osc::SipDictionary::~SipDictionary()
{
  DEBUG_STACK_FRAME;
  delete[] (m_indexTableHead);
  delete[] (m_indexTable);
  delete[] (m_offsetTableHead);
  delete[] (m_offsetTableNext);
}

/**
  Assignment operator for osc::SipDictionary.
 */
osc::SipDictionary &
osc::SipDictionary::operator=(SipDictionary const &r)
{
  DEBUG_STACK_FRAME;
  if (&r == this)
  {
    return *this;
  }
  /* Assign attributes */
  assert(0);
  return *this;
}

bool
osc::SipDictionary::findIndex(const osc::byte_t *input,
                              size_t inputSize,
                              size_t &index,
                              size_t &length) const
{
  index = 0;
  length = 0;
  
  if (!m_indexTableHead || inputSize < 3)
  {
    return false;
  }

  osc::u16 i;

#ifdef OPTIMIZE_SIZE
  i = m_indexTableHead[*input];
#else
  i = m_indexTableHead[(input[0] << 9) | 
                       (input[1] << 2) |
                       (input[2] & 0x03)];
#endif

  size_t entrySize, entryOffset;
  while (i != 0xFFFF)
  {
    entrySize = s_stateValue[TABLE_OFFSET+(i * 3)];
    entryOffset = ((s_stateValue[TABLE_OFFSET+(i * 3)+1] << 8) |
                    s_stateValue[TABLE_OFFSET+(i * 3)+2]) - 1024;
    if (entrySize <= inputSize)
    {
      bool match = true;
      for (size_t j = 0; j < entrySize && match; j++)
      {
        if (s_stateValue[entryOffset + j] != input[j])
        {
          match = false;
        }
      }
      if (match && entrySize > length)
      {
        index = i;
        length = entrySize;
      }
    }
    i = m_indexTable[i];
  }

  return (length > 3)?true:false;
}

bool
osc::SipDictionary::findOffset(const osc::byte_t *input,
                               size_t inputSize,
                               size_t &offset,
                               size_t &length) const
{
  offset = 0;
  length = 0;  

  size_t matchLength;

  if (!m_offsetTableHead || inputSize < 3)
  {
    return false;
  }

  osc::u16 i;

#ifdef OPTIMIZE_SIZE
  i = m_offsetTableHead[*input];
#else
  i = m_offsetTableHead[(input[0] << 9) | 
                        (input[1] << 2) |
                        (input[2] & 0x03)];
#endif

  while (i != 0xFFFF)
  {
#ifdef OPTIMIZE_SIZE
    matchLength = 1;
#else
    matchLength = 2;
#endif
    while (matchLength < inputSize && 
           input[matchLength] == s_stateValue[i+matchLength])
    {
      matchLength++;
    }

    if (matchLength >= length)
    {
      length = matchLength;
      offset = i;
    }

    i = m_offsetTableNext[i];
  }

  return (length>2)?true:false;
}

bool
osc::SipDictionary::buildIndexTable()
{
  if (m_indexTableHead)
  {
    return true;
  }

  /* First, allocate memory for the tables */
#ifdef OPTIMIZE_SIZE
  m_indexTableHead = new osc::u16[128];
  if (!m_indexTableHead) { return false; }
  // Set all indices to 0xFFFF (65535)
  OSC_MEMSET(m_indexTableHead, 0xFF, 128 * sizeof(osc::u16));
#else
  m_indexTableHead = new osc::u16[65536];
  if (!m_indexTableHead) { return false; }
  // Set all indices to 0xFFFF (65535)
  OSC_MEMSET(m_indexTableHead, 0xFF, 65536 * sizeof(osc::u16));
#endif
  m_indexTable = new osc::u16[456];
  if (!m_indexTable) { return false; }

  /* Then, populate the tables */  
  size_t size, offset;
  osc::u16 hash;
  for (int i = 0; i < 456; i++)
  {
    size = s_stateValue[TABLE_OFFSET+(i * 3)];
    offset = ((s_stateValue[TABLE_OFFSET+(i * 3)+1] << 8) |
               s_stateValue[TABLE_OFFSET+(i * 3)+2]) - 1024;
#ifdef OPTIMIZE_SIZE
    hash = s_stateValue[offset] & 0x7F;
#else
    hash = (s_stateValue[offset] << 9) | 
           (s_stateValue[offset+1] << 2) |
           (s_stateValue[offset+2] & 0x03);
#endif
    m_indexTable[i] = m_indexTableHead[hash];
    m_indexTableHead[hash] = i;
  }

  return false;
}

bool
osc::SipDictionary::buildOffsetTable()
{
  if (m_offsetTableHead)
  {
    return true;
  }
  /* First, allocate memory for the tables */
#ifdef OPTIMIZE_SIZE
  m_offsetTableHead = new osc::u16[128];
  if (!m_offsetTableHead) { return false; }
  // Set all indices to 0xFFFF (65535)
  OSC_MEMSET(m_offsetTableHead, 0xFF, 128 * sizeof(osc::u16));
#else
  m_offsetTableHead = new osc::u16[65536];
  if (!m_offsetTableHead) { return false; }
  // Set all indices to 0xFFFF (65535)
  OSC_MEMSET(m_offsetTableHead, 0xFF, 65536 * sizeof(osc::u16));
#endif
  m_offsetTableNext = new osc::u16[TABLE_OFFSET];
  if (!m_offsetTableNext) { return false; }

  /* Then, populate the tables with appropriate indices */
#ifdef OPTIMIZE_SIZE
  osc::byte_t byte;
  for (int i = 0; i < TABLE_OFFSET; i++)
  {
    byte = s_stateValue[i] & 0x7F;
    m_offsetTableNext[i] = m_offsetTableHead[byte];
    m_offsetTableHead[byte] = i;
  }
#else
  osc::u16 bytes;
  for(int i = 0; i < TABLE_OFFSET - 2; i++)
  {
    bytes = (s_stateValue[i] << 9) | 
            (s_stateValue[i+1] << 2) |
            (s_stateValue[i+2] & 0x03);

    m_offsetTableNext[i] = m_offsetTableHead[bytes];
    m_offsetTableHead[bytes] = i;
  }
#endif

  return true;
}

#ifdef DEBUG
void
osc::SipDictionary::dump(std::ostream &os, unsigned int indent) const
{
  os << std::setw(indent) << ""
     << "[SipDictionary " << this << "]" << std::endl;

  size_t entry = TABLE_OFFSET;
  size_t size, offset;
  int index = 0;
  while (entry < sizeof(s_stateValue))
  {
    size = s_stateValue[entry];
    offset = ((s_stateValue[entry+1] << 8) | s_stateValue[entry+2]) - 1024;
    os << std::setw(indent) << ""
       << "  Entry #" << index << ": size = " << size 
       << ", offset = " << offset << ": [";
    for (size_t i = 0; i < size; i++)
    {
      osc::byte_t c = s_stateValue[i+offset];
      os << ((c>=32)?((char)c):'.');
    }
    os << "]" << std::endl;
    entry += 3;
    index++;
  }

  static_cast<const osc::State*>(this)->dump(os, indent+2);
}

std::ostream &
osc::operator<< (std::ostream &os, const osc::SipDictionary &s)
{
  DEBUG_STACK_FRAME;
  s.dump(os);
  return os;
}
#endif

