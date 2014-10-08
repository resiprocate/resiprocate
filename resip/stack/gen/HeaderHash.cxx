/* C++ code produced by gperf version 3.0.3 */
/* Command-line: gperf -C -D -E -L C++ -t --key-positions='*' --compare-strncmp --ignore-case -Z HeaderHash HeaderHash.gperf  */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "HeaderHash.gperf"

#include <string.h>
#include <ctype.h>
#include "resip/stack/HeaderTypes.hxx"

namespace resip
{
using namespace std;
#line 10 "HeaderHash.gperf"
struct headers { const char *name; Headers::Type type; };
/* maximum key range = 438, duplicates = 0 */

#ifndef GPERF_DOWNCASE
#define GPERF_DOWNCASE 1
static unsigned char gperf_downcase[256] =
  {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
     45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
     60,  61,  62,  63,  64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255
  };
#endif

#ifndef GPERF_CASE_STRNCMP
#define GPERF_CASE_STRNCMP 1
static int
gperf_case_strncmp (register const char *s1, register const char *s2, register unsigned int n)
{
  for (; n > 0;)
    {
      unsigned char c1 = gperf_downcase[(unsigned char)*s1++];
      unsigned char c2 = gperf_downcase[(unsigned char)*s2++];
      if (c1 != 0 && c1 == c2)
        {
          n--;
          continue;
        }
      return (int)c1 - (int)c2;
    }
  return 0;
}
#endif

class HeaderHash
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static const struct headers *in_word_set (const char *str, unsigned int len);
};

inline unsigned int
HeaderHash::hash (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439,   0, 439, 439, 439,   5,
        0, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439,   0,  65,  25,   0,  20,
       45,   5,  15,  30,  15,  80,  50,  40,   0,   5,
        0,  30,  15,  10,   0,   0,  70,   0, 110,  35,
        0, 439, 439, 439, 439, 439, 439,   0,  65,  25,
        0,  20,  45,   5,  15,  30,  15,  80,  50,  40,
        0,   5,   0,  30,  15,  10,   0,   0,  70,   0,
      110,  35,   0, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439, 439, 439, 439, 439,
      439, 439, 439, 439, 439, 439
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[28]];
      /*FALLTHROUGH*/
      case 28:
        hval += asso_values[(unsigned char)str[27]];
      /*FALLTHROUGH*/
      case 27:
        hval += asso_values[(unsigned char)str[26]];
      /*FALLTHROUGH*/
      case 26:
        hval += asso_values[(unsigned char)str[25]];
      /*FALLTHROUGH*/
      case 25:
        hval += asso_values[(unsigned char)str[24]];
      /*FALLTHROUGH*/
      case 24:
        hval += asso_values[(unsigned char)str[23]];
      /*FALLTHROUGH*/
      case 23:
        hval += asso_values[(unsigned char)str[22]];
      /*FALLTHROUGH*/
      case 22:
        hval += asso_values[(unsigned char)str[21]];
      /*FALLTHROUGH*/
      case 21:
        hval += asso_values[(unsigned char)str[20]];
      /*FALLTHROUGH*/
      case 20:
        hval += asso_values[(unsigned char)str[19]];
      /*FALLTHROUGH*/
      case 19:
        hval += asso_values[(unsigned char)str[18]];
      /*FALLTHROUGH*/
      case 18:
        hval += asso_values[(unsigned char)str[17]];
      /*FALLTHROUGH*/
      case 17:
        hval += asso_values[(unsigned char)str[16]];
      /*FALLTHROUGH*/
      case 16:
        hval += asso_values[(unsigned char)str[15]];
      /*FALLTHROUGH*/
      case 15:
        hval += asso_values[(unsigned char)str[14]];
      /*FALLTHROUGH*/
      case 14:
        hval += asso_values[(unsigned char)str[13]];
      /*FALLTHROUGH*/
      case 13:
        hval += asso_values[(unsigned char)str[12]];
      /*FALLTHROUGH*/
      case 12:
        hval += asso_values[(unsigned char)str[11]];
      /*FALLTHROUGH*/
      case 11:
        hval += asso_values[(unsigned char)str[10]];
      /*FALLTHROUGH*/
      case 10:
        hval += asso_values[(unsigned char)str[9]];
      /*FALLTHROUGH*/
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      /*FALLTHROUGH*/
      case 8:
        hval += asso_values[(unsigned char)str[7]];
      /*FALLTHROUGH*/
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      /*FALLTHROUGH*/
      case 6:
        hval += asso_values[(unsigned char)str[5]];
      /*FALLTHROUGH*/
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      /*FALLTHROUGH*/
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

