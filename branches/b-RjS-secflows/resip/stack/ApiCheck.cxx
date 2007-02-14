#include <stdlib.h>
#include <iostream>
#include <iomanip>

#include "resip/stack/ApiCheck.hxx"
#include "resip/stack/ApiCheckList.hxx"

namespace resip
{

ApiCheck::ApiCheck( ApiEntry * list, int len)
{
    int bad = 0;

    ApiEntry* p = list;
    ApiEntry* q = ::anonymous_resipApiSizeList;
    int resipListLen = sizeof(::anonymous_resipApiSizeList)/sizeof(*::anonymous_resipApiSizeList);

    if (list == ::anonymous_resipApiSizeList )
    {
        return;
    }

    if (resipListLen != len)
    {
        std::cerr << "reSIProcate Type verification list lengths are different."
                  << std::endl
                  << "\tEither the library and application are radically out of date" 
                  << std::endl
                  << "application length: " << len << std::endl
                  << "reSIProcate length: " << resipListLen << std::endl;
        ++bad;
    }

    std::cerr << std::setfill(' ') 
              << std::setw(34) << "Class" << ' '
              << std::setw(8) << "App" << ' '
              << std::setw(8) << "Lib" << ' '
              << std::setw(8) << "Possible Culprit Flags"
              << std::endl;

    for(int i = 0 ; i < resipListLen && i < len; ++i)
    {
        bool oops = false;
        if (strcmp(p[i].name, q[i].name))
        {
            std::cerr << "!!! Miss match entry for : (app)"
                      << p[i].name << " vs. (resip)" <<  q[i].name
                      << std::endl;
            ++bad;
            continue;
        }

        if (p[i].sz != q[i].sz)
        {
            ++bad;
            oops = true;
        }
        const char w = oops?'!':' ';

        std::cerr << w << w << w << std::setfill(' ')
                  << std::setw(30-strlen(p[i].name)) << "resip::" 
                  << p[i].name << ' '
                  << std::setw(8) << p[i].sz << ' '
                  << std::setw(8) << q[i].sz << ' '
                  << (oops?p[i].culprits:"")
                  << std::endl;


    }

    if (bad)
    {
        std::cerr <<"SERIOUS COMPILATION / CONFIGURATION ERRORS -- ABORTING" << std::endl;
#ifndef UNDER_CE
        abort();
#endif
        exit(bad);
    }
    std::cerr << std::endl;
}

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
