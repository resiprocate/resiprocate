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
      
      SubComponent* find(SubComponent::Type type) const;
      void erase(SubComponent::Type type);

      SubComponent* find(const std::string& type) const;
  // find just sees if it is there and returns 0 if not.
  // get creates an unknownSubComponent with an empty value 
      SubComponent* get(const std::string& type) const;
      void erase(const std::string& type);

      SubComponent* first;


};

std::ostream& operator<<(std::ostream& stream, SubComponentList& pList);

}



#endif
