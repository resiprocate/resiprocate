/* C++ code produced by gperf version 3.0.1 */
/* Command-line: gperf --ignore-case -D -E -L C++ -t -k '*' --compare-strncmp -Z HeaderHash HeaderHash.gperf  */

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
struct headers { char *name; Headers::Type type; };
/* maximum key range = 430, duplicates = 0 */

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
  static struct headers *in_word_set (const char *str, unsigned int len);
};

inline unsigned int
HeaderHash::hash (register const char *str, register unsigned int len)
{
  static unsigned short asso_values[] =
    {
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431,   0, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431,   0,  55,  15,   0,  30,
       35,  10,   5,  25,   5,  70,  40,  75,   0,   5,
        0,  25,  10,  50,   0,  10,  45,   0,  60,  20,
       50, 431, 431, 431, 431, 431, 431,   0,  55,  15,
        0,  30,  35,  10,   5,  25,   5,  70,  40,  75,
        0,   5,   0,  25,  10,  50,   0,  10,  45,   0,
       60,  20,  50, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431, 431, 431, 431, 431,
      431, 431, 431, 431, 431, 431
    };
  register int hval = len;

  switch (hval)
    {
      default:
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

struct headers *
HeaderHash::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 96,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 25,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 430
    };

  static struct headers wordlist[] =
    {
#line 20 "HeaderHash.gperf"
      {"t", Headers::To},
#line 26 "HeaderHash.gperf"
      {"o", Headers::Event},
#line 39 "HeaderHash.gperf"
      {"To", Headers::To},
#line 31 "HeaderHash.gperf"
      {"Path", Headers::Path},
#line 22 "HeaderHash.gperf"
      {"r", Headers::ReferTo},
#line 16 "HeaderHash.gperf"
      {"c", Headers::ContentType},
#line 25 "HeaderHash.gperf"
      {"y", Headers::Identity},
#line 12 "HeaderHash.gperf"
      {"i", Headers::CallID},
#line 14 "HeaderHash.gperf"
      {"e", Headers::ContentEncoding},
#line 60 "HeaderHash.gperf"
      {"Date", Headers::Date},
#line 17 "HeaderHash.gperf"
      {"f", Headers::From},
#line 95 "HeaderHash.gperf"
      {"Join", Headers::Join},
#line 15 "HeaderHash.gperf"
      {"l", Headers::ContentLength},
#line 38 "HeaderHash.gperf"
      {"Contact", Headers::Contact},
#line 21 "HeaderHash.gperf"
      {"v", Headers::Via},
#line 18 "HeaderHash.gperf"
      {"s", Headers::Subject},
#line 77 "HeaderHash.gperf"
      {"Warning", Headers::Warning},
#line 23 "HeaderHash.gperf"
      {"b", Headers::ReferredBy},
#line 29 "HeaderHash.gperf"
      {"Route", Headers::Route},
#line 24 "HeaderHash.gperf"
      {"x", Headers::SessionExpires},
#line 47 "HeaderHash.gperf"
      {"Accept", Headers::Accept},
#line 19 "HeaderHash.gperf"
      {"k", Headers::Supported},
#line 27 "HeaderHash.gperf"
      {"Via", Headers::Via},
#line 13 "HeaderHash.gperf"
      {"m", Headers::Contact},
#line 56 "HeaderHash.gperf"
      {"Content-ID", Headers::ContentId},
#line 51 "HeaderHash.gperf"
      {"Allow", Headers::Allow},
#line 90 "HeaderHash.gperf"
      {"RAck", Headers::RAck},
#line 91 "HeaderHash.gperf"
      {"Reason", Headers::Reason},
#line 66 "HeaderHash.gperf"
      {"Priority", Headers::Priority},
#line 35 "HeaderHash.gperf"
      {"Identity", Headers::Identity},
#line 99 "HeaderHash.gperf"
      {"Accept-Contact", Headers::AcceptContact},
#line 84 "HeaderHash.gperf"
      {"Event", Headers::Event},
#line 59 "HeaderHash.gperf"
      {"Content-Type", Headers::ContentType},
#line 68 "HeaderHash.gperf"
      {"Reply-To", Headers::ReplyTo},
#line 73 "HeaderHash.gperf"
      {"Supported", Headers::Supported},
#line 89 "HeaderHash.gperf"
      {"RSeq", Headers::RSeq},
#line 92 "HeaderHash.gperf"
      {"Privacy", Headers::Privacy},
#line 71 "HeaderHash.gperf"
      {"SIP-ETag", Headers::SIPETag},
#line 42 "HeaderHash.gperf"
      {"CSeq", Headers::CSeq},
#line 75 "HeaderHash.gperf"
      {"Unsupported", Headers::Unsupported},
#line 41 "HeaderHash.gperf"
      {"Call-ID", Headers::CallID},
#line 80 "HeaderHash.gperf"
      {"Refer-To", Headers::ReferTo},
#line 40 "HeaderHash.gperf"
      {"From", Headers::From},
#line 78 "HeaderHash.gperf"
      {"WWW-Authenticate", Headers::WWWAuthenticate},
#line 30 "HeaderHash.gperf"
      {"Record-Route", Headers::RecordRoute},
#line 100 "HeaderHash.gperf"
      {"Reject-Contact", Headers::RejectContact},
#line 61 "HeaderHash.gperf"
      {"Error-Info", Headers::ErrorInfo},
#line 62 "HeaderHash.gperf"
      {"In-Reply-To", Headers::InReplyTo},
#line 65 "HeaderHash.gperf"
      {"Organization", Headers::Organization},
#line 96 "HeaderHash.gperf"
      {"Target-Dialog", Headers::TargetDialog},
#line 37 "HeaderHash.gperf"
      {"Require", Headers::Require},
#line 82 "HeaderHash.gperf"
      {"Authorization", Headers::Authorization},
#line 103 "HeaderHash.gperf"
      {"Content-Length", Headers::ContentLength},
#line 76 "HeaderHash.gperf"
      {"User-Agent", Headers::UserAgent},
#line 55 "HeaderHash.gperf"
      {"Content-Encoding", Headers::ContentEncoding},
#line 50 "HeaderHash.gperf"
      {"Alert-Info", Headers::AlertInfo},
#line 69 "HeaderHash.gperf"
      {"Retry-After", Headers::RetryAfter},
#line 48 "HeaderHash.gperf"
      {"Accept-Encoding", Headers::AcceptEncoding},
#line 57 "HeaderHash.gperf"
      {"Content-Language", Headers::ContentLanguage},
#line 53 "HeaderHash.gperf"
      {"Call-Info", Headers::CallInfo},
#line 43 "HeaderHash.gperf"
      {"Subject", Headers::Subject},
#line 49 "HeaderHash.gperf"
      {"Accept-Language", Headers::AcceptLanguage},
#line 36 "HeaderHash.gperf"
      {"Identity-Info", Headers::IdentityInfo},
#line 70 "HeaderHash.gperf"
      {"Server", Headers::Server},
#line 83 "HeaderHash.gperf"
      {"Replaces", Headers::Replaces},
#line 46 "HeaderHash.gperf"
      {"Min-SE", Headers::MinSE},
#line 101 "HeaderHash.gperf"
      {"P-Called-Party-ID", Headers::PCalledPartyId},
#line 52 "HeaderHash.gperf"
      {"Authentication-Info", Headers::AuthenticationInfo},
#line 105 "HeaderHash.gperf"
      {"Answer-Mode", Headers::AnswerMode},
#line 44 "HeaderHash.gperf"
      {"Expires", Headers::Expires},
#line 107 "HeaderHash.gperf"
      {"Remote-Party-ID", Headers::RemotePartyId},
#line 34 "HeaderHash.gperf"
      {"Proxy-Authenticate", Headers::ProxyAuthenticate},
#line 102 "HeaderHash.gperf"
      {"P-Associated-URI", Headers::PAssociatedUri},
#line 104 "HeaderHash.gperf"
      {"Refer-Sub", Headers::ReferSub},
#line 81 "HeaderHash.gperf"
      {"Referred-By", Headers::ReferredBy},
#line 72 "HeaderHash.gperf"
      {"SIP-If-Match", Headers::SIPIfMatch},
#line 33 "HeaderHash.gperf"
      {"Proxy-Require", Headers::ProxyRequire},
#line 67 "HeaderHash.gperf"
      {"Proxy-Authorization", Headers::ProxyAuthorization},
#line 85 "HeaderHash.gperf"
      {"Allow-Events", Headers::AllowEvents},
#line 54 "HeaderHash.gperf"
      {"Content-Disposition", Headers::ContentDisposition},
#line 28 "HeaderHash.gperf"
      {"Max-Forwards", Headers::MaxForwards},
#line 74 "HeaderHash.gperf"
      {"Timestamp", Headers::Timestamp},
#line 32 "HeaderHash.gperf"
      {"Service-Route", Headers::ServiceRoute},
#line 98 "HeaderHash.gperf"
      {"P-Preferred-Identity", Headers::PPreferredIdentity},
#line 86 "HeaderHash.gperf"
      {"Security-Client", Headers::SecurityClient},
#line 94 "HeaderHash.gperf"
      {"P-Media-Authorization", Headers::PMediaAuthorization},
#line 97 "HeaderHash.gperf"
      {"P-Asserted-Identity", Headers::PAssertedIdentity},
#line 58 "HeaderHash.gperf"
      {"Content-Transfer-Encoding", Headers::ContentTransferEncoding},
#line 106 "HeaderHash.gperf"
      {"Priv-Answer-Mode", Headers::PrivAnswerMode},
#line 63 "HeaderHash.gperf"
      {"Min-Expires", Headers::MinExpires},
#line 88 "HeaderHash.gperf"
      {"Security-Verify", Headers::SecurityVerify},
#line 79 "HeaderHash.gperf"
      {"Subscription-State", Headers::SubscriptionState},
#line 87 "HeaderHash.gperf"
      {"Security-Server", Headers::SecurityServer},
#line 93 "HeaderHash.gperf"
      {"Request-Disposition", Headers::RequestDisposition},
#line 64 "HeaderHash.gperf"
      {"MIME-Version", Headers::MIMEVersion},
#line 45 "HeaderHash.gperf"
      {"Session-Expires", Headers::SessionExpires}
    };

  static signed char lookup[] =
    {
      -1,  0, -1, -1, -1, -1,  1,  2, -1,  3, -1,  4, -1, -1,
      -1, -1,  5, -1, -1, -1, -1,  6, -1, -1, -1, -1,  7, -1,
      -1, -1, -1,  8, -1, -1,  9, -1, 10, -1, -1, 11, -1, 12,
      13, -1, -1, -1, 14, -1, -1, -1, -1, 15, 16, -1, -1, -1,
      17, -1, -1, -1, 18, 19, -1, -1, -1, -1, 20, -1, -1, -1,
      -1, 21, -1, 22, -1, -1, 23, -1, -1, -1, -1, -1, -1, -1,
      -1, 24, -1, -1, -1, -1, 25, -1, -1, -1, -1, -1, -1, -1,
      -1, 26, -1, 27, -1, 28, -1, -1, -1, -1, 29, 30, 31, -1,
      32, 33, 34, -1, -1, -1, -1, 35, -1, -1, 36, 37, 38, -1,
      39, 40, 41, 42, -1, 43, -1, -1, -1, -1, -1, 44, -1, 45,
      46, 47, 48, 49, -1, -1, -1, 50, 51, 52, 53, 54, -1, -1,
      -1, 55, 56, -1, -1, -1, 57, -1, -1, -1, -1, -1, 58, -1,
      -1, 59, -1, -1, 60, -1, -1, 61, -1, -1, 62, -1, -1, 63,
      -1, 64, -1, -1, 65, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 66, -1, 67, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 68, 69, -1, -1, -1, -1, -1, -1, -1, 70, -1, -1, -1,
      -1, -1, -1, -1, 71, -1, -1, -1, -1, -1, -1, -1, 72, -1,
      -1, 73, -1, 74, 75, -1, -1, -1, -1, -1, 76, 77, -1, -1,
      78, -1, 79, -1, -1, 80, -1, -1, -1, -1, -1, -1, 81, -1,
      -1, -1, -1, -1, -1, -1, -1, 82, -1, 83, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, 84, 85, -1, -1, 86, -1, -1, -1, -1,
      -1, 87, 88, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 89, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 90, -1, -1, 91, -1, -1, -1, -1, -1, -1,
      92, -1, -1, -1, -1, -1, -1, -1, -1, 93, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 94, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 95
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
#line 108 "HeaderHash.gperf"

}
