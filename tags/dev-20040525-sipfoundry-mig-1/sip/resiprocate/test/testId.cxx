#include <iostream>

#include "resiprocate/os/Id.hxx"

//class A;

//template class resip::Id<A>;

//resip::Id<A>::value_type resip::Id<A>::theGenerator = 0;
//resip::Id<A>::map_type   resip::Id<A>::theIdMap;

using namespace std;

class A;
template class resip::Id<A>;

class A
{
    public:
        typedef resip::Id<A> Id;

        //friend ostream& operator<<(ostream& , const A&);

        A();
        virtual ~A();
        const A::Id& getId() const;
        int callme() { return mynumber; }
    private:
        static int generator;
        int mynumber;
        A::Id mId;
        
};

int A::generator = 100;
#if 0
ostream& operator << (ostream& os, const A& a)
{
    os << "A(mynumber=" << a.mynumber << " id=" << a.mId.value() << ')' <<endl;
    return os;
}
#endif

A::A() : mynumber(++generator) {};

A::~A(){};

const A::Id&
A::getId() const { return mId; }


int
main(int, char*[])
{
    A arr[4];
    
    for(unsigned int i = 0 ; i < sizeof(arr)/sizeof(*arr); i++)
    {
        cout << "arr["<<i<<"].getId().value() == " << arr[i].getId().value() 
             << endl
             << "  and access example: "
             << "   arr["<<i<<"].getId()->callme() == " 
             << arr[i].getId()->callme()
             << endl;
    }


    A* na = new A;

    A::Id aid( na->getId());

    cout << "na==" << (void*)na << endl;
    cout << "na->callme(): " << na->callme() << endl;
    if (aid.valid())
    {
        cout << "aid is valid" << endl;
        cout << "aid->callme() " << aid->callme() << endl;
        cout << "deleting na" << endl;
    }
    delete na;
    cout << "aid.valid() == " << aid.valid() << endl;
    
    return 0;
}
