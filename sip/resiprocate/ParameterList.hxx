#ifndef PARAMETERLIST_HXX
#define PARAMETERLIST_HXX

#include <ostream>
#include <sip2/sipstack/Parameter.hxx>

namespace Vocal2
{


class ParameterList
{
   public:
      ParameterList();

      ParameterList(const ParameterList& other);
      
      ~ParameterList();
      
      void insert(Parameter* param);
      
      Parameter* find(Parameter::ParamType type) const;
      void erase(Parameter::ParamType type);

      Parameter* find(const std::string& type) const;
      void erase(const std::string& type);

      Parameter* first;
};

}

std::ostream& operator<<(std::ostream& stream, Vocal2::ParameterList& pList);

#endif
