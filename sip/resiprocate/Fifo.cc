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


static const char* const Fifo_cc_Version =
    "$Id: Fifo.cc,v 1.2 2002/09/21 23:31:18 dabryan Exp $";

#include <errno.h>

template <class Msg>
Fifo<Msg>::Fifo()
{
}

template <class Msg>
Fifo<Msg>::~Fifo()
{
}

template <class Msg>
void
Fifo<Msg>::add(Msg* msg)
{
    Lock lock(mMutex); (void)lock;
    mFifo.push_back(msg);
    wakeup();
}

template <class Msg>
Msg*
Fifo<Msg> ::getNext()
{
    Lock lock(mMutex); (void)lock;

    // Wait while there are no messages available.
    //
    while ( !messageAvailableNoLock() )
    {
        blockNoLock();
    }

    // Return the first message on the fifo.
    //
    Msg* firstMessage = mFifo.front();
    mFifo.pop_front();

    return ( firstMessage );
}


template <class Msg>
unsigned int
Fifo<Msg> ::size() const
{
    Lock lock(mMutex); (void)lock;

    return mFifo.size(); 
}

template <class Msg>
bool
Fifo<Msg>::messageAvailable() const
{
    Lock lock(mMutex); (void)lock;
    
    return ( messageAvailableNoLock() );
}

template <class Msg>
bool
Fifo<Msg>::messageAvailableNoLock() const
{
    return ( mFifo.size() > 0);
}

template <class Msg>
int
Fifo<Msg>::blockNoLock()
{
    // Wait for an event. A timer expiry or signal will return a 0 here.
    //
    int numberActive = wait();

    if ( numberActive > 0 )
    {
        return ( numberActive );
    }

    // See if a timer has expired.  If it has expired, return 1 to indicate
    // a message is available from the timer.
    //
    if ( messageAvailableNoLock() )
    {
        return ( 1 );
    }

    // To get here, either a signal woke us up, or the we used the
    // relativeTimeout value, meaning that a message isn't available from
    // the timer container.
    //
    return ( 0 );
}


template <class Msg>
void
Fifo<Msg>::wakeup()
{
    // Tell any waiting threads the FIFO is no longer empty
    //
    int error = mCondition.signal();
    assert( !error );
}


template <class Msg>
int
Fifo<Msg>::wait()
{
    // This will unlock the object's mutex, wait for a condition's signal,
    // and relock.
    //
    int error = mCondition.wait(&mMutex);

    if ( error != 0 )
    {
        // If we have been awaken due to a unix signal, i.e. SIGTERM,
        // or a timeout return 0 to indicate no activity.
        //
        if ( error == EINTR || error == ETIMEDOUT )
        {
            return ( 0 );
        }

        // Defensive, shouldn't ever get here.
        //
        else
        {
            assert( 0 );
        }
    }

    return ( mFifo.size() );
}
