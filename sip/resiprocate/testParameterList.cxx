#include <sipstack/ParameterList.hxx>
#include <sipstack/DataParameter.hxx>
#include <sipstack/IntegerParameter.hxx>
#include <iostream>
#include <util/ParseBuffer.hxx>

using namespace Vocal2;
using namespace std;

int main(int argc, char** argv)
{
   ParameterList* p = new ParameterList;
   cerr << *p << endl;
   assert(p->find(ParameterTypes::ttl) == 0);
   delete p;

   p = new ParameterList();
   ParseBuffer f("=17");
   p->insert(new IntegerParameter(ParameterTypes::ttl, f));
   cerr << *p << endl;
   assert(p->find(ParameterTypes::ttl)->getType() == ParameterTypes::ttl);
   delete p;

   p = new ParameterList();
   ParseBuffer f1("=18");
   p->insert(new IntegerParameter(ParameterTypes::ttl, f1));
   ParseBuffer f2("=bar");
   p->insert(new DataParameter(ParameterTypes::transport, f2));
   cerr << *p << endl;
   assert(p->find(ParameterTypes::ttl)->getType() == ParameterTypes::ttl);
   assert(p->find(ParameterTypes::transport)->getType() == ParameterTypes::transport);
   delete p;

   p = new ParameterList();
   ParseBuffer f3("=baz");
   p->insert(new DataParameter(ParameterTypes::method, f3));
   ParseBuffer f4("=7");
   p->insert(new IntegerParameter(ParameterTypes::ttl, f4));
   ParseBuffer f5("=bar");
   p->insert(new IntegerParameter(ParameterTypes::transport, f5));
   cerr << *p << endl;
   assert(p->find(ParameterTypes::ttl)->getType() == ParameterTypes::ttl);
   assert(p->find(ParameterTypes::transport)->getType() == ParameterTypes::transport);
   assert(p->find(ParameterTypes::method)->getType() == ParameterTypes::method);
   delete p;

   cerr << "Finished Insertion Tests." << endl;
   
   p = new ParameterList();
   ParseBuffer f6("=5");
   p->insert(new IntegerParameter(ParameterTypes::ttl, f6));
   p->erase(ParameterTypes::ttl);
   cerr << *p << endl;
   assert(p->find(ParameterTypes::ttl) == 0);
   delete p;

   p = new ParameterList();
   ParseBuffer f7("=8");
   p->insert(new IntegerParameter(ParameterTypes::ttl, f7));
   ParseBuffer f8("=bar");
   p->insert(new DataParameter(ParameterTypes::transport, f8));
   p->erase(ParameterTypes::ttl);
   cerr << *p << endl;
   assert(p->find(ParameterTypes::ttl) == 0);
   delete p;

   p = new ParameterList();
   ParseBuffer f9("=baz");
   p->insert(new DataParameter(ParameterTypes::method, f9));
   ParseBuffer f10("=6");
   p->insert(new IntegerParameter(ParameterTypes::ttl, f10));
   ParseBuffer f11("=bar");
   p->insert(new DataParameter(ParameterTypes::transport, f11));
   p->erase(ParameterTypes::ttl);
   cerr << *p << endl;
   assert(p->find(ParameterTypes::ttl) == 0);
   delete p;

   cerr << "Deep copy test." << endl;

   p = new ParameterList();
   ParseBuffer f12("=baz");
   p->insert(new DataParameter(ParameterTypes::method, f12));
   ParseBuffer f13("=9");
   p->insert(new IntegerParameter(ParameterTypes::ttl, f13));
   ParseBuffer f14("=bar");
   p->insert(new DataParameter(ParameterTypes::transport,  f14));
   
   cerr << *p << endl;

   ParameterList* p2 = new ParameterList(*p);

   delete p;

   cerr << *p2 << endl;

   delete p2;

}
