
#include <resiprocate/SipStack.hxx>

int main(int argc, char* argv[])
{
    using namespace std;
    int i = 1;
    int j = 2;
    int k = 3;

    cerr << "i = " << i << ", j = " << j << ", k = " << k << endl;
    resip::SipStack stack1;
    cerr << "i = " << i << ", j = " << j << ", k = " << k << endl;
    return 0;
}

