#include <sipstack/HeaderFieldValueList.hxx>
#include <sipstack/ParserContainerBase.hxx>

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
      first = (other.first)->clone();
      last = first;
      HeaderFieldValue* p = first;
      while(p->next != 0)
      {
	HeaderFieldValue* q = p->next->clone();
	last = p->next = q;
	p = p->next;
      }
   }

   if (other.mParserContainer != 0)
   {
      mParserContainer = other.mParserContainer->clone(this);
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
   delete mParserContainer;
}

HeaderFieldValueList* 
HeaderFieldValueList::clone() const
{
   return new HeaderFieldValueList(*this);
}

ParserContainerBase*
HeaderFieldValueList::getParserContainer()
{
   return mParserContainer;
}

void HeaderFieldValueList::push_front(HeaderFieldValue* header)
{
  // if either is zero, then we don't have an element, this is first
  if (last == 0)
  {
     last = header;
  }
  // if last is zero, then first is zero, so this will be null
   header->next = first;
   first = header;

}

void HeaderFieldValueList::push_back(HeaderFieldValue* header)
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


void HeaderFieldValueList::pop_front()
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

ostream& 
HeaderFieldValueList::encode(std::ostream& str) const
{
   if (first)
   {
      HeaderFieldValue* current = first;
      do
      {
         current->encode(str);
         current = current->next;
      } while (current != 0);
   }
   return str;
}

ostream& operator<<(ostream& stream, HeaderFieldValueList& hList)
{
   if (hList.first)
   {
      HeaderFieldValue* current = hList.first;
      stream << "Header List: " << endl;
      do
	{
	  stream << *current << endl;
	  current = current->next;
	} while (current != 0);
   }
   return stream;
}


