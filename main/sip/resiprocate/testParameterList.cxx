

#include <sip2/sipstack/StringParameter.hxx>
#include <sip2/sipstack/ParameterList.hxx>
#include <iostream>

using namespace Vocal2;
using namespace std;

int main(int argc, char** argv)
{
   ParameterList* p = new ParameterList;
   cout << *p << endl;
   assert(p->find(Parameter::TTL) == 0);
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   cout << *p << endl;
   assert(p->find(Parameter::TTL)->getType() == Parameter::TTL);
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   p->insert(new StringParameter(Parameter::Transport, "bar"));
   cout << *p << endl;
   assert(p->find(Parameter::TTL)->getType() == Parameter::TTL);
   assert(p->find(Parameter::Transport)->getType() == Parameter::Transport);
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(Parameter::Method, "baz"));
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   p->insert(new StringParameter(Parameter::Transport, "bar"));
   cout << *p << endl;
   assert(p->find(Parameter::TTL)->getType() == Parameter::TTL);
   assert(p->find(Parameter::Transport)->getType() == Parameter::Transport);
   assert(p->find(Parameter::Method)->getType() == Parameter::Method);
   delete p;

   cout << "Finished Insertion Tests." << endl;
   
   p = new ParameterList();
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   p->erase(Parameter::TTL);
   cout << *p << endl;
   assert(p->find(Parameter::TTL) == 0);
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   p->insert(new StringParameter(Parameter::Transport, "bar"));
   p->erase(Parameter::TTL);
   cout << *p << endl;
   assert(p->find(Parameter::TTL) == 0);
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(Parameter::Method, "baz"));
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   p->insert(new StringParameter(Parameter::Transport, "bar"));
   p->erase(Parameter::TTL);
   cout << *p << endl;
   assert(p->find(Parameter::TTL) == 0);
   delete p;

   cout << "Deep copy test." << endl;

   p = new ParameterList();
   p->insert(new StringParameter(Parameter::Method, "baz"));
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   p->insert(new StringParameter(Parameter::Transport, "bar"));
   
   cout << *p << endl;

   ParameterList* p2 = new ParameterList(*p);

   delete p;

   cout << *p2 << endl;

   delete p2;

}

   
   
   
   
   

   

   
   
   
   
   

   
