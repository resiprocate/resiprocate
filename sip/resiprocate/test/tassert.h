#if !defined (TASSERT_HXX)
#define TASSERT_HXX

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

#define tassert_reset() (tassert_enabled = 1, tassert_failure = 0)
#define tassert_verify() { CritLog(<<"TASSERT: Section " << (tassert_failure ? "FAILED":"PASSED")); }

static int tassert_enabled = 1;
static int tassert_failure = 0;
static int tassert_stack[10];
const int tassert_nstack = sizeof(tassert_stack)/sizeof(*tassert_stack);
static int tassert_stack_ptr = 0;

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
#endif
