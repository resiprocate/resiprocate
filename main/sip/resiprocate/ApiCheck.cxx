#include <stdlib.h>
#include <iostream>
#include "resiprocate/ApiCheck.hxx"


#include "resiprocate/ApiCheckList.hxx"

namespace resip
{

ApiCheck::ApiCheck(ApiEntry* list)
{
    int bad = 0;
    int clientListLength = 0;

#if defined(OLD_BUBBLE_SORT)    
    for( ; list->name ; ++list)
    {
        ++clientListLength;
        ApiEntry *localList = ::anonymous_resipApiSizeList;
        for ( ; localList->name ; ++localList)
        {
            if (!strcmp(localList->name,list->name))
            {
                const char *n = localList->name;
                // matched , check sizes.
                bool oops = localList->sz != list->sz;
                const char w = oops?'!':' ';

#if defined(RESIP_QUIET_API_CHECK)
                if (oops)
                {
#endif

                    // We are too low level (runtime-wise) to use the logging
                    // facilities, hence the direct output to cerr.
                    std::cerr << "application == " << list->sz << "\t"
                              << w << w << w << " resip == " << localList->sz 
                              <<"\t (resip::"<< n << ")" 
                              << std::endl;
                    if (oops)
                    {
                        std::cerr << "\t" << w << w << w << " Check flag(s): " << localList->culprits
                                  << std::endl;
                    }

#if defined(RESIP_QUIET_API_CHECK)
                }
#endif
                if (oops)
                {
                    ++bad;
                }

                break;
            }
        }

        if (!localList->name)
        {
            std::cerr << "ERR:" << (list->name?list->name:"(nil)")
                      << ": no matching class in Library's ApiCheckList." << std::endl;
            ++bad;
        }
    }


       // -1 for {0,0} @ end.
    int resipListLen = sizeof(::anonymous_resipApiSizeList)/sizeof(*::anonymous_resipApiSizeList)-1;


    if ( resipListLen != clientListLength)
    {
        std::cerr <<"Type verification lists are different lengths. resip == "
                  << resipListLen << ", client == " << clientListLength
                  << std::endl;
        ++bad;
    }

    if (bad)
    {
        std::cerr <<"SERIOUS COMPILATION / CONFIGURATION ERRORS -- ABORTING" << std::endl;
        abort();
        exit(bad);
    }
}

}
