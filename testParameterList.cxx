

#include <sip2/sipstack/StringParameter.hxx>
#include <sip2/sipstack/ParameterList.hxx>



int main(int argc, char** argv)
{
   ParameterList* p = new ParameterList();
   cout << *p << endl;
   assert(p->find(Parameter::TTL) == 0);
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   assert(p->find(Parameter::TTL)->getType() == Parameter::TTL);
   cout << *p << endl;
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   p->insert(new StringParameter(Parameter::Transport, "bar"));
   assert(p->find(Parameter::TTL)->getType() == Parameter::TTL);
   assert(p->find(Parameter::Transport)->getType() == Parameter::Transport);
   cout << *p << endl;
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(Parameter::Method, "baz"));
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   p->insert(new StringParameter(Parameter::Transport, "bar"));
   assert(p->find(Parameter::TTL)->getType() == Parameter::TTL);
   assert(p->find(Parameter::Transport)->getType() == Parameter::Transport);
   assert(p->find(Parameter::Method)->getType() == Parameter::Method);
   cout << *p << endl;
   delete p;


   p = new ParameterList();
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   p->erase(Parameter::TTL);
   assert(p->find(Parameter::TTL)->getType() == 0);
   cout << *p << endl;
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   p->insert(new StringParameter(Parameter::Transport, "bar"));
   p->erase(Parameter::TTL);
   assert(p->find(Parameter::TTL)->getType() == 0);
   cout << *p << endl;
   delete p;

   p = new ParameterList();
   p->insert(new StringParameter(Parameter::Method, "baz"));
   p->insert(new StringParameter(Parameter::TTL, "foo"));
   p->insert(new StringParameter(Parameter::Transport, "bar"));
   p->erase(Parameter::TTL);
   assert(p->find(Parameter::TTL)->getType() == 0);
   cout << *p << endl;
   delete p;
}

   
   
   
   
   

   

   
   
   
   
   

   
