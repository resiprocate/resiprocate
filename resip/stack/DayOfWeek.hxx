#ifndef __GPERF_DAYOFWEEK_H
#define __GPERF_DAYOFWEEK_H

#include "DateCategory.hxx"

namespace resip
{

struct days { char name[32]; DayOfWeek day; };

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
dayofweek_hash (register const char *str, register unsigned int len);

#ifdef __GNUC__
__inline
#endif
struct days *
in_dayofweek_word_set (register const char *str, register unsigned int len);

}

#endif

