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
           mIsParsed(false)
      {}

      ParserCategory(const ParserCategory& rhs);

      //!dcm! -- will need to add different type of clones to HeaderFieldValue
      //in order to write copy constructor

      virtual ~ParserCategory();

      // called by HeaderFieldValue::clone()
      ParserCategory* clone(HeaderFieldValue*) const;
      virtual ParserCategory* clone() const = 0;

      virtual std::ostream& encode(std::ostream& str) const = 0;

#ifndef WIN32
      template <typename ParameterTypes::Type T>
      typename ParameterType<T>::Type::Type& 
      param(const ParameterType<T>& parameterType) const
      {
         checkParsed();
         typename ParameterType<T>::Type* p = dynamic_cast<typename ParameterType<T>::Type*>(getParameterByEnum(T));
         if (!p)
         {
            p = new typename ParameterType<T>::Type(T);
            mParameters.push_back(p);
         }
         return p->value();
      }
#endif

      template <ParameterTypes::Type T>
      bool
      exists(const ParameterType<T>& parameterType) const
      {
         checkParsed();
         return getParameterByEnum(T);
      }

      //not necessary to call exists before remove(removing nothing is allowed)      
      template <typename ParameterTypes::Type T>
      void
      remove(const ParameterType<T>& parameterType)
      {
         checkParsed();
         removeParameterByEnum(T);
      }
      
      void parseParameters(ParseBuffer& pb);
      void encodeParameters(std::ostream& str) const;

      bool isParsed() const
      {
         return mIsParsed;
      }
      
      virtual void parse(ParseBuffer& pb) = 0;

      UnknownParameter& param(const Data& param) const;

      void remove(const Data& param); 
      bool exists(const Data& param) const;
      

      HeaderFieldValue& getHeaderField() { return *mHeaderField; }
   protected:
      ParserCategory();

      // call before every access 
      void checkParsed() const
      {
         // !dlb! thread safety?
         if (!mIsParsed)
         {
            ParserCategory* ncThis = const_cast<ParserCategory*>(this);
            ncThis->mIsParsed = true;
            ParseBuffer pb(mHeaderField->mField, mHeaderField->mFieldLength);
            ncThis->parse(pb);
         }
      }

      Parameter* getParameterByEnum(ParameterTypes::Type type) const;
      void removeParameterByEnum(ParameterTypes::Type type);

      Parameter* getParameterByData(const Data& data) const;
      void removeParameterByData(const Data& data);

      HeaderFieldValue* mHeaderField;
      typedef std::list<Parameter*> ParameterList; 
      mutable ParameterList mParameters;
      mutable ParameterList mUnknownParameters;

   private:
      friend std::ostream& operator<<(std::ostream&, const ParserCategory&);
      bool mIsParsed;
};

std::ostream&
operator<<(std::ostream&, const ParserCategory& category);

}

#endif
