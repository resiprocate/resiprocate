#include <iostream>

#include "rutil/Id.hxx"

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

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
