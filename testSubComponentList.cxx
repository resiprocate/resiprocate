

#include <sipstack/StringSubComponent.hxx>
#include <sipstack/SubComponentList.hxx>
#include <iostream>

using namespace Vocal2;
using namespace std;

int main(int argc, char** argv)
{
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

}

   
   
   
   
   

   

   
   
   
   
   

   
