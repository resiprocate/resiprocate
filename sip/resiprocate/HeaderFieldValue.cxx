#ifndef HeaderFieldValue
#define HeaderFieldValue


namespace Vocal2
{

class HeaderFieldValue
{
   public:
      HeaderFieldValue(const char* field, uint fieldLength)
         : mField(field),
           mFieldLength(fieldLength),
           mComponent(0)
      {}

      HeaderFieldValue(const HeaderFieldValue& hfv)
         : mField(hfv.mField),
           mFieldLength(hfv.mFieldLength),
           mParsed(hfv.mParsed),
           mParamList(hfv.mParamList),
           mUnkownParamList(hfv.UnknownParamList),
           mComponent(hfv.mComponent->clone(this))
      {}

      HeaderFieldValue* clone() const
      {
         return new HeaderFieldValue(*this);
      }
      
      ParameterList& getParameters()
      {
         return mParamList;
      }

      ParameterList& getUnknownParameters()
      {
         return mParamList;
      }

      HeaderFieldValue* clone() const
      {
         return new HeaderFieldValue(*this);
      }

      bool isParsed() const
      {
         return mComponent != 0;
      }

   private:
      const char* mField;
      const uint mFieldLength;
      ParameterList mParamList;
      ParameterList mUnkownParamList;
      Component* mComponent;
};


}
