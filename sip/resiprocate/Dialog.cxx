#include <iostream>
#include <sipstack/Dialog.hxx>
#include <sipstack/Logger.hxx>
#include <sipstack/Helper.hxx>

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP


Dialog::Dialog(Url& localContact) 
   : mVia(),
     mContact(localContact),
     mCreated(false),
     mRouteSet(),
     mRemoteTarget(),
     mRemoteSequence(0),
     mRemoteEmpty(true),
     mLocalSequence(0),
     mLocalEmpty(true),
     mCallId(),
     mLocalTag(),
     mRemoteTag(),
     mRemoteUri(),
     mLocalUri(),
     mDialogId()
{
   //DebugLog (<< "Creating a dialog: " << localContact << " " << this);
   mVia.sentHost() = localContact.host();
   mVia.sentPort() = localContact.port();
   mVia.transport() = localContact[p_transport];
#if 0
   if (mVia.transport().size() == 0)
   {
      mVia.transport() = Vocal::DEFAULT_TRANSPORT); // !jf!
   }
#endif
}

void
Dialog::createDialogAsUAS(SipMessage& request, SipMessage& response)
{
   if (!mCreated)
   {
      assert(request.isRequest());
      assert(response.isResponse());
      assert(request.header(h_RequestLine).getMethod() == INVITE ||
             request.header(h_RequestLine).getMethod() == SUBSCRIBE);
      assert (request.header(h_Contacts).size() == 1);

      mRouteSet = request.header(h_RecordRoutes);
      mRemoteTarget = request.header(h_Contacts).front();
      mRemoteSequence = request.header(h_CSeq).sequence();
      mRemoteEmpty = false;
      mLocalSequence = 0;
      mLocalEmpty = true;
      mCallId = request.header(h_CallId);
      mLocalTag = response.header(h_To)[p_tag]; // from response
      mRemoteTag = request.header(h_From)[p_tag]; 
      mRemoteUri = request.header(h_From);
      mLocalUri = request.header(h_To);
      mCreated = true;

      mDialogId.value() = mCallId.value();
      mDialogId[p_toTag] = mRemoteTag;
      mDialogId[p_fromTag] = mLocalTag;
   }
}

void 
Dialog::createDialogAsUAC(SipMessage& request, SipMessage& response)
{
   if (!mCreated)
   {
      assert(request.isRequest());
      assert(response.isResponse());
      assert(request.header(h_RequestLine).getMethod() == INVITE ||
             request.header(h_RequestLine).getMethod() == SUBSCRIBE);
      assert (request.header(h_Contacts).size() == 1);

      // reverse order from response
      mRouteSet = response.header(h_RecordRoutes).reverse();
      
      mRemoteTarget = response.header(h_Contacts).front();
      mRemoteSequence = 0;
      mRemoteEmpty = true;
      mLocalSequence = request.header(h_CSeq).sequence();
      mLocalEmpty = false;
      mCallId = request.header(h_CallId);
      mLocalTag = response.header(h_From)[p_tag];  
      mRemoteTag = response.header(h_To)[p_tag]; 
      mRemoteUri = request.header(h_To);
      mLocalUri = request.header(h_From);
      mCreated = true;
      
      mDialogId.value() = mCallId.value();
      mDialogId[p_toTag] = mRemoteTag;
      mDialogId[p_fromTag] = mLocalTag;
   }
}

void 
Dialog::targetRefreshResponse(SipMessage& response)
{
   if (response.header(h_Contacts).size() == 1)
   {
      mRemoteTarget = response.header(h_Contacts).front();
   }
}

int 
Dialog::targetRefreshRequest(SipMessage& request)
{
   assert (request.header(h_RequestLine).getMethod() != CANCEL);
   if (request.header(h_RequestLine).getMethod() != ACK)
   {
      unsigned long cseq = request.header(h_CSeq).sequence();
   
      if (mRemoteEmpty)
      {
         mRemoteSequence = cseq;
         mRemoteEmpty = false;
      }
      else if (cseq < mRemoteSequence)
      {
         return 500; // !jf! should be exception
      }
      else
      {
         mRemoteSequence = cseq;
      }

      if (request.header(h_Contacts).size() == 1)
      {
         mRemoteTarget = request.header(h_Contacts).front();
      }
   }
   
   return 0;
}