const struct headers *
HeaderHash::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 113,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 29,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 438
    };

  static const struct headers wordlist[] =
    {
#line 20 "HeaderHash.gperf"
      {"t", Headers::To},
#line 26 "HeaderHash.gperf"
      {"o", Headers::Event},
#line 36 "HeaderHash.gperf"
      {"to", Headers::To},
#line 18 "HeaderHash.gperf"
      {"s", Headers::Subject},
#line 22 "HeaderHash.gperf"
      {"r", Headers::ReferTo},
#line 99 "HeaderHash.gperf"
      {"path", Headers::Path},
#line 14 "HeaderHash.gperf"
      {"e", Headers::ContentEncoding},
#line 52 "HeaderHash.gperf"
      {"date", Headers::Date},
#line 16 "HeaderHash.gperf"
      {"c", Headers::ContentType},
#line 12 "HeaderHash.gperf"
      {"i", Headers::CallID},
#line 64 "HeaderHash.gperf"
      {"host", Headers::Host},
#line 25 "HeaderHash.gperf"
      {"y", Headers::Identity},
#line 13 "HeaderHash.gperf"
      {"m", Headers::Contact},
#line 34 "HeaderHash.gperf"
      {"route", Headers::Route},
#line 17 "HeaderHash.gperf"
      {"f", Headers::From},
#line 15 "HeaderHash.gperf"
      {"l", Headers::ContentLength},
#line 93 "HeaderHash.gperf"
      {"join", Headers::Join},
#line 103 "HeaderHash.gperf"
      {"reason", Headers::Reason},
#line 83 "HeaderHash.gperf"
      {"warning", Headers::Warning},
#line 77 "HeaderHash.gperf"
      {"supported", Headers::Supported},
#line 81 "HeaderHash.gperf"
      {"unsupported", Headers::Unsupported},
#line 29 "HeaderHash.gperf"
      {"contact", Headers::Contact},
#line 23 "HeaderHash.gperf"
      {"b", Headers::ReferredBy},
#line 90 "HeaderHash.gperf"
      {"hide", Headers::UNKNOWN},
#line 21 "HeaderHash.gperf"
      {"v", Headers::Via},
#line 75 "HeaderHash.gperf"
      {"sip-etag", Headers::SIPETag},
#line 38 "HeaderHash.gperf"
      {"accept", Headers::Accept},
#line 110 "HeaderHash.gperf"
      {"rseq", Headers::RSeq},
#line 82 "HeaderHash.gperf"
      {"user-agent", Headers::UserAgent},
#line 19 "HeaderHash.gperf"
      {"k", Headers::Supported},
#line 27 "HeaderHash.gperf"
      {"cseq", Headers::CSeq},
#line 47 "HeaderHash.gperf"
      {"content-id", Headers::ContentId},
#line 63 "HeaderHash.gperf"
      {"origin", Headers::Origin},
#line 57 "HeaderHash.gperf"
      {"organization", Headers::Organization},
#line 37 "HeaderHash.gperf"
      {"via", Headers::Via},
#line 116 "HeaderHash.gperf"
      {"min-se", Headers::MinSE},
#line 124 "HeaderHash.gperf"
      {"user-to-user", Headers::UserToUser},
#line 32 "HeaderHash.gperf"
      {"from", Headers::From},
#line 43 "HeaderHash.gperf"
      {"allow", Headers::Allow},
#line 24 "HeaderHash.gperf"
      {"x", Headers::SessionExpires},
#line 86 "HeaderHash.gperf"
      {"authorization", Headers::Authorization},
#line 89 "HeaderHash.gperf"
      {"event", Headers::Event},
#line 50 "HeaderHash.gperf"
      {"content-type", Headers::ContentType},
#line 79 "HeaderHash.gperf"
      {"answer-mode", Headers::AnswerMode},
#line 91 "HeaderHash.gperf"
      {"identity", Headers::Identity},
#line 102 "HeaderHash.gperf"
      {"rack", Headers::RAck},
#line 84 "HeaderHash.gperf"
      {"www-authenticate",Headers::WWWAuthenticate},
#line 104 "HeaderHash.gperf"
      {"refer-to",Headers::ReferTo},
#line 69 "HeaderHash.gperf"
      {"record-route", Headers::RecordRoute},
#line 70 "HeaderHash.gperf"
      {"reply-to", Headers::ReplyTo},
#line 71 "HeaderHash.gperf"
      {"require", Headers::Require},
#line 65 "HeaderHash.gperf"
      {"priority", Headers::Priority},
#line 39 "HeaderHash.gperf"
      {"accept-contact", Headers::AcceptContact},
#line 88 "HeaderHash.gperf"
      {"encryption", Headers::UNKNOWN},
#line 35 "HeaderHash.gperf"
      {"subject", Headers::Subject},
#line 100 "HeaderHash.gperf"
      {"target-dialog", Headers::TargetDialog},
#line 49 "HeaderHash.gperf"
      {"content-language", Headers::ContentLanguage},
#line 106 "HeaderHash.gperf"
      {"replaces",Headers::Replaces},
#line 78 "HeaderHash.gperf"
      {"timestamp", Headers::Timestamp},
#line 48 "HeaderHash.gperf"
      {"content-encoding", Headers::ContentEncoding},
#line 30 "HeaderHash.gperf"
      {"content-length", Headers::ContentLength},
#line 74 "HeaderHash.gperf"
      {"server", Headers::Server},
#line 53 "HeaderHash.gperf"
      {"error-info", Headers::ErrorInfo},
#line 95 "HeaderHash.gperf"
      {"p-associated-uri", Headers::PAssociatedUri},
#line 28 "HeaderHash.gperf"
      {"call-id", Headers::CallID},
#line 107 "HeaderHash.gperf"
      {"reject-contact", Headers::RejectContact},
#line 41 "HeaderHash.gperf"
      {"accept-language", Headers::AcceptLanguage},
#line 54 "HeaderHash.gperf"
      {"in-reply-to", Headers::InReplyTo},
#line 40 "HeaderHash.gperf"
      {"accept-encoding", Headers::AcceptEncoding},
#line 62 "HeaderHash.gperf"
      {"cookie", Headers::Cookie},
#line 42 "HeaderHash.gperf"
      {"alert-info",Headers::AlertInfo},
#line 72 "HeaderHash.gperf"
      {"retry-after", Headers::RetryAfter},
#line 101 "HeaderHash.gperf"
      {"privacy", Headers::Privacy},
#line 46 "HeaderHash.gperf"
      {"content-disposition", Headers::ContentDisposition},
#line 118 "HeaderHash.gperf"
      {"remote-party-id", Headers::RemotePartyId},
#line 117 "HeaderHash.gperf"
      {"refer-sub", Headers::ReferSub},
#line 119 "HeaderHash.gperf"
      {"history-info", Headers::HistoryInfo},
#line 76 "HeaderHash.gperf"
      {"sip-if-match", Headers::SIPIfMatch},
#line 92 "HeaderHash.gperf"
      {"identity-info", Headers::IdentityInfo},
#line 94 "HeaderHash.gperf"
      {"p-asserted-identity", Headers::PAssertedIdentity},
#line 97 "HeaderHash.gperf"
      {"p-media-authorization", Headers::PMediaAuthorization},
#line 31 "HeaderHash.gperf"
      {"expires", Headers::Expires},
#line 45 "HeaderHash.gperf"
      {"call-info", Headers::CallInfo},
#line 73 "HeaderHash.gperf"
      {"flow-timer", Headers::FlowTimer},
#line 44 "HeaderHash.gperf"
      {"authentication-info", Headers::AuthenticationInfo},
#line 109 "HeaderHash.gperf"
      {"response-key", Headers::UNKNOWN},
#line 108 "HeaderHash.gperf"
      {"request-disposition", Headers::RequestDisposition},
#line 87 "HeaderHash.gperf"
      {"allow-events", Headers::AllowEvents},
#line 85 "HeaderHash.gperf"
      {"subscription-state",Headers::SubscriptionState},
#line 80 "HeaderHash.gperf"
      {"priv-answer-mode", Headers::PrivAnswerMode},
#line 96 "HeaderHash.gperf"
      {"p-called-party-id", Headers::PCalledPartyId},
#line 114 "HeaderHash.gperf"
      {"service-route", Headers::ServiceRoute},
#line 121 "HeaderHash.gperf"
      {"p-charging-vector", Headers::PChargingVector},
#line 33 "HeaderHash.gperf"
      {"max-forwards", Headers::MaxForwards},
#line 105 "HeaderHash.gperf"
      {"referred-by",Headers::ReferredBy},
#line 51 "HeaderHash.gperf"
      {"content-transfer-encoding", Headers::ContentTransferEncoding},
#line 111 "HeaderHash.gperf"
      {"security-client", Headers::SecurityClient},
#line 67 "HeaderHash.gperf"
      {"proxy-authorization", Headers::ProxyAuthorization},
#line 98 "HeaderHash.gperf"
      {"p-preferred-identity", Headers::PPreferredIdentity},
#line 55 "HeaderHash.gperf"
      {"min-expires", Headers::MinExpires},
#line 56 "HeaderHash.gperf"
      {"mime-version", Headers::MIMEVersion},
#line 66 "HeaderHash.gperf"
      {"proxy-authenticate", Headers::ProxyAuthenticate},
#line 112 "HeaderHash.gperf"
      {"security-server", Headers::SecurityServer},
#line 115 "HeaderHash.gperf"
      {"session-expires", Headers::SessionExpires},
#line 68 "HeaderHash.gperf"
      {"proxy-require", Headers::ProxyRequire},
#line 120 "HeaderHash.gperf"
      {"p-access-network-info", Headers::PAccessNetworkInfo},
#line 122 "HeaderHash.gperf"
      {"p-charging-function-addresses", Headers::PChargingFunctionAddresses},
#line 123 "HeaderHash.gperf"
      {"p-visited-network-id", Headers::PVisitedNetworkID},
#line 113 "HeaderHash.gperf"
      {"security-verify", Headers::SecurityVerify},
#line 61 "HeaderHash.gperf"
      {"sec-websocket-accept", Headers::SecWebSocketAccept},
#line 58 "HeaderHash.gperf"
      {"sec-websocket-key", Headers::SecWebSocketKey},
#line 60 "HeaderHash.gperf"
      {"sec-websocket-key2", Headers::SecWebSocketKey2},
#line 59 "HeaderHash.gperf"
      {"sec-websocket-key1", Headers::SecWebSocketKey1}
    };

  static const signed char lookup[] =
    {
       -1,   0,  -1,  -1,  -1,  -1,   1,   2,  -1,  -1,
       -1,   3,  -1,  -1,  -1,  -1,   4,  -1,  -1,   5,
       -1,   6,  -1,  -1,   7,  -1,   8,  -1,  -1,  -1,
       -1,   9,  -1,  -1,  10,  -1,  11,  -1,  -1,  -1,
       -1,  12,  -1,  -1,  -1,  13,  14,  -1,  -1,  -1,
       -1,  15,  -1,  -1,  16,  -1,  17,  18,  -1,  19,
       -1,  20,  21,  -1,  -1,  -1,  22,  -1,  -1,  23,
       -1,  24,  -1,  25,  -1,  -1,  26,  -1,  -1,  27,
       28,  29,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  30,
       31,  32,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  33,  34,  -1,  -1,  35,  36,  -1,  37,
       38,  39,  -1,  40,  -1,  41,  -1,  42,  -1,  -1,
       -1,  43,  -1,  44,  45,  -1,  46,  -1,  47,  -1,
       -1,  -1,  48,  49,  -1,  -1,  -1,  50,  51,  52,
       53,  -1,  54,  55,  -1,  -1,  56,  -1,  57,  58,
       -1,  59,  -1,  -1,  60,  -1,  61,  -1,  -1,  -1,
       62,  63,  64,  -1,  65,  66,  67,  -1,  -1,  -1,
       68,  69,  -1,  -1,  -1,  70,  71,  -1,  -1,  -1,
       -1,  -1,  72,  -1,  -1,  -1,  -1,  -1,  -1,  73,
       -1,  -1,  -1,  -1,  -1,  74,  -1,  -1,  -1,  75,
       -1,  -1,  76,  -1,  -1,  -1,  -1,  77,  78,  79,
       -1,  80,  81,  -1,  82,  83,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  84,  -1,  -1,  85,  -1,  -1,
       -1,  -1,  -1,  -1,  86,  -1,  -1,  87,  88,  -1,
       -1,  89,  90,  91,  -1,  -1,  -1,  92,  -1,  -1,
       -1,  -1,  93,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  94,  -1,  -1,  -1,  95,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  96,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  97,  98,  99,  -1,  -1,  -1,
       -1,  -1, 100, 101,  -1,  -1,  -1,  -1,  -1,  -1,
      102,  -1,  -1,  -1,  -1, 103,  -1,  -1, 104,  -1,
       -1, 105,  -1,  -1, 106,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      107,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1, 108,  -1,  -1,  -1,  -1,
      109,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 110, 111,  -1,  -1,  -1,  -1, 112
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index].name;

              if ((((unsigned char)*str ^ (unsigned char)*s) & ~32) == 0 && !gperf_case_strncmp (str, s, len) && s[len] == '\0')
                return &wordlist[index];
            }
        }
    }
  return 0;
}
#line 125 "HeaderHash.gperf"

}
