
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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


static const char* const VException_cxx_Version =
    "$Id: VException.cxx,v 1.1 2002/09/26 21:58:40 jason Exp $";


#include "global.h"
#include "cpLog.h"
#include "VException.hxx"

using namespace Vocal2;
using namespace std;


VException::VException( const Data& msg,
                        const Data& file,
                        const int line,
                        const int error /*Default Argument*/):
        message( msg ),
        fileName( file ),
        lineNumber( line ),
        errorCode( error )
{
   cpLog(LOG_DEBUG, "Exception at %s:%d %s", file.c_str(), line, message.c_str());
}


VException::~VException()
    throw()
{
}

Data
VException::getDescription( void ) const
{
    return ( getName() + ": " + message );
}

int
VException::getError() const
{
    return ( errorCode );
}

void
VException::log( void ) const
{
    if ( errorCode )
    {
        cpLog( LOG_DEBUG,
               "%s: %d at %s:%d\n",
               getDescription().c_str(),
               errorCode,
               fileName.c_str(),
               lineNumber );
    }
    else
    {
        cpLog( LOG_DEBUG,
               "%s at %s:%d\n",
               getDescription().c_str(),
               fileName.c_str(),
               lineNumber );
    }
}

ostream& Vocal2::operator<<(ostream& strm, const VException& e)
{
   strm << e.getDescription();
   return strm;
}

VExceptionMemory::VExceptionMemory(
    const Data& msg,
    const Data& file,
    const int line,
    const int error /*Default Argument*/)
        : VException( msg, file, line, error )
{
    log();
}

Data
VExceptionMemory::getName( void ) const
{
    return "VExceptionMemory";
}
