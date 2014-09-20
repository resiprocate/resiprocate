#ifndef __GPERF_MONTH_H
#define __GPERF_MONTH_H

#include "DateCategory.hxx"

namespace resip
{

struct months { const char *name; Month type; };

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
month_hash (register const char *str, register unsigned int len);

#ifdef __GNUC__
__inline
#ifdef __GNUC_STDC_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
struct months *
in_month_word_set (register const char *str, register unsigned int len);

}

#endif
