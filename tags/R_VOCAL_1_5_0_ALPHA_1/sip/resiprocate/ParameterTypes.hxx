#ifndef ParameterTypes_hxx
#define ParameterTypes_hxx

#include <sipstack/DataParameter.hxx>
#include <sipstack/IntegerParameter.hxx>
#include <sipstack/FloatParameter.hxx>
#include <sipstack/ExistsParameter.hxx>
#include <sipstack/ParameterTypeEnums.hxx>
#include <sipstack/Symbols.hxx>

namespace Vocal2
{

class ParamBase
{
   public:
      virtual ParameterTypes::Type getTypeNum() const = 0;
};

class Transport_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::transport;}
      Transport_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::transport] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::transport] = Symbols::transport;
      }
};
extern Transport_Param p_transport;

class User_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::user;}
      User_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::user] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::user] = Symbols::user;
      }
};
extern User_Param p_user;

class Method_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::method;}
      Method_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::method] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::method] = Symbols::method;
      }
};
extern Method_Param p_method;

class Ttl_Param : public ParamBase
{
   public:
      typedef IntegerParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::ttl;}
      Ttl_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::ttl] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::ttl] = Symbols::ttl;
      }
};
extern Ttl_Param p_ttl;

class Maddr_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::maddr;}
      Maddr_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::maddr] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::maddr] = Symbols::maddr;
      }
};
extern Maddr_Param p_maddr;

class Lr_Param : public ParamBase
{
   public:
      typedef ExistsParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::lr;}
      Lr_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::lr] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::lr] = Symbols::lr;
      }
};
extern Lr_Param p_lr;

class Q_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::q;}
      Q_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::q] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::q] = Symbols::q;
      }
};
extern Q_Param p_q;

class Purpose_Param : public ParamBase
{
   public:
      typedef FloatParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::purpose;}
      Purpose_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::purpose] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::purpose] = Symbols::purpose;
      }
};
extern Purpose_Param p_purpose;

class Expires_Param : public ParamBase
{
   public:
      typedef IntegerParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::expires;}
      Expires_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::expires] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::expires] = Symbols::expires;
      }
};
extern Expires_Param p_expires;

class Handling_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::handling;}
      Handling_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::handling] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::handling] = Symbols::handling;
      }
};
extern Handling_Param p_handling;

class Tag_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::tag;}
      Tag_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::tag] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::tag] = Symbols::tag;
      }
};
extern Tag_Param p_tag;

class ToTag_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::toTag;}
      ToTag_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::toTag] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::toTag] = Symbols::toTag;
      }
};
extern ToTag_Param p_toTag;

class FromTag_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::fromTag;}
      FromTag_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::fromTag] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::fromTag] = Symbols::fromTag;
      }
};
extern FromTag_Param p_fromTag;

class Duration_Param : public ParamBase
{
   public:
      typedef IntegerParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::duration;}
      Duration_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::duration] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::duration] = Symbols::duration;
      }
};
extern Duration_Param p_duration;

class Branch_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::branch;}
      Branch_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::branch] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::branch] = Symbols::branch;
      }
};
extern Branch_Param p_branch;

class Received_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::received;}
      Received_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::received] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::received] = Symbols::received;
      }
};
extern Received_Param p_received;

class Mobility_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::mobility;}
      Mobility_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::mobility] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::mobility] = Symbols::mobility;
      }
};
extern Mobility_Param p_mobility;

class Comp_Param : public ParamBase
{
   public:
      typedef DataParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::comp;}
      Comp_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::comp] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::comp] = Symbols::comp;
      }
};
extern Comp_Param p_com;

class Rport_Param : public ParamBase
{
   public:
      typedef IntegerParameter Type;
      virtual ParameterTypes::Type getTypeNum() const {return ParameterTypes::rport;}
      Rport_Param()
      {
         ParameterTypes::ParameterFactories[ParameterTypes::rport] = Type::decode;
         ParameterTypes::ParameterNames[ParameterTypes::rport] = Symbols::rport;
      }
};
extern Rport_Param p_rport;
 
}

#endif
