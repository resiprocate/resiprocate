#ifndef SUBCOMPONENTLIST_HXX
#define SUBCOMPONENTLIST_HXX

#include <ostream>
#include <sip2/sipstack/SubComponent.hxx>

namespace Vocal2
{


class SubComponentList
{
   public:
      SubComponentList();

      SubComponentList(const SubComponentList& other);
      
      ~SubComponentList();
      
      void insert(SubComponent* param);
      
      SubComponent* find(SubComponent::ParamType type) const;
      void erase(SubComponent::ParamType type);

      SubComponent* find(const std::string& type) const;
      void erase(const std::string& type);

      SubComponent* first;
};

}

std::ostream& operator<<(std::ostream& stream, Vocal2::SubComponentList& pList);

#endif
