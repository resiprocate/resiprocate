#include "resipfaststreams.h"

#ifndef RESIP_USE_STL_STREAMS
resip::ResipStdCOStream resip::resipFastCerr(resip::ResipStdBuf::stdCerr);
resip::ResipStdCOStream resip::resipFastCout(resip::ResipStdBuf::stdCout);
#endif
resip::ResipStdCOStream resip::resipFastNull(resip::ResipStdBuf::null);

