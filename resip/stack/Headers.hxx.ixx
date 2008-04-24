//#define PARTIAL_TEMPLATE_SPECIALIZATION
#ifdef PARTIAL_TEMPLATE_SPECIALIZATION
template<bool>
class TypeIf
{
   public:
      template <class T>
      class Resolve
      {
         public:
            typedef T Type;
      };
};

class UnusedHeader
{
};

class TypeIf<false>
{
   public:
      template <class T>
      class Resolve
      {
         public:
            typedef UnusedHeader Type;
      };
};

#define UnusedChecking(_enum)                                           \
      typedef TypeIf<Headers::_enum != Headers::UNKNOWN> TypeIfT;       \
      typedef TypeIfT::Resolve<Type> Resolver;                          \
      typedef Resolver::Type UnknownReturn

#define MultiUnusedChecking(_enum)                                              \
      typedef TypeIf<Headers::_enum != Headers::UNKNOWN> TypeIfT;               \
      typedef TypeIfT::Resolve< ParserContainer<Type> > Resolver;               \
      typedef Resolver::Type UnknownReturn

#else

#define UnusedChecking(_enum) typedef int _dummy
#define MultiUnusedChecking(_enum) typedef int _dummy

#endif

#include "resip/stack/Headers.hxx.ixx.ixx"

//Enforces string encoding of extension headers
class H_RESIP_DO_NOT_USEs : public HeaderBase
{
   public:
      RESIP_HeapCount(H_RESIP_DO_NOT_USEs);
      enum {Single = false};
      typedef ParserContainer<StringCategory> Type;
      MultiUnusedChecking(RESIP_DO_NOT_USE);
      static Type& knownReturn(ParserContainerBase* container);
      virtual ParserContainerBase* makeContainer(HeaderFieldValueList* hfvs) const;
      virtual Headers::Type getTypeNum() const;
      virtual void merge(SipMessage&, const SipMessage&);
      H_RESIP_DO_NOT_USEs();
};
extern H_RESIP_DO_NOT_USEs h_RESIP_DO_NOT_USEs;
