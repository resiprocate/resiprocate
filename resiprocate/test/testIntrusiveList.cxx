#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/IntrusiveListElement.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class Foo : public IntrusiveListElement<Foo*>
{
   public:
      Foo(int v) : va1(v) {}
      int va1;
      int va2;
};

class FooFoo;

typedef IntrusiveListElement<FooFoo*> FooRead;
typedef IntrusiveListElement1<FooFoo*> FooWrite;


class FooFoo : public FooRead, public FooWrite
{
   public:
      
      FooFoo(int v) : va1(v) {}

      int va1;
      int va2;
};

int
main(int argc, char* argv[])
{
   {
      Foo* fooHead = new Foo(-1);
      Foo* foo1 = new Foo(1);
      Foo* foo2 = new Foo(2);
      Foo* foo3 = new Foo(3);
      Foo* foo4 = new Foo(4);

      Foo::makeList(fooHead);
      assert(fooHead->empty());
      for (Foo::iterator f = fooHead->begin(); f != fooHead->end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooHead->push_front(foo1);
      assert(!fooHead->empty());
      cerr << endl << "first" << endl;
      assert((*fooHead->begin())->va1 == 1);
      assert((*fooHead->end())->va1 == -1);

      Foo::iterator j = fooHead->begin();
      ++j;
      cerr << (*j)->va1 << endl;
      assert((*j)->va1 == -1);      
      assert(*j == *fooHead->end());

      for (Foo::iterator f = fooHead->begin(); f != fooHead->end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooHead->push_front(foo2);
      cerr << endl << "second" << endl;
      for (Foo::iterator f = fooHead->begin(); f != fooHead->end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooHead->push_front(foo3);   
      cerr << endl << "third" << endl;
      for (Foo::iterator f = fooHead->begin(); f != fooHead->end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted second" << endl;
      delete foo2;
      for (Foo::iterator f = fooHead->begin(); f != fooHead->end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "fourth" << endl;
      fooHead->push_front(foo4);
      for (Foo::iterator f = fooHead->begin(); f != fooHead->end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted fourth, first" << endl;
      delete foo1;
      delete foo4;
      for (Foo::iterator f = fooHead->begin(); f != fooHead->end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }
   
      cerr << endl << "deleted third (empty)" << endl;
      delete foo3;
      for (Foo::iterator f = fooHead->begin(); f != fooHead->end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }
   }

   //=============================================================================
   // Read version
   //=============================================================================
   cerr << endl << "READ VERSION" << endl;
   {
      FooFoo* fooFooHead = new FooFoo(-1);
      FooFoo* fooFoo1 = new FooFoo(1);
      FooFoo* fooFoo2 = new FooFoo(2);
      FooFoo* fooFoo3 = new FooFoo(3);
      FooFoo* fooFoo4 = new FooFoo(4);

      FooRead::makeList(fooFooHead);
      FooWrite::makeList(fooFooHead);
      assert(fooFooHead->FooRead::empty());
      assert(fooFooHead->FooWrite::empty());
      for (FooRead::iterator f = fooFooHead->FooRead::begin(); f != fooFooHead->FooRead::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->FooRead::push_front(fooFoo1);
      assert(!fooFooHead->FooRead::empty());
      assert(fooFooHead->FooWrite::empty());
      cerr << endl << "first" << endl;
      for (FooRead::iterator f = fooFooHead->FooRead::begin(); f != fooFooHead->FooRead::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->FooRead::push_front(fooFoo2);
      cerr << endl << "second" << endl;
      for (FooRead::iterator f = fooFooHead->FooRead::begin(); f != fooFooHead->FooRead::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->FooRead::push_front(fooFoo3);   
      cerr << endl << "third" << endl;
      for (FooRead::iterator f = fooFooHead->FooRead::begin(); f != fooFooHead->FooRead::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted second" << endl;
      delete fooFoo2;
      for (FooRead::iterator f = fooFooHead->FooRead::begin(); f != fooFooHead->FooRead::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "fourth" << endl;
      fooFooHead->FooRead::push_front(fooFoo4);
      for (FooRead::iterator f = fooFooHead->FooRead::begin(); f != fooFooHead->FooRead::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted fourth, first" << endl;
      delete fooFoo1;
      delete fooFoo4;
      for (FooRead::iterator f = fooFooHead->FooRead::begin(); f != fooFooHead->FooRead::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }
   
      cerr << endl << "deleted third (empty)" << endl;
      delete fooFoo3;
      for (FooRead::iterator f = fooFooHead->FooRead::begin(); f != fooFooHead->FooRead::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }
   }

   //=============================================================================
   // Write version
   //=============================================================================
   cerr << endl << "WRITE VERSION" << endl;
   {      
      FooFoo* fooFooHead = new FooFoo(-1);
      FooFoo* fooFoo1 = new FooFoo(1);
      FooFoo* fooFoo2 = new FooFoo(2);
      FooFoo* fooFoo3 = new FooFoo(3);
      FooFoo* fooFoo4 = new FooFoo(4);

      FooWrite::makeList(fooFooHead);
      FooRead::makeList(fooFooHead);
      assert(fooFooHead->FooWrite::empty());
      assert(fooFooHead->FooRead::empty());
      for (FooWrite::iterator f = fooFooHead->FooWrite::begin(); f != fooFooHead->FooWrite::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->FooWrite::push_front(fooFoo1);
      assert(!fooFooHead->FooWrite::empty());
      assert(fooFooHead->FooRead::empty());
      cerr << endl << "first" << endl;
      for (FooWrite::iterator f = fooFooHead->FooWrite::begin(); f != fooFooHead->FooWrite::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->FooWrite::push_front(fooFoo2);
      cerr << endl << "second" << endl;
      for (FooWrite::iterator f = fooFooHead->FooWrite::begin(); f != fooFooHead->FooWrite::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->FooWrite::push_front(fooFoo3);   
      cerr << endl << "third" << endl;
      for (FooWrite::iterator f = fooFooHead->FooWrite::begin(); f != fooFooHead->FooWrite::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted second" << endl;
      delete fooFoo2;
      for (FooWrite::iterator f = fooFooHead->FooWrite::begin(); f != fooFooHead->FooWrite::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "fourth" << endl;
      fooFooHead->FooWrite::push_front(fooFoo4);
      for (FooWrite::iterator f = fooFooHead->FooWrite::begin(); f != fooFooHead->FooWrite::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted fourth, first" << endl;
      delete fooFoo1;
      delete fooFoo4;
      for (FooWrite::iterator f = fooFooHead->FooWrite::begin(); f != fooFooHead->FooWrite::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }
   
      cerr << endl << "deleted third (empty)" << endl;
      delete fooFoo3;
      for (FooWrite::iterator f = fooFooHead->FooWrite::begin(); f != fooFooHead->FooWrite::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }
   }

   return 0;
}
