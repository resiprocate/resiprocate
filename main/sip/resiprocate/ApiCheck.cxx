#include <stdlib.h>

#include "resiprocate/ApiCheck.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

#include "resiprocate/ApiCheckList.hxx"

namespace resip
{

ApiCheck::ApiCheck(ApiEntry* list)
{
    int bad = 0;
    int clientListLength = 0;
    
    for( ; list->name ; ++list)
    {
        ++clientListLength;
        ApiEntry *localList = ::resipApiSizeList;
        for ( ; localList->name ; ++localList)
        {
            if (!strcmp(localList->name,list->name))
            {
                const char *n = localList->name;
                // matched , check sizes.
                bool oops = localList->sz != list->sz;
                const char w = oops?'!':' ';

                InfoLog(<< "application == " << list->sz << "\t" << w << w << w << " resip == " << localList->sz 
                       <<"\t (resip::"<< n << ")");

                if (oops) ++bad;

                break;
            }
        }
        if (!localList->name)
        {
            ErrLog(<< list->name << ": no matching class in Library.");
            ++bad;
        }
    }

    if (sizeof(::resipApiSizeList)/sizeof(*::resipApiSizeList) != clientListLength)
    {
        CritLog(<<"Type verification lists are different lengths.");
        ++bad;
    }

    if (bad)
    {
        CritLog(<<"SERIOUS COMPILATION / CONFIGURATION ERRORS -- ABORTING");
        abort();
        exit(bad);
    }
}

}
