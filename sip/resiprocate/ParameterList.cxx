#include <sip2/sipstack/ParameterList.hxx>


using namespace Vocal2;
using namespace std;

ParameterList::ParameterList()
   : first(0)
{}


ParameterList::ParameterList(const ParameterList& other)
   : first(0)
{
   if (other.first)
   {
      first = other.first->clone();
      Parameter* p = first;
      while(p->next != 0)
      {
         p->next = p->next->clone();
         p = p->next;
      }
   }
}

ParameterList::~ParameterList()
{
   if (first)
   {
      Parameter* current = first;
      do
      {
         Parameter* next = current->next;
         delete current;
         current = next;
      } while (current != 0);
   }
}

void ParameterList::insert(Parameter* param)
{
   param->next = first;
   first = param;
}

Parameter* ParameterList::find(Parameter::ParamType type)
{
   if (first)
   {
      Parameter* p = first;
      
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

void ParameterList::erase(Parameter::ParamType type)
{
   if(first)
   {
      if (first->getType() == type)
      {
         Parameter* tmp = first;
         first = first->next;
         delete tmp;
         return;
      }
      
      Parameter* lag = first;
      Parameter* p = first->next;
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

ostream& operator<<(ostream& stream, ParameterList& pList)
{
   if (pList.first)
   {
      Parameter* p = pList.first;
      
      do
      {
         stream << p->getName() << " ";
      }
      while((p = p->next) != 0);
   }
   return stream;
}
