#ifndef MethodTypes_hxx
#define MethodTypes_hxx

namespace Vocal2
{

class Data;

typedef enum
{
   ACK,
   BYE,
   CANCEL,
   INVITE,
   NOTIFY,
   OPTIONS,
   REFER,
   REGISTER,
   SUBSCRIBE,
   UNKNOWN
} MethodTypes;

MethodTypes
getMethodType(const Data& name);

MethodTypes
getMethodType(const char* name, int len);

}


#endif
