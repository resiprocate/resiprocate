/* C++ code produced by gperf version 3.0.1 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z HeaderHash HeaderHash.gperf  */

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
#include "resiprocate/HeaderTypes.hxx"

namespace resip
{
using namespace std;
using namespace resip;
#line 11 "HeaderHash.gperf"
struct headers { char *name; Headers::Type type; };
/* maximum key range = 277, duplicates = 0 */

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
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278,   0, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278,  10, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278,   0,   0, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278,   0,  10,  10,
        0,  15,  20,  10,   0,   5,   5, 115,  25,  75,
        0,   0,   0,  55,   0,  30,   0,   5,  45,  10,
        5,   0,   5, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
      278, 278, 278, 278, 278, 278
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)tolower(str[24])];
      /*FALLTHROUGH*/
      case 24:
        hval += asso_values[(unsigned char)tolower(str[23])];
      /*FALLTHROUGH*/
      case 23:
        hval += asso_values[(unsigned char)tolower(str[22])];
      /*FALLTHROUGH*/
      case 22:
        hval += asso_values[(unsigned char)tolower(str[21])];
      /*FALLTHROUGH*/
      case 21:
        hval += asso_values[(unsigned char)tolower(str[20])];
      /*FALLTHROUGH*/
      case 20:
        hval += asso_values[(unsigned char)tolower(str[19])];
      /*FALLTHROUGH*/
      case 19:
        hval += asso_values[(unsigned char)tolower(str[18])];
      /*FALLTHROUGH*/
      case 18:
        hval += asso_values[(unsigned char)tolower(str[17])];
      /*FALLTHROUGH*/
      case 17:
        hval += asso_values[(unsigned char)tolower(str[16])];
      /*FALLTHROUGH*/
      case 16:
        hval += asso_values[(unsigned char)tolower(str[15])];
      /*FALLTHROUGH*/
      case 15:
        hval += asso_values[(unsigned char)tolower(str[14])];
      /*FALLTHROUGH*/
      case 14:
        hval += asso_values[(unsigned char)tolower(str[13])];
      /*FALLTHROUGH*/
      case 13:
        hval += asso_values[(unsigned char)tolower(str[12])];
      /*FALLTHROUGH*/
      case 12:
        hval += asso_values[(unsigned char)tolower(str[11])];
      /*FALLTHROUGH*/
      case 11:
        hval += asso_values[(unsigned char)tolower(str[10])];
      /*FALLTHROUGH*/
      case 10:
        hval += asso_values[(unsigned char)tolower(str[9])];
      /*FALLTHROUGH*/
      case 9:
        hval += asso_values[(unsigned char)tolower(str[8])];
      /*FALLTHROUGH*/
      case 8:
        hval += asso_values[(unsigned char)tolower(str[7])];
      /*FALLTHROUGH*/
      case 7:
        hval += asso_values[(unsigned char)tolower(str[6])];
      /*FALLTHROUGH*/
      case 6:
        hval += asso_values[(unsigned char)tolower(str[5])];
      /*FALLTHROUGH*/
      case 5:
        hval += asso_values[(unsigned char)tolower(str[4])];
      /*FALLTHROUGH*/
      case 4:
        hval += asso_values[(unsigned char)tolower(str[3])];
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)tolower(str[2])];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)tolower(str[1])];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)tolower(str[0])];
        break;
    }
  return hval;
}

