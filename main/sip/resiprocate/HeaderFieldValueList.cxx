#include <sip2/sipstack/HeaderFieldValueList.hxx>

using namespace Vocal2;
using namespace std;

HeaderFieldValueList::HeaderFieldValueList()
  : first(0), last(0)
{}


HeaderFieldValueList::HeaderFieldValueList(const HeaderFieldValueList& other)
  : first(0), last(0)
{
   if (other.first)
   {
      first = other.first->clone();
      last = first;
      HeaderFieldValue* p = first;
      while(p->next != 0)
      {
         p->next = p->next->clone();
	 last = p->next;
         p = p->next;
      }
   }
}

HeaderFieldValueList::~HeaderFieldValueList()
{
   if (first)
   {
      HeaderFieldValue* current = first;
      do
      {
         HeaderFieldValue* next = current->next;
         delete current;
         current = next;
      } while (current != 0);
   }
}

void HeaderFieldValueList::insert(HeaderFieldValue* header)
{
  // if either is zero, then we don't have an element, this is first
  if (last == 0)
     {
       last == header;
     }
  // if last is zero, then first is zero, so this will be null
   header->next = first;
   first = header;

}

void HeaderFieldValueList::append(HeaderFieldValue* header)
{
  if (last == 0)
    {
      first = header;
      last = header;
    }
  else
    {
      last->next = header;
      last = header;
    }
  header->next = 0;
}


void HeaderFieldValueList::deleteFirst()
{

  if (first == last)
    {
      if (first != 0)
	{
	  delete first;
	  first = last = 0;
	}
      return;
    }
  
  HeaderFieldValue *tmp = first;
  first = first->next;
  delete tmp;
}


ostream& operator<<(ostream& stream, HeaderFieldValueList& hList)
{
  stream << "Header "; 
  return stream;
}


