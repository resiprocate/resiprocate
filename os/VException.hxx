#ifndef VEXCEPTION_HXX
#define VEXCEPTION_HXX

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

static const char* const VExceptionHeaderVersion =
    "$Id: VException.hxx,v 1.1 2002/09/26 21:58:40 jason Exp $";

#include <exception>
#include <iostream>


#include <util/Data.hxx>


namespace Vocal2
{


/** Vocal specific exception.
 */
class VException : public std::exception
{
    protected:

        /** Creates an exception object, should be called with a
         ** descriptive msg, the filename and line number where the
         ** exception occured, and optionally an error code asscociated
         ** with the exception
         ** Note, the log() method handles the logging format, so
         ** blank spaces, newlines, etc. should NOT be included in the
         ** parameters to the constructor.
         */
        VException( const Vocal2::Data& msg,
                    const Vocal2::Data& file,
                    const int line,
                    const int error = 0);

        /** destroys the exception object
         */
        ~VException() throw();

        /** Returns the predefined name of the exception
         */
        virtual Vocal2::Data getName() const = 0;

        /** Logs the exception including the error code (if any) as well
         ** as the filename and line number where the exception
         ** occurred.
         **
         ** The logging is formatted as follows:
         ** <msg> [:<error>] at <file>:<line> 
         */
        void log() const;

    public:

        /** Returns the predefined name and user supplied msg
         */
        Vocal2::Data getDescription() const;

        /** Returns the error code supplied with the exception.
         */
        int getError() const;

    protected:
        /// user supplied msg
        Vocal2::Data message;

        /// file in which exception occurred
        Vocal2::Data fileName;

        /// line number at which exception occurred
        int lineNumber;

        /// user supplied error code (optional)
        int errorCode;
};

std::ostream& operator<<(std::ostream& strm, const VException& e);

/** Generic exception classes
 *  Note, class specific exception classes should be defined in
 *  separate <ClassName>Exception.[ch]xx files
 */
class VExceptionMemory: protected VException
{
    public:
        VExceptionMemory( const Vocal2::Data& msg,
                          const Vocal2::Data& file,
                          const int line,
                          const int error = 0 );

        Vocal2::Data getName( void ) const;
};
 
 
}


// VEXCEPTION_HXX
#endif
