#ifndef DialogSet_hxx
#define DialogSet_hxx

#include <vector>
#include <boost/shared_ptr.hpp>

#include "resip/stack/DeprecatedDialog.hxx"

class TestSipEndPoint;

class DialogSet
{
   public:
      DialogSet(boost::shared_ptr<resip::SipMessage> msg,const TestSipEndPoint& testSipEndPoint);
      bool isMatch(boost::shared_ptr<resip::SipMessage> msg) const;

      // find or create Dialog
      void dispatch(boost::shared_ptr<resip::SipMessage> msg);
      boost::shared_ptr<resip::SipMessage> getMessage() const;
      std::vector<resip::DeprecatedDialog>& getDialogs();
         
   private:
      boost::shared_ptr<resip::SipMessage> mMsg;
      std::vector<resip::DeprecatedDialog> mDialogs;
      const TestSipEndPoint* mTestSipEndPoint;
};

#endif

// Copyright 2004 PurpleComm, Inc.

/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
