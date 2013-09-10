#ifndef TARGET_HXX
#define TARGET_HXX 1

#include <list>
#include <vector>

#include "resip/stack/Uri.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/Via.hxx"
#include "resip/dum/ContactInstanceRecord.hxx"
#include "rutil/KeyValueStore.hxx"

namespace repro
{

class Target
{
   public:
   
      typedef enum
      {
         Candidate, //Transaction has not started
         Started, //Transaction has started, no final responses
         Cancelled, //Transaction has been cancelled, but no final response yet
         Terminated, //Transaction has received a final response
         NonExistent //The state of transactions that do not exist
      } Status;
   
      Target();
      Target(const resip::Uri& uri);
      Target(const resip::NameAddr& target);
      Target(const resip::ContactInstanceRecord& record);

      virtual ~Target();
      
      virtual const resip::Data& tid() const;
           
      virtual Status& status();
      virtual const Status& status() const;
      
      virtual const resip::Via& setVia(const resip::Via& via);
      virtual const resip::Via& via() const;
      
      virtual const resip::Uri& uri() const {return mRec.mContact.uri();}
      
      virtual const resip::ContactInstanceRecord& rec() const;
      virtual resip::ContactInstanceRecord& rec();
      virtual void setRec(const resip::ContactInstanceRecord& rec);
      
      virtual Target* clone() const;
      
      //In case you need const accessors to keep things happy.
      virtual int getPriority() const;
      virtual bool shouldAutoProcess() const;
      
      // Accessor for per-target extensible state storage for monkeys
      resip::KeyValueStore& getKeyValueStore() { return mKeyValueStore; }

      static bool priorityMetricCompare(const Target* lhs, const Target* rhs)
      {
         return lhs->mPriorityMetric > rhs->mPriorityMetric;
      }

      /**
         Higher value denotes higher priority.
      */
      int mPriorityMetric;
      bool mShouldAutoProcess;
      
   protected:
      Status mStatus;
      resip::Via mVia;
      resip::ContactInstanceRecord mRec;
      resip::KeyValueStore mKeyValueStore;
};// class Target

#ifdef __SUNPRO_CC
typedef std::vector<Target*> TargetPtrList;
#else
typedef std::list<Target*> TargetPtrList;
#endif

EncodeStream& 
operator<<(EncodeStream& strm, const repro::Target& t);

}// namespace repro

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
