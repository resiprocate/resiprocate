#ifndef ParameterList_hxx
#define ParameterList_hxx

#include <iostream>
#include <sipstack/Parameter.hxx>
#include <sipstack/UnknownParameter.hxx>

namespace Vocal2
{

class ParameterList
{
   public:
      ParameterList();
  
      ParameterList(const ParameterList& other);
  
      ~ParameterList();
  
      void insert(Parameter* param);
  
      Parameter* find(ParameterTypes::Type type) const;
      void erase(ParameterTypes::Type type);
  
      Parameter* find(const Data& type) const;

      std::ostream& encode(std::ostream& str) const;
  
      // find just sees if it is there and returns 0 if not.
  
      // get creates an unknownParameter with an empty value 
      Parameter* get(const Data& type);
      void erase(const Data& type);
  
      Parameter* first;
};

}

#endif
