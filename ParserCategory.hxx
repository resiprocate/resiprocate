#ifndef ParserCategory_hxx
#define ParserCategory_hxx

#include <sipstack/Data.hxx>
#include <sipstack/ParameterTypes.hxx>
#include <iostream>


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

      //!dcm! -- will need to add different type of clones to HeaderFieldValue
      //in order to write copy constructor

      virtual ~ParserCategory() {}
      virtual ParserCategory* clone(HeaderFieldValue*) const = 0;

      virtual std::ostream& encode(std::ostream& str) const = 0;

      template <int T>
      typename ParameterType<T>::Type::Type& 
      operator[](const ParameterType<T>& parameterType)
      {
         checkParsed();
         return ((typename ParameterType<T>::Type&)mHeaderField->getParameter((typename ParameterTypes::Type)T)).value();
      }
      
      bool isParsed() const
      {
         return mIsParsed;
      }
      
      virtual void parse() = 0;

      UnknownParameter& operator[](const Data& param);

      HeaderFieldValue& getHeaderField() { return *mHeaderField; }
   protected:
      ParserCategory() 
      {}

      // call before every access 
      void checkParsed()
      {
         if (!mIsParsed)
         {
            mIsParsed = true;
            parse();
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
