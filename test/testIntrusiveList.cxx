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
      typedef IntrusiveListElement<FooFoo*, Read> reader;
      typedef IntrusiveListElement<FooFoo*, Write> writer;

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

      fooHead->init();
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

      fooFooHead->reader::init();
      fooFooHead->writer::init();
      assert(fooFooHead->reader::empty());
      assert(fooFooHead->writer::empty());
      for (FooFoo::reader::iterator f = fooFooHead->reader::begin(); f != fooFooHead->reader::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->reader::push_front(fooFoo1);
      assert(!fooFooHead->reader::empty());
      assert(fooFooHead->writer::empty());
      cerr << endl << "first" << endl;
      for (FooFoo::reader::iterator f = fooFooHead->reader::begin(); f != fooFooHead->reader::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->reader::push_front(fooFoo2);
      cerr << endl << "second" << endl;
      for (FooFoo::reader::iterator f = fooFooHead->reader::begin(); f != fooFooHead->reader::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->reader::push_front(fooFoo3);   
      cerr << endl << "third" << endl;
      for (FooFoo::reader::iterator f = fooFooHead->reader::begin(); f != fooFooHead->reader::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted second" << endl;
      delete fooFoo2;
      for (FooFoo::reader::iterator f = fooFooHead->reader::begin(); f != fooFooHead->reader::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "fourth" << endl;
      fooFooHead->reader::push_front(fooFoo4);
      for (FooFoo::reader::iterator f = fooFooHead->reader::begin(); f != fooFooHead->reader::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted fourth, first" << endl;
      delete fooFoo1;
      delete fooFoo4;
      for (FooFoo::reader::iterator f = fooFooHead->reader::begin(); f != fooFooHead->reader::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }
   
      cerr << endl << "deleted third (empty)" << endl;
      delete fooFoo3;
      for (FooFoo::reader::iterator f = fooFooHead->reader::begin(); f != fooFooHead->reader::end(); ++f)
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


      fooFooHead->writer::init();
      fooFooHead->reader::init();
      assert(fooFooHead->writer::empty());
      assert(fooFooHead->reader::empty());
      for (FooFoo::writer::iterator f = fooFooHead->writer::begin(); f != fooFooHead->writer::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->writer::push_front(fooFoo1);
      assert(!fooFooHead->writer::empty());
      assert(fooFooHead->reader::empty());
      cerr << endl << "first" << endl;
      for (FooFoo::writer::iterator f = fooFooHead->writer::begin(); f != fooFooHead->writer::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->writer::push_front(fooFoo2);
      cerr << endl << "second" << endl;
      for (FooFoo::writer::iterator f = fooFooHead->writer::begin(); f != fooFooHead->writer::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      fooFooHead->writer::push_front(fooFoo3);   
      cerr << endl << "third" << endl;
      for (FooFoo::writer::iterator f = fooFooHead->writer::begin(); f != fooFooHead->writer::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted second" << endl;
      delete fooFoo2;
      for (FooFoo::writer::iterator f = fooFooHead->writer::begin(); f != fooFooHead->writer::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "fourth" << endl;
      fooFooHead->writer::push_front(fooFoo4);
      for (FooFoo::writer::iterator f = fooFooHead->writer::begin(); f != fooFooHead->writer::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }

      cerr << endl << "deleted fourth, first" << endl;
      delete fooFoo1;
      delete fooFoo4;
      for (FooFoo::writer::iterator f = fooFooHead->writer::begin(); f != fooFooHead->writer::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }
   
      cerr << endl << "deleted third (empty)" << endl;
      delete fooFoo3;
      for (FooFoo::writer::iterator f = fooFooHead->writer::begin(); f != fooFooHead->writer::end(); ++f)
      {
         cerr << (*f)->va1 << endl;
      }
   }

   return 0;
}
