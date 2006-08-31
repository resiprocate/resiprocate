#if !defined(TFM_ExpectFunctor_hxx)
#define TFM_ExpectFunctor_hxx

#define EXPECT_FUNCTOR(_class_, _name_)                         \
      class _name_ : public MessageExpectAction                 \
      {                                                         \
         public:                                                \
            explicit _name_(_class_ & endPoint)                 \
               : MessageExpectAction(endPoint),                 \
                 mEndPoint(endPoint)                            \
            {}                                                  \
                                                                \
            virtual boost::shared_ptr<resip::SipMessage>        \
            go(boost::shared_ptr<resip::SipMessage>);           \
            _class_& mEndPoint;                                 \
      }

#define EXPECT_FUNCTOR_RESPONSE(_class_, _name_, _code_)                                \
      class _name_ : public MessageExpectAction                                         \
      {                                                                                 \
         public:                                                                        \
            explicit _name_(_class_ & endPoint)                                         \
               : MessageExpectAction(endPoint),                                         \
                 mEndPoint(endPoint)                                                    \
            {}                                                                          \
                                                                                        \
            virtual boost::shared_ptr<resip::SipMessage>                                \
            go(boost::shared_ptr<resip::SipMessage> msg)                                \
            {                                                                           \
               /* !jf! use the dialog */                                                \
               if (msg->isRequest() && _code_ != 487)                                   \
               {                                                                        \
                  return mEndPoint.makeResponse(*msg, _code_);                          \
               }                                                                        \
               else                                                                     \
               {                                                                        \
                   boost::shared_ptr<resip::SipMessage> invite;                         \
                   invite = mEndPoint.getReceivedInvite(msg->header(resip::h_CallId));  \
                   if (invite == NULL) \
                     return mEndPoint.makeResponse(*msg, _code_);                          \
                   else \
                     return mEndPoint.makeResponse(*invite, _code_);                      \
               }                                                                        \
            }                                                                           \
                                                                                        \
            _class_ & mEndPoint;                                                        \
      }

#define EXPECT_FUNCTOR_TARGETED(_class_, _name_)                        \
      class _name_ : public MessageExpectAction                         \
      {                                                                 \
         public:                                                        \
            _name_(_class_ & endPoint, const resip::Uri & target)  \
                 : MessageExpectAction(endPoint),                       \
                   mEndPoint(endPoint),                                 \
                   mTarget(target)                                      \
            {}                                                          \
                                                                        \
            virtual boost::shared_ptr<resip::SipMessage>                \
            go(boost::shared_ptr<resip::SipMessage> msg)                \
            {                                                           \
               return go(msg, mTarget);                                 \
            }                                                           \
                                                                        \
            boost::shared_ptr<resip::SipMessage>                        \
            go(boost::shared_ptr<resip::SipMessage> msg,                \
                                 const resip::Uri& target);        \
                                                                        \
            _class_& mEndPoint;                                         \
            resip::Uri mTarget;                                    \
      }

#endif
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
