#ifndef MethodTypes_hxx
#define MethodTypes_hxx


namespace Vocal2
{
enum MethodTypes
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
};

MethodTypes
getMethodType(const char* name, int len);

}


#endif
