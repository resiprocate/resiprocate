#include "Dialog.hxx"
#include "Logger.hxx"

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP


Dialog::Dialog(Contact& localContact) 
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
      assert(request.get(h_Request_Line).getMethod() == INVITE ||
             request.get(h_Request_Line).getMethod() == SUBSCRIBE);
      assert (request.get(h_Contacts).size() == 1);

      mRouteSet = request.get(h_Record_Routes);
      mRemoteTarget = request.get(h_Contacts).front();
      mRemoteSequence = request.get(h_CSeq).sequence();
      mRemoteEmpty = false;
      mLocalSequence = 0;
      mLocalEmpty = true;
      mCallId = request.get(h_Call_ID);
      mLocalTag = response.get(h_To)[p_tag]; // from response
      mRemoteTag = request.get(h_From)[p_tag]; 
      mRemoteUri = request.get(h_From);
      mLocalUri = request.get(h_To);
      mCreated = true;

      mDialogId = mCallId.value();
      mDialogId += ";to-tag=";
      mDialogId += mRemoteTag;
      mDialogId += ";from-tag=";
      mDialogId += mLocalTag;
   }
}

void 
Dialog::createDialogAsUAC(SipMessage& request, SipMessage& response)
{
   if (!mCreated)
   {
      assert(request.isRequest());
      assert(response.isResponse());
      assert(request.get(h_Request_Line).getMethod() == INVITE ||
             request.get(h_Request_Line).getMethod() == SUBSCRIBE);
      assert (request.get(h_Contacts).size() == 1);

      // reverse order from response
      mRouteSet = response.get(h_Record_Routes).reverse();
      
      mRemoteTarget = response.get(h_Contacts).front();
      mRemoteSequence = 0;
      mRemoteEmpty = true;
      mLocalSequence = request.get(h_CSeq).sequence();
      mLocalEmpty = false;
      mCallId = request.get(h_Call_ID);
      mLocalTag = response.get(h_From)[p_tag];  
      mRemoteTag = response.get(h_To)[p_tag]; 
      mRemoteUri = request.get(h_To);
      mLocalUri = request.get(h_From);
      mCreated = true;
      
      mDialogId = mCallId.value();
      mDialogId += ";to-tag=";
      mDialogId += mRemoteTag;
      mDialogId += ";from-tag=";
      mDialogId += mLocalTag;
   }
}

void 
Dialog::targetRefreshResponse(SipMessage& response)
{
   if (response.get(h_Contacts).size() == 1)
   {
      mRemoteTarget = response.get(h_Contacts).front();
   }
}

int 
Dialog::targetRefreshRequest(SipMessage& request)
{
   assert (request.get(h_Request_Line).getMethod() != CANCEL);
   if (request.get(h_Request_Line).getMethod() != ACK)
   {
      unsigned long cseq = request.get(h_CSeq).sequence();
   
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

      if (request.get(h_Contacts).size() == 1)
      {
         mRemoteTarget = request.get(h_Contacts).front();
      }
   }
   
   return 0;
}

SipMessage
Dialog::makeInvite()
{
   SipMessage request(INVITE);
   request.get(h_Request_Line).
   setRequestDefaults(request);
   incrementCSeq(request);
   return request;
}

SipMessage
Dialog::makeBye()
{
   SipMessage request(BYE);
   setRequestDefaults(request);
   incrementCSeq(request);
   return request;
}


SipMessage
Dialog::makeRefer(NameAddr& referTo)
{
   SipMessage request(REFER);
   setRequestDefaults(request);
   incrementCSeq(request);
   request.get(h_Refer_To) = referTo;
   request.get(h_Referred_By) = mLocalUri;
   return request;
}

SipMessage
Dialog::makeNotify()
{
   SipMessage request(NOTIFY);
   setRequestDefaults(request);
   return request;
}


SipMessage
Dialog::makeOptions()
{
   SipMessage request(OPTIONS);
   setRequestDefaults(request);
   incrementCSeq(request);
   return request;
}

SipMessage
Dialog::makeAck(SipMessage& original)
{
   assert(mRemoteTarget != 0);

   SipMessage request(ACK);
   setRequestDefaults(request);
   copyCSeq(request);
   return request;
}

SipMessage
Dialog::makeCancel(SipMessage& original)
{
   SipMessage request(CANCEL);
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
   request.get(h_To) = mRemoteUri;
   request.get(h_To)[p_tag] = mRemoteTag;
   request.get(h_From) = mLocalUri;
   request.get(h_From)[p_tag] = mLocalTag;
   request.get(h_Call_ID) = mCallId;

   setRequestUri(request.get(h_Request_Line), mRemoteTarget);
   request.get(h_Routes) = mRouteSet;
   request.get(h_Contacts).front() = _contact;
   request.get(h_Vias).clear();
   request.get(h_Vias).front() = _via;
   request.get(h_Vias).front()[p_branch] = Helper::computeUniqueBranch();
}

void
Dialog::copyCSeq(SipMessage& request)
{
   if (mLocalEmpty)
   {
      mLocalSequence = 1;
      mLocalEmpty = false;
   }
   request.get(h_CSeq).sequence() = mLocalSequence;
}

void
Dialog::incrementCSeq(SipMessage& request)
{
   if (mLocalEmpty)
   {
      mLocalSequence = 1;
      mLocalEmpty = false;
   }
   request.get(h_CSeq).sequence()++;
}

std::ostream&
Vocal2::operator<<(std::ostream& strm, Dialog& d)
{
   strm << endl
        << "Dialog: [" << d.mDialogId << " " 
        << "created=" << d.mCreated 
        << " remoteTarget=" << d.mRemoteTarget << endl
        << "routeset=" << Inserter(d.mRouteSet) << endl
        << "remoteSeq=" << d.mRemoteSequence << ","
        << "remote=" << d.mRemoteUri << ","
        << "remoteTag=" << d.mRemoteTag << endl
        << "localSeq=" << d.mLocalSequence << ","
        << "local=" << d.mLocalUri << ","
        << "localTag=" << d.mLocalTag 
        << "]";
   return strm;
}

