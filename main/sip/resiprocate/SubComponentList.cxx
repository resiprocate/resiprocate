#include <sipstack/SubComponentList.hxx>


using namespace Vocal2;
using namespace std;

SubComponentList::SubComponentList()
   : first(0)
{}


SubComponentList::SubComponentList(const SubComponentList& other)
   : first(0)
{
   if (other.first)
   {
      first = other.first->clone();
      SubComponent* p = first;
      while(p->next != 0)
      {
         p->next = p->next->clone();
         p = p->next;
      }
   }
}

SubComponentList::~SubComponentList()
{
   if (first)
   {
      SubComponent* current = first;
      do
      {
         SubComponent* next = current->next;
         delete current;
         current = next;
      } while (current != 0);
   }
}

void SubComponentList::insert(SubComponent* param)
{
   param->next = first;
   first = param;
}

SubComponent* SubComponentList::find(SubComponent::Type type) const
{
   if (first)
   {
      SubComponent* p = first;
      
      do
      {
         if (p->getType() == type)
         {
            return p;
         }
      }
      while((p = p->next) != 0);
      return 0;
   }
   return 0;
}



void SubComponentList::erase(SubComponent::Type type)
{
   if(first)
   {
      if (first->getType() == type)
      {
         SubComponent* tmp = first;
         first = first->next;
         delete tmp;
         return;
      }
      
      SubComponent* lag = first;
      SubComponent* p = first->next;
      while (p != 0)
      {
         if (p->getType() == type)
         {
            lag->next = p->next;
            delete p;
            return;
         }
         lag = p;
         p = p->next;
      }
   }	 
}

SubComponent* SubComponentList::find(const Data& type) const
{
   if (first)
   {
      SubComponent* p = first;
      
      do
      {
         if (p->getName() == type)
         {
            return p;
         }
      }
      while((p = p->next) != 0);
      return 0;
   }
   return 0;
}


SubComponent* SubComponentList::get(const Data& type)
{
   if (first)
   {
      SubComponent* p = first;
      
      do
      {
         if (p->getName() == type)
         {
            return p;
         }
      }
      while((p = p->next) != 0);
      
   }

   UnknownSubComponent* toInsert = new UnknownSubComponent(type, "");
   insert(toInsert);
   
   return toInsert;

}


void SubComponentList::erase(const Data& type)
{
   if(first)
   {
      if (first->getName() == type)
      {
         SubComponent* tmp = first;
         first = first->next;
         delete tmp;
         return;
      }
      
      SubComponent* lag = first;
      SubComponent* p = first->next;
      while (p != 0)
      {
         if (p->getName() == type)
         {
            lag->next = p->next;
            delete p;
            return;
         }
         lag = p;
         p = p->next;
      }
   }	 
}

ostream& Vocal2::operator<<(ostream& stream, SubComponentList& pList)
{
   if (pList.first)
   {
      SubComponent* p = pList.first;
      
      do
      {
         stream << *p << " : ";
      }
      while((p = p->next) != 0);
   }
   return stream;
}

