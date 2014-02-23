#if !defined(RESIP_PERSISTENTMESSAGEQUEUE_HXX)
#define RESIP_PERSISTENTMESSAGEQUEUE_HXX 

// Note: Using this define will remove the dependency on BerkeleyDB, however it will render the AccountingCollector useless
#ifndef DISABLE_BERKELEYDB_USE

#ifdef WIN32
#include <db_cxx.h>
#elif HAVE_CONFIG_H
#include "config.h"
#include DB_HEADER
//#elif defined(__APPLE__) 
//#include <db42/db_cxx.h>
#else
#include <db_cxx.h>
#endif

#endif

#include "rutil/Data.hxx"
#include <vector>

// This class implements a persistent message queue that utilizes a BerkeleyDb backing store.
// Messages are persisted out to disk and can be consumed from another process using the 
// PeristentMessageDequeue class.
//
// If PersistentMessageEnqueue::push or 
//    PersistentMessageDequeue::pop or 
//    PersistentMessageDequeue::commit
// even fail, then the isRecoveryNeeded should be called.  If this returns true, then the 
// PersistentMessageQueue in use should be destroyed and a new one created in order to "recover"
// the backing store.
//
// Warning:  If autoCommit is not used on PersistentMessageDequeue::pop then there can only be 1
//           consumer.
//
// Producer example:
// -----------------
// PersistentMessageEnqueue* queue = new PersistentMessageEnqueue;
// if(queue->init(true, "msgqueue"))
// {
//    for(int i = 0; i < 1000; i++)
//    {
//       Data qstr("test string" + Data(i));
//       if(!queue->push(qstr))
//       {
//          if(queue->isRecoveryNeeded())
//          {
//             delete queue;
//             queue = new PersistentMessageEnqueue;
//             if(!queue->init(true, "msgqueue")) break;
//             if(queue->push(qstr))
//             {
//                cout << "Queued: " << qstr << endl;
//             }
//             else
//             {
//                cout << "Error queuing - exiting!" << endl;
//                break;
//             }
//          }
//          else
//          {
//             cout << "Error queuing - exiting!" << endl;
//             break;
//          }
//       }
//       else
//       {
//          cout << "Queued: " << qstr << endl;
//       }
//   }
//}
//delete queue;
//
//
// Consumer example:
// -----------------
//PersistentMessageDequeue* queue = new PersistentMessageDequeue;
//if(queue->init(true, "msgqueue"))
//{
//   vector<resip::Data> recs;
//   while(true)
//   {
//      if(queue->pop(5, recs, false))
//      {
//         if(recs.size() > 0)
//         {
//            for(int i = 0; i < recs.size(); i++)
//            {
//               cout << "Popped(" << i << "): " << recs[i] << endl;
//            }
//            queue->commit();
//         }
//         else
//         {
//            cout << "No records to pop." << endl;
//            sleepMs(1000);
//            //break;
//         }
//      }
//      else 
//      {
//         if(queue->isRecoveryNeeded())
//         {
//            delete queue;
//            queue = new PersistentMessageDequeue;
//            if(!queue->init(true, "msgqueue")) break;
//         }
//         else
//         {
//            cout << "Error Dequeuing!" << endl;
//            break;
//         }
//      }
//   }
//}
//delete queue;


namespace repro
{
#ifndef DISABLE_BERKELEYDB_USE
class PersistentMessageQueue : public DbEnv 
#else
class PersistentMessageQueue
#endif
{ 
public:     
   PersistentMessageQueue(const resip::Data& baseDir);
   virtual ~PersistentMessageQueue();

   bool init(bool sync, const resip::Data& queueName);
   bool isRecoveryNeeded();

protected:
#ifndef DISABLE_BERKELEYDB_USE
   Db* mDb;
#endif
   resip::Data mBaseDir;
   bool mRecoveryNeeded;
};  

class PersistentMessageEnqueue : public PersistentMessageQueue 
{ 
public:
   PersistentMessageEnqueue(const resip::Data& baseDir) : 
      PersistentMessageQueue(baseDir) {}
   virtual ~PersistentMessageEnqueue() {}
   
   // Note:  this has a potential to block if the a consumer crashes and leaves a lock open on the database (deadlock)
   // typically restarting the consumer will "recover" the "dead" lock and allow this call to unblock
   bool push(const resip::Data& data);
};  

class PersistentMessageDequeue : public PersistentMessageQueue 
{ 
public:     
   PersistentMessageDequeue(const resip::Data& baseDir) : 
      PersistentMessageQueue(baseDir), 
      mNumRecords(0) {}
   virtual ~PersistentMessageDequeue () {}

   // returns true for success, false for failure - can return true and 0 records if none available
   // Note:  if autoCommit is used then it is safe to allow multiple consumers
   bool pop(size_t numRecords, std::vector<resip::Data>& records, bool autoCommit);
   bool commit();
   void abort();

private:
   size_t mNumRecords;
 }; 

}
#endif  

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