struct headers *
HeaderHash::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 79,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 25,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 277
    };

  static struct headers wordlist[] =
    {
#line 21 "HeaderHash.gperf"
      {"t", Headers::To},
#line 32 "HeaderHash.gperf"
      {"to", Headers::To},
#line 78 "HeaderHash.gperf"
      {"path", Headers::UNKNOWN},
#line 13 "HeaderHash.gperf"
      {"i", Headers::CallID},
#line 17 "HeaderHash.gperf"
      {"c", Headers::ContentType},
#line 15 "HeaderHash.gperf"
      {"e", Headers::ContentEncoding},
#line 52 "HeaderHash.gperf"
      {"priority", Headers::Priority},
#line 46 "HeaderHash.gperf"
      {"date", Headers::Date},
#line 18 "HeaderHash.gperf"
      {"f", Headers::From},
#line 74 "HeaderHash.gperf"
      {"hide", Headers::UNKNOWN},
#line 30 "HeaderHash.gperf"
      {"route", Headers::Route},
#line 16 "HeaderHash.gperf"
      {"l", Headers::ContentLength},
#line 25 "HeaderHash.gperf"
      {"contact", Headers::Contact},
#line 19 "HeaderHash.gperf"
      {"s", Headers::Subject},
#line 67 "HeaderHash.gperf"
      {"warning", Headers::Warning},
#line 70 "HeaderHash.gperf"
      {"authorization", Headers::Authorization},
#line 51 "HeaderHash.gperf"
      {"organization", Headers::Organization},
#line 72 "HeaderHash.gperf"
      {"encryption", Headers::UNKNOWN},
#line 34 "HeaderHash.gperf"
      {"accept", Headers::Accept},
#line 54 "HeaderHash.gperf"
      {"proxy-authorization", Headers::ProxyAuthorization},
#line 22 "HeaderHash.gperf"
      {"v", Headers::Via},
#line 57 "HeaderHash.gperf"
      {"reply-to", Headers::ReplyTo},
#line 47 "HeaderHash.gperf"
      {"error-info", Headers::ErrorInfo},
#line 81 "HeaderHash.gperf"
      {"reason", Headers::UNKNOWN},
#line 44 "HeaderHash.gperf"
      {"content-type", Headers::ContentType},
#line 33 "HeaderHash.gperf"
      {"via", Headers::Via},
#line 48 "HeaderHash.gperf"
      {"in-reply-to", Headers::InReplyTo},
#line 56 "HeaderHash.gperf"
      {"record-route", Headers::RecordRoute},
#line 82 "HeaderHash.gperf"
      {"refer-to",Headers::ReferTo},
#line 63 "HeaderHash.gperf"
      {"supported", Headers::Supported},
#line 59 "HeaderHash.gperf"
      {"retry-after", Headers::RetryAfter},
#line 38 "HeaderHash.gperf"
      {"allow", Headers::Allow},
#line 65 "HeaderHash.gperf"
      {"unsupported", Headers::Unsupported},
#line 79 "HeaderHash.gperf"
      {"privacy", Headers::UNKNOWN},
#line 61 "HeaderHash.gperf"
      {"sip-etag", Headers::SIPETag},
#line 24 "HeaderHash.gperf"
      {"call-id", Headers::CallID},
#line 53 "HeaderHash.gperf"
      {"proxy-authenticate", Headers::ProxyAuthenticate},
#line 90 "HeaderHash.gperf"
      {"RSeq", Headers::RSeq},
#line 37 "HeaderHash.gperf"
      {"alert-info",Headers::AlertInfo},
#line 14 "HeaderHash.gperf"
      {"m", Headers::Contact},
#line 27 "HeaderHash.gperf"
      {"expires", Headers::Expires},
#line 73 "HeaderHash.gperf"
      {"event", Headers::Event},
#line 42 "HeaderHash.gperf"
      {"content-encoding", Headers::ContentEncoding},
#line 31 "HeaderHash.gperf"
      {"subject", Headers::Subject},
#line 39 "HeaderHash.gperf"
      {"authentication-info", Headers::AuthenticationInfo},
#line 66 "HeaderHash.gperf"
      {"user-agent", Headers::UserAgent},
#line 83 "HeaderHash.gperf"
      {"referred-by",Headers::ReferredBy},
#line 26 "HeaderHash.gperf"
      {"content-length", Headers::ContentLength},
#line 35 "HeaderHash.gperf"
      {"accept-encoding", Headers::AcceptEncoding},
#line 40 "HeaderHash.gperf"
      {"call-info", Headers::CallInfo},
#line 68 "HeaderHash.gperf"
      {"www-authenticate",Headers::WWWAuthenticate},
#line 28 "HeaderHash.gperf"
      {"from", Headers::From},
#line 58 "HeaderHash.gperf"
      {"require", Headers::Require},
#line 84 "HeaderHash.gperf"
      {"replaces",Headers::Replaces},
#line 86 "HeaderHash.gperf"
      {"rseq", Headers::RSeq},
#line 43 "HeaderHash.gperf"
      {"content-language", Headers::ContentLanguage},
#line 77 "HeaderHash.gperf"
      {"p-preferred-identity", Headers::UNKNOWN},
#line 60 "HeaderHash.gperf"
      {"server", Headers::Server},
#line 55 "HeaderHash.gperf"
      {"proxy-require", Headers::ProxyRequire},
#line 23 "HeaderHash.gperf"
      {"cseq", Headers::CSeq},
#line 36 "HeaderHash.gperf"
      {"accept-language", Headers::AcceptLanguage},
#line 20 "HeaderHash.gperf"
      {"k", Headers::Supported},
#line 41 "HeaderHash.gperf"
      {"content-disposition", Headers::ContentDisposition},
#line 62 "HeaderHash.gperf"
      {"ip-if-match", Headers::SIPIfMatch},
#line 80 "HeaderHash.gperf"
      {"rack", Headers::RAck},
#line 75 "HeaderHash.gperf"
      {"p-asserted-identity", Headers::UNKNOWN},
#line 87 "HeaderHash.gperf"
      {"security-client", Headers::SecurityClient},
#line 76 "HeaderHash.gperf"
      {"p-media-authorization", Headers::UNKNOWN},
#line 91 "HeaderHash.gperf"
      {"RAck", Headers::RAck},
#line 29 "HeaderHash.gperf"
      {"max-forwards", Headers::MaxForwards},
#line 45 "HeaderHash.gperf"
      {"content-transfer-encoding", Headers::ContentTransferEncoding},
#line 69 "HeaderHash.gperf"
      {"subscription-state",Headers::SubscriptionState},
#line 49 "HeaderHash.gperf"
      {"min-expires", Headers::MinExpires},
#line 89 "HeaderHash.gperf"
      {"security-verify", Headers::SecurityVerify},
#line 71 "HeaderHash.gperf"
      {"allow-events", Headers::AllowEvents},
#line 88 "HeaderHash.gperf"
      {"security-server", Headers::SecurityServer},
#line 64 "HeaderHash.gperf"
      {"timestamp", Headers::Timestamp},
#line 85 "HeaderHash.gperf"
      {"response-key", Headers::UNKNOWN},
#line 50 "HeaderHash.gperf"
      {"mime-version", Headers::MIMEVersion}
    };

  static signed char lookup[] =
    {
      -1,  0,  1, -1,  2, -1,  3, -1, -1, -1, -1,  4, -1, -1,
      -1, -1,  5, -1,  6,  7, -1,  8, -1, -1,  9, 10, 11, 12,
      -1, -1, -1, 13, 14, 15, -1, -1, -1, 16, -1, -1, 17, 18,
      -1, -1, 19, -1, 20, -1, 21, -1, 22, 23, 24, 25, -1, -1,
      26, 27, 28, 29, -1, 30, -1, -1, -1, 31, 32, 33, 34, -1,
      -1, -1, 35, 36, 37, 38, 39, 40, -1, -1, 41, 42, 43, -1,
      44, 45, 46, -1, -1, 47, 48, -1, -1, -1, 49, -1, 50, -1,
      -1, 51, -1, -1, 52, 53, 54, -1, 55, -1, -1, -1, 56, 57,
      -1, 58, 59, 60, 61, -1, -1, 62, -1, -1, -1, -1, -1, -1,
      63, -1, -1, 64, -1, -1, -1, -1, 65, 66, 67, -1, -1, 68,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 69, -1,
      -1, 70, -1, -1, 71, -1, -1, 72, -1, -1, -1, 73, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, 74, -1, -1, -1, -1,
      -1, -1, -1, 75, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 76,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 77, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 78
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

              if (tolower(*str) == *s && !strncasecmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                return &wordlist[index];
            }
        }
    }
  return 0;
}
#line 92 "HeaderHash.gperf"

}
