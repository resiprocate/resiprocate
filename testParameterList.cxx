#include <sipstack/StringParameter.hxx>
#include <sipstack/ParameterList.hxx>
#include <iostream>

using namespace Vocal2;
using namespace std;

int main(int argc, char** argv)
{
   ParameterList* p = new ParameterList;
   cout << *p << endl;
   assert(p->find(ParameterTypes::TTL) == 0);
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(ParameterTypes::TTL, "foo"));
   cout << *p << endl;
   assert(p->find(ParameterTypes::TTL)->getType() == ParameterTypes::TTL);
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(ParameterTypes::TTL, "foo"));
   p->insert(new StringParameter(ParameterTypes::Transport, "bar"));
   cout << *p << endl;
   assert(p->find(ParameterTypes::TTL)->getType() == ParameterTypes::TTL);
   assert(p->find(ParameterTypes::Transport)->getType() == ParameterTypes::Transport);
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(ParameterTypes::Method, "baz"));
   p->insert(new StringParameter(ParameterTypes::TTL, "foo"));
   p->insert(new StringParameter(ParameterTypes::Transport, "bar"));
   cout << *p << endl;
   assert(p->find(ParameterTypes::TTL)->getType() == ParameterTypes::TTL);
   assert(p->find(ParameterTypes::Transport)->getType() == ParameterTypes::Transport);
   assert(p->find(ParameterTypes::Method)->getType() == ParameterTypes::Method);
   delete p;

   cout << "Finished Insertion Tests." << endl;
   
   p = new ParameterList();
   p->insert(new StringParameter(ParameterTypes::TTL, "foo"));
   p->erase(ParameterTypes::TTL);
   cout << *p << endl;
   assert(p->find(ParameterTypes::TTL) == 0);
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(ParameterTypes::TTL, "foo"));
   p->insert(new StringParameter(ParameterTypes::Transport, "bar"));
   p->erase(ParameterTypes::TTL);
   cout << *p << endl;
   assert(p->find(ParameterTypes::TTL) == 0);
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(ParameterTypes::Method, "baz"));
   p->insert(new StringParameter(ParameterTypes::TTL, "foo"));
   p->insert(new StringParameter(ParameterTypes::Transport, "bar"));
   p->erase(ParameterTypes::TTL);
   cout << *p << endl;
   assert(p->find(ParameterTypes::TTL) == 0);
   delete p;

   cout << "Deep copy test." << endl;

   p = new ParameterList();
   p->insert(new StringParameter(ParameterTypes::Method, "baz"));
   p->insert(new StringParameter(ParameterTypes::TTL, "foo"));
   p->insert(new StringParameter(ParameterTypes::Transport, "bar"));
   
   cout << *p << endl;

   ParameterList* p2 = new ParameterList(*p);

   delete p;

   cout << *p2 << endl;

   delete p2;

}
