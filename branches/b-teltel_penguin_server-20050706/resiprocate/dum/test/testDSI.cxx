#include "resiprocate/dum/DialogUsageManager.hxx"

#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/os/Log.hxx"

using namespace resip;

int 
main (int argc, char** argv)
{
    int level=(int)Log::DEBUG;
    if (argc > 1 ) level = atoi(argv[1]);

    Log::initialize(Log::COUT, (resip::Log::Level)level, argv[0]);


    DialogSetId a("a","a");
    DialogSetId b("a","b");
    DialogSetId c("b","a");
    DialogSetId d("b","b");
    DialogSetId e("a","a");

    assert( a == a );
    assert( ! (a < a ) );
    assert( a < b );
    assert( ! ( b < a ) );

    assert( a == a );
    assert( ! ( a > a ) );
    assert( b > a );
    assert( ! ( a >  b ) );

    cout << "a.hash()= " << a.hash() << endl;
    cout << "b.hash()= " << b.hash() << endl;
    cout << "e.hash()= " << e.hash() << endl;

    assert( a.hash() != b.hash() );
    assert( a.hash() == a.hash() );
    assert( a.hash() == e.hash() );

    cout << "test passed." << endl;

}
