#include "sip2/sipstack/Circular.hxx"

using namespace Vocal2;

Circular::Circular(int size)
{
    mBuf = new char[size];
    mFront = 0;
    mBack = 0;
    mSize = size;
}


Circular::~Circular()
{
    delete mBuf;
}


void 
Circular::push_back(char c)
{
    mBuf[mBack] = c;

    mBack = (mBack + 1) % mSize;
}

void 
Circular::pop_front()
{
    mFront = (mFront + 1) % mSize;
}

char 
Circular::front()
{
    return mBuf[mFront];
}

int 
Circular::size()
{
    if (mBack < mFront)
        return mBack + (mSize - mFront);

    else
        return mBack - mFront;
}

bool 
Circular::empty()
{
    return mFront == mBack;
}
