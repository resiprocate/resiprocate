#ifndef HeaderFieldValue
#define HeaderFieldValue




namespace Vocal2
{

class HeaderFieldValue
{
   public:
      HeaderFieldValue(const char* field, uint fieldLength);
      HeaderFieldValue(const HeaderFieldValue& hfv);
      HeaderFieldValue(Component* component);

      HeaderFieldValue* clone() const;
      ParameterList& getParameters();
      ParameterList& getUnknownParameters();
      bool isParsed() const;
   private:
      const char* mField;
      const uint mFieldLength;
      ParameterList mParamList;
      ParameterList mUnkownParamList;
      Component* mComponent;
};


}