SipMessage
Dialog::makeInvite()
{
   SipMessage request;
   request.header(h_RequestLine) = RequestLine(INVITE);
   setRequestDefaults(request);
   incrementCSeq(request);
   return request;
}

SipMessage
Dialog::makeBye()
{
   SipMessage request;
   request.header(h_RequestLine) = RequestLine(BYE);
   setRequestDefaults(request);
   incrementCSeq(request);
   return request;
}


SipMessage
Dialog::makeRefer(Url& referTo)
{
   SipMessage request;
   request.header(h_RequestLine) = RequestLine(REFER);
   setRequestDefaults(request);
   incrementCSeq(request);
   request.header(h_ReferTo) = referTo;
   request.header(h_ReferredBy) = mLocalUri;
   return request;
}

SipMessage
Dialog::makeNotify()
{
   SipMessage request;
   request.header(h_RequestLine) = RequestLine(NOTIFY);
   setRequestDefaults(request);
   return request;
}


SipMessage
Dialog::makeOptions()
{
   SipMessage request;
   request.header(h_RequestLine) = RequestLine(OPTIONS);
   setRequestDefaults(request);
   incrementCSeq(request);
   return request;
}

SipMessage
Dialog::makeAck(SipMessage& original)
{
   SipMessage request;
   request.header(h_RequestLine) = RequestLine(ACK);
   setRequestDefaults(request);
   copyCSeq(request);
   return request;
}

SipMessage
Dialog::makeCancel(SipMessage& original)
{
   SipMessage request;
   request.header(h_RequestLine) = RequestLine(CANCEL);
   setRequestDefaults(request);
   copyCSeq(request);
   return request;
}

void
Dialog::clear()
{
   mCreated = false;
   mRouteSet.clear();
   mRemoteEmpty = true;
   mLocalEmpty = true;
   mLocalTag = "";
   mRemoteTag = "";
   mRemoteUri = 0;
   mLocalUri = 0;
   mRemoteTarget = 0;
}


void 
Dialog::setRequestDefaults(SipMessage& request)
{
   assert(mCreated);
   request.header(h_To) = mRemoteUri;
   request.header(h_To)[p_tag] = mRemoteTag;
   request.header(h_From) = mLocalUri;
   request.header(h_From)[p_tag] = mLocalTag;
   request.header(h_CallId) = mCallId;

   Helper::setUri(request.header(h_RequestLine), mRemoteTarget);
   request.header(h_Routes) = mRouteSet;
   request.header(h_Contacts).front() = mContact;
   request.header(h_Vias).clear();
   request.header(h_Vias).front() = mVia;
   request.header(h_Vias).front()[p_branch] = Helper::computeUniqueBranch();
}

void
Dialog::copyCSeq(SipMessage& request)
{
   if (mLocalEmpty)
   {
      mLocalSequence = 1;
      mLocalEmpty = false;
   }
   request.header(h_CSeq).sequence() = mLocalSequence;
}

void
Dialog::incrementCSeq(SipMessage& request)
{
   if (mLocalEmpty)
   {
      mLocalSequence = 1;
      mLocalEmpty = false;
   }
   request.header(h_CSeq).sequence()++;
}

std::ostream&
Vocal2::operator<<(std::ostream& strm, Dialog& d)
{
   strm << std::endl
        << "Dialog: [" << d.mDialogId << " " 
        << "created=" << d.mCreated 
        << " remoteTarget=" << d.mRemoteTarget << std::endl
      //<< "routeset=" << Inserter(d.mRouteSet) << std::endl
        << "remoteSeq=" << d.mRemoteSequence << ","
        << "remote=" << d.mRemoteUri << ","
        << "remoteTag=" << d.mRemoteTag << std::endl
        << "localSeq=" << d.mLocalSequence << ","
        << "local=" << d.mLocalUri << ","
        << "localTag=" << d.mLocalTag 
        << "]";
   return strm;
}

