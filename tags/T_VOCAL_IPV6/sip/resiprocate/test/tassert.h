#if !defined (TASSERT_HXX)
#define TASSERT_HXX

#if 0 // If you are having bib memory problems with your compiler - try putting this to 1
#define tassert assert
#else
#define tassert(expr)                           \
if ( tassert_enabled )                          \
{                                               \
    if (!(expr))                                \
    {                                           \
        CritLog(<<"TESTASSERT: "#expr);         \
        tassert_enabled = 0;                    \
        tassert_failure = 1;                    \
    }                                           \
}                                               \
else                                            \
{                                               \
    CritLog(<<"SUPRESSED: "#expr);              \
}
#endif
#define tassert_broken(x) tassert(!(x))
#define tassert_reset() (tassert_enabled = 1, tassert_failure = 0)

#define tassert_verify(x) {                                                \
  CritLog(<<"TASSERT: Section " << (tassert_failure ? "FAILED":"PASSED")); \
  assert(x > 0 && x <= tassert_ncases);                                    \
  tassert_results[x-1].result = !tassert_failure;                          \
  tassert_results[x-1].ran = true;                                         \
}


static int tassert_enabled = 1;
static int tassert_failure = 0;
static int tassert_stack[10];
const int tassert_nstack = sizeof(tassert_stack)/sizeof(*tassert_stack);
static int tassert_stack_ptr = 0;
const int tassert_max_cases = 100;
static int tassert_ncases = 0;

struct {
      bool ran;
      bool result;
} tassert_results[tassert_max_cases];

#define tassert_push()                                          \
{                                                               \
    assert(tassert_stack_ptr < tassert_nstack - 2);             \
    tassert_stack[tassert_stack_ptr++] = tassert_enabled;       \
    tassert_stack[tassert_stack_ptr++] = tassert_failure;       \
}


#define tassert_pop()                                           \
{                                                               \
    assert(tassert_stack_ptr > 1);                              \
    tassert_failure = tassert_stack[--tassert_stack_ptr] ;      \
    tassert_enabled = tassert_stack[--tassert_stack_ptr];       \
}

#define tassert_init(nCases)                    \
{                                               \
                                                \
  assert(nCases < tassert_max_cases);           \
  tassert_ncases = nCases;                      \
  for (int i = 0 ; i < tassert_ncases ; i++)    \
  {                                             \
      tassert_results[i].ran = false;           \
  }                                             \
}

#define tassert_report()                                                \
{                                                                       \
   bool overall = true;                                                 \
   for(int i = 0; i < tassert_ncases ; i++)                             \
   {                                                                    \
      if (!tassert_results[i].ran)                                      \
      {                                                                 \
         CritLog(<<"CASE " << i+1 << ": no results reported!");         \
         overall = false;                                               \
      }                                                                 \
      else                                                              \
      {                                                                 \
         bool res = tassert_results[i].result;                          \
         overall = overall && res;                                      \
         CritLog(<<"CASE "<<i+1<<": " << (res?"PASSED":"FAILED ***"));  \
      }                                                                 \
   }                                                                    \
   CritLog(<<"OVERALL RESULTS: " << (overall?"PASSED":"FAILED ***"));   \
}
#endif

