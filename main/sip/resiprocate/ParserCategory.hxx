#ifndef ParserCategory_hxx
#define ParserCategory_hxx

#include <iostream>
#include <list>
#include <util/Data.hxx>
#include <util/ParseBuffer.hxx>

#include <sipstack/ParameterTypes.hxx>
#include <sipstack/HeaderFieldValue.hxx>


namespace Vocal2
{
class UnknownParameter;
class Parameter;

class ParserCategory
{
   public:
      ParserCategory(HeaderFieldValue* headerFieldValue)
         : mHeaderField(headerFieldValue),
           mIsParsed(false),
           mMine(false)
      {}

      ParserCategory(const ParserCategory& rhs);
      ParserCategory& operator=(const ParserCategory& rhs);

      virtual ~ParserCategory();

      virtual ParserCategory* clone() const = 0;

      virtual std::ostream& encode(std::ostream& str) const = 0;
      std::ostream& encodeFromHeaderFieldValue(std::ostream& str) const;

      bool exists(const ParamBase& paramType) const
      {
         checkParsed();
         return getParameterByEnum(paramType.getTypeNum());
      }

      // removing non-present parameter is allowed      
      void remove(const ParamBase& paramType)
      {
         checkParsed();
         removeParameterByEnum(paramType.getTypeNum());
      }

      Transport_Param::Type::Type& param(const Transport_Param& paramType) const;
      User_Param::Type::Type& param(const User_Param& paramType) const;
      Method_Param::Type::Type& param(const Method_Param& paramType) const;
      Ttl_Param::Type::Type& param(const Ttl_Param& paramType) const;
      Maddr_Param::Type::Type& param(const Maddr_Param& paramType) const;
      Lr_Param::Type::Type& param(const Lr_Param& paramType) const;
      Q_Param::Type::Type& param(const Q_Param& paramType) const;
      Purpose_Param::Type::Type& param(const Purpose_Param& paramType) const;
      Expires_Param::Type::Type& param(const Expires_Param& paramType) const;
      Handling_Param::Type::Type& param(const Handling_Param& paramType) const;
      Tag_Param::Type::Type& param(const Tag_Param& paramType) const;
      ToTag_Param::Type::Type& param(const ToTag_Param& paramType) const;
      FromTag_Param::Type::Type& param(const FromTag_Param& paramType) const;
      Duration_Param::Type::Type& param(const Duration_Param& paramType) const;
      Branch_Param::Type::Type& param(const Branch_Param& paramType) const;
      Received_Param::Type::Type& param(const Received_Param& paramType) const;
      Mobility_Param::Type::Type& param(const Mobility_Param& paramType) const;
      Comp_Param::Type::Type& param(const Comp_Param& paramType) const;
      Rport_Param::Type::Type& param(const Rport_Param& paramType) const;
      
      UnknownParameter& param(const Data& param) const;
      void remove(const Data& param); 
      bool exists(const Data& param) const;
      
      void parseParameters(ParseBuffer& pb);
      std::ostream& encodeParameters(std::ostream& str) const;

      bool isParsed() const {return mIsParsed;}
      
      virtual void parse(ParseBuffer& pb) = 0;

      HeaderFieldValue& getHeaderField() { return *mHeaderField; }
   protected:
      ParserCategory();

      // call before every access 
      void checkParsed() const;

      Parameter* getParameterByEnum(int type) const;
      void removeParameterByEnum(int type);

      Parameter* getParameterByData(const Data& data) const;
      void removeParameterByData(const Data& data);

      HeaderFieldValue* mHeaderField;
      typedef std::list<Parameter*> ParameterList; 
      mutable ParameterList mParameters;
      mutable ParameterList mUnknownParameters;

   private:
      friend std::ostream& operator<<(std::ostream&, const ParserCategory&);
      friend class NameAddr;
      bool mIsParsed;
      bool mMine;
};

std::ostream&
operator<<(std::ostream&, const ParserCategory& category);

}

#endif
