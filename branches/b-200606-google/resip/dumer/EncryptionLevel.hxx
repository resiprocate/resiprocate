#if !defined(DumEncryptionLevel_hxx)
#define DumEncryptionLevel_hxx

namespace resip
{

typedef enum
{
   None = 0,
   Sign,
   Encrypt,
   SignAndEncrypt
} EncryptionLevel;

}

#endif
