#ifndef ParserCategory_hxx
#define ParserCategory_hxx

#include <iostream>
#include <sipstack/ParameterTypes.hxx>
#include <util/Data.hxx>


namespace Vocal2
{

class HeaderFieldValue;
class UnknownParameter;

class ParserCategory
{
   public:
      ParserCategory(HeaderFieldValue* headerFieldValue)
         : mHeaderField(headerFieldValue) 
      {}

      ParserCategory(const ParserCategory& rhs);

      //!dcm! -- will need to add different type of clones to HeaderFieldValue
      //in order to write copy constructor

      virtual ~ParserCategory() {}

      // called by HeaderFieldValue::clone()
      ParserCategory* clone(HeaderFieldValue*) const;
      virtual ParserCategory* clone() const = 0;

      virtual std::ostream& encode(std::ostream& str) const = 0;

      template <int T>
      typename ParameterType<T>::Type::Type& 
      operator[](const ParameterType<T>& parameterType) const
      {
         checkParsed();
         return mHeaderField->getParameter(parameterType).value();
      }

      template <int T>
      bool
      exists(const ParameterType<T>& parameterType) const
      {
         checkParsed();
         return mHeaderField->exists(parameterType);
      }

      //not necessary to call exists before remove(removing nothing is allowed)      
      template <int T>
      void
      remove(const ParameterType<T>& parameterType)
      {
         checkParsed();
         mHeaderField->remove(parameterType);
      }
      
      void parseParameters(const char* start, unsigned int length);

      bool isParsed() const
      {
         return mIsParsed;
      }
      
      virtual void parse() = 0;

      UnknownParameter& operator[](const Data& param) const;

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
            ncThis->parse();
         }
      }

      HeaderFieldValue* mHeaderField;
   private:
      bool mIsParsed;
};

std::ostream&
operator<<(std::ostream&, const ParserCategory& category);

}

#endif
