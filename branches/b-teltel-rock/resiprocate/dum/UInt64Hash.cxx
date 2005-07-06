#include "UInt64Hash.hxx"
#include "resiprocate/os/Data.hxx"

#if defined(HASH_MAP_NAMESPACE)
size_t HASH_MAP_NAMESPACE::hash<UInt64>::operator()(const UInt64& v) const
{
   return resip::Data::rawHash((const char*)&v, sizeof(v));
}
#endif

#if defined(__INTEL_COMPILER)
size_t std::hash_value(const UInt64& v)
{
   return resip::Data::rawHash((const char*)v,
                               sizeof(v));
}
#endif
