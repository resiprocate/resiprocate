#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include "resiprocate/ApiCheck.hxx"


#include "resiprocate/ApiCheckList.hxx"

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
                  <<std::endl
                  << "\tor NEW_MSG_HEADER_SCANNER is not defined in one of the components."
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
        abort();
        exit(bad);
    }
    std::cerr << std::endl;
}

}
