#ifndef digcalc_hxx
#define digcalc_hxx


#include "resiprocate/os/vmd5.hxx"
#include <string.h>
#include <iostream>
#include "resiprocate/os/Data.hxx"

#define HASHLEN 16
typedef char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN+1];
#define IN
#define OUT


using namespace std;

namespace resip
{

typedef MD5Context MD5_CTX;


void
MD5_Update(struct MD5Context *ctx, char const *buf, unsigned len)
{
   MD5Update(ctx, (unsigned char*)(buf), len);
}

void
MD5_Final(char digest[16], struct MD5Context *ctx)
{
   MD5Final((unsigned char*)digest, ctx);
}

/* calculate H(A1) as per HTTP Digest spec */
void DigestCalcHA1(
    IN char * pszAlg,
    IN char * pszUserName,
    IN char * pszRealm,
    IN char * pszPassword,
    IN char * pszNonce,
    IN char * pszCNonce,
    OUT HASHHEX SessionKey
    );

/* calculate request-digest/response-digest as per HTTP Digest spec */
void DigestCalcResponse(
    IN HASHHEX HA1,           /* H(A1) */
    IN char * pszNonce,       /* nonce from server */
    IN char * pszNonceCount,  /* 8 hex digits */
    IN char * pszCNonce,      /* client nonce */
    IN char * pszQop,         /* qop-value: "", "auth", "auth-int" */
    IN char * pszMethod,      /* method from the request */
    IN char * pszDigestUri,   /* requested URL */
    IN HASHHEX HEntity,       /* H(entity body) if qop="auth-int" */
    OUT HASHHEX Response      /* request-digest or response-digest */
    );

void CvtHex(
    IN HASH Bin,
    OUT HASHHEX Hex
    )
{
    unsigned short i;
    unsigned char j;

    for (i = 0; i < HASHLEN; i++) {
        j = (Bin[i] >> 4) & 0xf;
        if (j <= 9)
            Hex[i*2] = (j + '0');
         else
            Hex[i*2] = (j + 'a' - 10);
        j = Bin[i] & 0xf;
        if (j <= 9)
            Hex[i*2+1] = (j + '0');
         else
            Hex[i*2+1] = (j + 'a' - 10);
    };
    Hex[HASHHEXLEN] = '\0';
};

/* calculate H(A1) as per spec */
void DigestCalcHA1(
    IN char * pszAlg,
    IN char * pszUserName,
    IN char * pszRealm,
    IN char * pszPassword,
    IN char * pszNonce,
    IN char * pszCNonce,
    OUT HASHHEX SessionKey
    )
{
      MD5_CTX Md5Ctx;
      HASH HA1;

      MD5Init(&Md5Ctx);
      MD5_Update(&Md5Ctx, pszUserName, strlen(pszUserName));
      MD5_Update(&Md5Ctx, ":", 1);
      MD5_Update(&Md5Ctx, pszRealm, strlen(pszRealm));
      MD5_Update(&Md5Ctx, ":", 1);
      MD5_Update(&Md5Ctx, pszPassword, strlen(pszPassword));
      MD5_Final(HA1, &Md5Ctx);
      if (strcasecmp(pszAlg, "md5-sess") == 0) {
            MD5Init(&Md5Ctx);
            MD5_Update(&Md5Ctx, HA1, HASHLEN);
            MD5_Update(&Md5Ctx, ":", 1);
            MD5_Update(&Md5Ctx, pszNonce, strlen(pszNonce));
            MD5_Update(&Md5Ctx, ":", 1);
            MD5_Update(&Md5Ctx, pszCNonce, strlen(pszCNonce));
            MD5_Final(HA1, &Md5Ctx);
      };
      CvtHex(HA1, SessionKey);
};

/* calculate request-digest/response-digest as per HTTP Digest spec */
void DigestCalcResponse(
    IN HASHHEX HA1,           /* H(A1) */
    IN char * pszNonce,       /* nonce from server */
    IN char * pszNonceCount,  /* 8 hex digits */
    IN char * pszCNonce,      /* client nonce */
    IN char * pszQop,         /* qop-value: "", "auth", "auth-int" */
    IN char * pszMethod,      /* method from the request */
    IN char * pszDigestUri,   /* requested URL */
    IN HASHHEX HEntity,       /* H(entity body) if qop="auth-int" */
    OUT HASHHEX Response      /* request-digest or response-digest */
    )
{
      MD5_CTX Md5Ctx;
      HASH HA2;
      HASH RespHash;
       HASHHEX HA2Hex;

      // calculate H(A2)
      MD5Init(&Md5Ctx);
      MD5_Update(&Md5Ctx, pszMethod, strlen(pszMethod));
      MD5_Update(&Md5Ctx, ":", 1);
      MD5_Update(&Md5Ctx, pszDigestUri, strlen(pszDigestUri));
      if (strcasecmp(pszQop, "auth-int") == 0) {
            MD5_Update(&Md5Ctx, ":", 1);
            MD5_Update(&Md5Ctx, HEntity, HASHHEXLEN);
      };
      MD5_Final(HA2, &Md5Ctx);
       CvtHex(HA2, HA2Hex);


      // calculate response
      MD5Init(&Md5Ctx);
      MD5_Update(&Md5Ctx, HA1, HASHHEXLEN);
      MD5_Update(&Md5Ctx, ":", 1);
      MD5_Update(&Md5Ctx, pszNonce, strlen(pszNonce));
      MD5_Update(&Md5Ctx, ":", 1);
      if (*pszQop) {
          MD5_Update(&Md5Ctx, pszNonceCount, strlen(pszNonceCount));
          MD5_Update(&Md5Ctx, ":", 1);
          MD5_Update(&Md5Ctx, pszCNonce, strlen(pszCNonce));
          MD5_Update(&Md5Ctx, ":", 1);
          MD5_Update(&Md5Ctx, pszQop, strlen(pszQop));
          MD5_Update(&Md5Ctx, ":", 1);
      };
      MD5_Update(&Md5Ctx, HA2Hex, HASHHEXLEN);
      MD5_Final(RespHash, &Md5Ctx);
      CvtHex(RespHash, Response);
};

 
}


#endif
