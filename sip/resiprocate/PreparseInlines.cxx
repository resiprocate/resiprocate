
// Constants for status
const short stNone            = 0;
const short stHeadersComplete = (1 << 0);
const short stPreparseError   = (1 << 1);
const short stDataAssigned    = (1 << 2);
const short stFragmented      = (1 << 3);
// --

INLINE bool
Preparse::isDataAssigned()
{
    return mStatus & stDataAssigned;
}

INLINE bool
Preparse::isHeadersComplete()
{
    return mStatus & stHeadersComplete;
}


INLINE bool
Preparse::isFragmented()
{
    return mStatus & stFragmented;
}

INLINE size_t
Preparse::nBytesUsed()
{
    return mUsed;
}

INLINE size_t
Preparse::nDiscardOffset()
{
    return mDiscard;
}


