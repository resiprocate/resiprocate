#ifndef HeaderFieldValue_hxx
#define HeaderFieldValue_hxx

#include <iostream>
#include <sipstack/ParameterList.hxx>
#include <sipstack/ParseException.hxx>
#include <sipstack/ParameterTypes.hxx>


namespace Vocal2
{

class ParserCategory;
class UnknownParameter;

class HeaderFieldValue
{
   public:
      HeaderFieldValue();
      HeaderFieldValue(const char* field, unsigned int fieldLength);
      HeaderFieldValue(const HeaderFieldValue& hfv);
      HeaderFieldValue(ParserCategory* parserCategory);

      ~HeaderFieldValue();

      HeaderFieldValue* clone() const;

#ifndef WIN32
      template <typename ParameterTypes::Type T>
      typename ParameterType<T>::Type& getParameter(const ParameterType<T>& type)
      {
         Parameter* p = mParameterList.find(T);
         if (p == 0)
         {
            mParameterList.insert(p = new typename ParameterType<T>::Type(ParameterTypes::Type(T)));
         }
         assert(p);
         return *(dynamic_cast<typename ParameterType<T>::Type*>(p));
      }
#endif

      template <typename ParameterTypes::Type T>
      bool exists(const ParameterType<T> type)
      {
         return mParameterList.find(T);
      }

      template <typename ParameterTypes::Type T>
      void 
      remove(const ParameterType<T> type)
      {
         mParameterList.erase(T);
      }

      ParameterList& getParameters();

      ParameterList& getUnknownParameters();

      void parseParameters(const char* startPos, unsigned int length);

      UnknownParameter* get(const Data& type);

      bool exists(const Data& parameter);


      //shouldn't complain if parameter doesn't exists, otherwise there will be
      //two list walks
      void remove(const ParameterTypes::Type type);
      void remove(const Data& parameter);
      
      bool isParsed() const;

      ParserCategory* getParserCategory()
      {
         return mParserCategory;
      }

      void setParserCategory(ParserCategory* parser)
      {
         mParserCategory = parser;
      }

      std::ostream& encode(std::ostream& str) const;
      
      HeaderFieldValue* next;
      ParserCategory* mParserCategory;
      const char* mField;
      const unsigned int mFieldLength;

   private:
      ParameterList mParameterList;
      ParameterList mUnknownParameterList;
      bool mMine;

      friend std::ostream& operator<<(std::ostream&, HeaderFieldValue&);
};

std::ostream& operator<<(std::ostream& stream, 
			 HeaderFieldValue& hList);


}

#endif
