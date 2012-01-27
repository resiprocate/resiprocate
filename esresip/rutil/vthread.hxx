/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_VTHREAD_HXX)
#define RESIP_VTHREAD_HXX 

#ifdef WIN32
#error this should not be used in win32 
#endif

#include <pthread.h>

/**
   @file
   @brief Wraps vthread functionality.
   @note This should not be used in win32
   @ingroup threading
   @see Condition
   @see Mutex
   @see RWMutex
*/

typedef pthread_t vthread_t;
typedef pthread_mutex_t vmutex_t;
typedef pthread_cond_t vcondition_t;
typedef pthread_attr_t vthread_attr_t;    

/** @def vmutex_init(mutex) 
    @brief uses default mutex attributes to initialize the mutex and unlock it
     Calling this twice will result in undefined behavior.
    @param mutex pthread_mutex_t* 
    @return int 
    @retval 0 if successful
    @retval errornumber if UNsuccessful
  */
#define     vmutex_init(mutex) \
                pthread_mutex_init((mutex),0)
                

/** @def vmutex_destroy(mutex) 
    @brief destroy the mutex object. The destroyed mutex object can however
     be reinitialized using vmutex_init.
    @param mutex pthread_mutex_t* 
    @return int 
    @retval 0 if successful
    @retval errornumber if UNsuccessful
  */
#define     vmutex_destroy(mutex) \
                pthread_mutex_destroy((mutex))
                

/** @def vmutex_lock(mutex) 
    @brief Call this before executing the code that needs to run in a 
     mutually exclusively manner.
     If another thread has already called this, then this will block until 
     the mutex is unlocked.
    @param mutex pthread_mutex_t* 
    @return int 
    @retval 0 if successful
    @retval errornumber if UNsuccessful
  */
#define     vmutex_lock(mutex) \
                pthread_mutex_lock((mutex))


/** @def vmutex_unlock(mutex) 
    @brief Call this after executing the code that needs to run in a 
     mutually exclusively manner.
     If another thread owns the mutex or the mutex is already unlocked, 
     then this will return an error.
    @param mutex pthread_mutex_t* 
    @return int 
    @retval 0 if successful
    @retval errornumber if UNsuccessful
  */
#define     vmutex_unlock(mutex) \
                pthread_mutex_unlock((mutex))
                

/** @def vcond_init(cond) 
    @brief Condition variables must be declared before they can be used.
     This allows you to see the id of the created condition (cond).
    @param cond  pthread_cond_t* 
    @return int 
    @retval 0 if successful
    @retval errornumber if UNsuccessful
  */
#define     vcond_init(cond) \
                pthread_cond_init((cond),0)
                

/** @def vcond_destroy(cond) 
    @brief Condition variables can be destroyed if they will no longer be used.
    @param cond  pthread_cond_t*
    @return int 
    @retval 0 if successful
    @retval errornumber if UNsuccessful
  */
#define     vcond_destroy(cond) \
                pthread_cond_destroy((cond))
                

/** @def vmutex_wait(cond,mutex) 
    @brief Blocks the calling thread until the condition (cond) is signalled.
     After the signal is received, the mutex will be automatically locked 
     for use by the calling thread to this function.
    @param cond  pthread_cond_t*
    @param mutex pthread_mutex_t* 
    @return int 
    @retval 0 if successful
    @retval errornumber if UNsuccessful
  */
#define     vcond_wait(cond, mutex) \
                pthread_cond_wait((cond),(mutex))


/** @def vcond_timedwait(cond, mutex, timeout) 
    @brief Blocks the calling thread until the condition (cond) is signalled.
     After the signal is received, the mutex will be automatically locked 
     for use by the calling thread to this function.
     This allows you to give up waiting for the lock after a specified time.
     This returns an error if the time specified in the timeout has passed. 
     The timeout specifies time that is relative to the present (eg. 5 seconds).
    @param cond  pthread_cond_t*
    @param mutex pthread_mutex_t* 
    @param timeout const struct timespec*
    @return int 
    @retval 0 if successful
    @retval errornumber if UNsuccessful
  */
#define     vcond_timedwait(cond, mutex, timeout) \
                pthread_cond_timedwait((cond),(mutex),(timeout))


/** @def vcond_signal(cond) 
    @brief Used to wake up another thread that is waiting on the condition 
     variable to be a certain value.
    @param cond  pthread_cond_t*
    @return int 
    @retval 0 if successful
    @retval errornumber if UNsuccessful
  */
#define     vcond_signal(cond) \
                pthread_cond_signal((cond))


/** @def vcond_broadcast(cond) 
    @brief Use this instead of vcond_signal if more than one thread is in a
     blocking wait state. Useful in single producer multiple consumer cases. 
    @param cond  pthread_cond_t*
    @return int 
    @retval 0 if successful
    @retval errornumber if UNsuccessful
  */
#define     vcond_broadcast(cond) \
                pthread_cond_broadcast((cond))
                
#endif

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
