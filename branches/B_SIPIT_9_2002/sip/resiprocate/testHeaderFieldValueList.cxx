

#include <sipstack/StringSubComponent.hxx>
#include <sipstack/SubComponentList.hxx>
#include <sipstack/HeaderFieldValueList.hxx>
#include <iostream>

using namespace Vocal2;
using namespace std;

int main(int argc, char** argv)
{

  HeaderFieldValueList *h = new HeaderFieldValueList;
  cout << "Empty list: " << endl;
  cout << *h << endl;

  HeaderFieldValue *myHeader1 = new HeaderFieldValue("foobar", 6);
  HeaderFieldValue *myHeader2 = new HeaderFieldValue("weekend", 7);
  
  h->push_front(myHeader1);
  h->push_front(myHeader2);
   
  cout << "List with two elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  myHeader1 = new HeaderFieldValue("asdfgh", 6);
  myHeader2 = new HeaderFieldValue("qwertyu", 7);

  h->push_front(myHeader1);
  h->push_front(myHeader2);

  cout << "List with four elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  myHeader1 = new HeaderFieldValue("poifds", 6);
  myHeader2 = new HeaderFieldValue("nmbhers", 7);

  h->push_back(myHeader1);
  h->push_back(myHeader2);

  cout << "List with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  HeaderFieldValueList *i = new HeaderFieldValueList(*h);

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with six elements: " << endl;
  cout << *i;
  cout << "Front of i " << *(i->first) << endl;
  cout << "Back of i " << *(i->last) << endl;
  cout << endl;

  i->pop_front();

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with five elements: " << endl;
  cout << *i;
  cout << "Front of i " << *(i->first) << endl;
  cout << "Back of i " << *(i->last) << endl;
  cout << endl;

  i->pop_front();

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with four elements: " << endl;
  cout << *i;
  cout << "Front of i " << *(i->first) << endl;
  cout << "Back of i " << *(i->last) << endl;
  cout << endl;

  i->pop_front();
  i->pop_front();
  i->pop_front();

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with one element: " << endl;
  cout << *i;
  cout << "Front of i " << *(i->first) << endl;
  cout << "Back of i " << *(i->last) << endl;
  cout << endl;

  i->pop_front();

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with zero element: " << endl;
  cout << *i;
  cout << "Front of i " << (i->first) << endl;
  cout << "Back of i " << (i->last) << endl;
  cout << endl;

  myHeader1 = new HeaderFieldValue("jgjgjg", 6);
  myHeader2 = new HeaderFieldValue("asasasa", 7);

  i->push_front(myHeader1);

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with one element: " << endl;
  cout << *i;
  cout << "Front of i " << *(i->first) << endl;
  cout << "Back of i " << *(i->last) << endl;
  cout << endl;



  i->push_front(myHeader2);

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with two elements: " << endl;
  cout << *i;
  cout << "Front of i " << *(i->first) << endl;
  cout << "Back of i " << *(i->last) << endl;
  cout << endl;

  



   /*
   SubComponentList* p = new SubComponentList;
   cout << *p << endl;
   assert(p->find(SubComponent::TTL) == 0);
   delete p;

   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   cout << *p << endl;
   assert(p->find(SubComponent::TTL)->getType() == SubComponent::TTL);
   delete p;

   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   p->insert(new StringSubComponent(SubComponent::Transport, "bar"));
   cout << *p << endl;
   assert(p->find(SubComponent::TTL)->getType() == SubComponent::TTL);
   assert(p->find(SubComponent::Transport)->getType() == SubComponent::Transport);
   delete p;

   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::Method, "baz"));
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   p->insert(new StringSubComponent(SubComponent::Transport, "bar"));
   cout << *p << endl;
   assert(p->find(SubComponent::TTL)->getType() == SubComponent::TTL);
   assert(p->find(SubComponent::Transport)->getType() == SubComponent::Transport);
   assert(p->find(SubComponent::Method)->getType() == SubComponent::Method);
   delete p;

   cout << "Finished Insertion Tests." << endl;
   
   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   p->erase(SubComponent::TTL);
   cout << *p << endl;
   assert(p->find(SubComponent::TTL) == 0);
   delete p;

   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   p->insert(new StringSubComponent(SubComponent::Transport, "bar"));
   p->erase(SubComponent::TTL);
   cout << *p << endl;
   assert(p->find(SubComponent::TTL) == 0);
   delete p;

   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::Method, "baz"));
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   p->insert(new StringSubComponent(SubComponent::Transport, "bar"));
   p->erase(SubComponent::TTL);
   cout << *p << endl;
   assert(p->find(SubComponent::TTL) == 0);
   delete p;

   cout << "Deep copy test." << endl;

   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::Method, "baz"));
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   p->insert(new StringSubComponent(SubComponent::Transport, "bar"));
   
   cout << *p << endl;

   SubComponentList* p2 = new SubComponentList(*p);

   delete p;

   cout << *p2 << endl;

   delete p2;
   */

}

   
   
   
   
   

   

   
   
   
   
   

   
