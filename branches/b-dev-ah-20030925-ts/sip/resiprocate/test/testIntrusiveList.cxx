#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/IntrusiveListElement.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class Foo : public IntrusiveListElement<Foo*, 1>
{
   public:
      Foo(int v) : va1(v) {}
      int va1;
      int va2;
};

enum {Read, Write};
class FooFoo : public IntrusiveListElement<FooFoo*, Read>, public IntrusiveListElement<FooFoo*, Write>
{
   public:
      typedef IntrusiveListElement<FooFoo*, Read> read;
      typedef IntrusiveListElement<FooFoo*, Write> write;

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

      FooFoo::read::makeList(fooFooHead);
      FooFoo::write::makeList(fooFooHead);
      assert(fooFooHead->read::empty());
      assert(fooFooHead->write::empty());
      for (FooFoo::read::iterator f = fooFooHead->read::begin(); f != fooFooHead->read::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->read::push_front(fooFoo1);
      assert(!fooFooHead->read::empty());
      assert(fooFooHead->write::empty());
      cerr << endl << "first" << endl;
      for (FooFoo::read::iterator f = fooFooHead->read::begin(); f != fooFooHead->read::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->read::push_front(fooFoo2);
      cerr << endl << "second" << endl;
      for (FooFoo::read::iterator f = fooFooHead->read::begin(); f != fooFooHead->read::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->read::push_front(fooFoo3);   
      cerr << endl << "third" << endl;
      for (FooFoo::read::iterator f = fooFooHead->read::begin(); f != fooFooHead->read::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted second" << endl;
      delete fooFoo2;
      for (FooFoo::read::iterator f = fooFooHead->read::begin(); f != fooFooHead->read::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "fourth" << endl;
      fooFooHead->read::push_front(fooFoo4);
      for (FooFoo::read::iterator f = fooFooHead->read::begin(); f != fooFooHead->read::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted fourth, first" << endl;
      delete fooFoo1;
      delete fooFoo4;
      for (FooFoo::read::iterator f = fooFooHead->read::begin(); f != fooFooHead->read::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }
   
      cerr << endl << "deleted third (empty)" << endl;
      delete fooFoo3;
      for (FooFoo::read::iterator f = fooFooHead->read::begin(); f != fooFooHead->read::end(); ++f)
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

      FooFoo::write::makeList(fooFooHead);
      FooFoo::read::makeList(fooFooHead);
      assert(fooFooHead->write::empty());
      assert(fooFooHead->read::empty());
      for (FooFoo::write::iterator f = fooFooHead->write::begin(); f != fooFooHead->write::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->write::push_front(fooFoo1);
      assert(!fooFooHead->write::empty());
      assert(fooFooHead->read::empty());
      cerr << endl << "first" << endl;
      for (FooFoo::write::iterator f = fooFooHead->write::begin(); f != fooFooHead->write::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->write::push_front(fooFoo2);
      cerr << endl << "second" << endl;
      for (FooFoo::write::iterator f = fooFooHead->write::begin(); f != fooFooHead->write::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->write::push_front(fooFoo3);   
      cerr << endl << "third" << endl;
      for (FooFoo::write::iterator f = fooFooHead->write::begin(); f != fooFooHead->write::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted second" << endl;
      delete fooFoo2;
      for (FooFoo::write::iterator f = fooFooHead->write::begin(); f != fooFooHead->write::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "fourth" << endl;
      fooFooHead->write::push_front(fooFoo4);
      for (FooFoo::write::iterator f = fooFooHead->write::begin(); f != fooFooHead->write::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted fourth, first" << endl;
      delete fooFoo1;
      delete fooFoo4;
      for (FooFoo::write::iterator f = fooFooHead->write::begin(); f != fooFooHead->write::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }
   
      cerr << endl << "deleted third (empty)" << endl;
      delete fooFoo3;
      for (FooFoo::write::iterator f = fooFooHead->write::begin(); f != fooFooHead->write::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }
   }

   return 0;
}
