
#include "resip/dumer/PageModePrd.hxx"
#include "resip/dumer/EncryptionLevel.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

PageModePrd::PageModePrd()
{
}

PageModePrd::~PageModePrd()
{
}

SipMessage&
PageModePrd::initialize(const NameAddr& target)
{
   // Initialize a request for UAC
   makeInitialRequest(target, MESSAGE);

   //rfc3428 section 9 - remove the header that may have been added by the BaseCreator and are not allowed
   mLastRequest->remove(h_Supporteds);
   mLastRequest->remove(h_AcceptEncodings);
   mLastRequest->remove(h_AcceptLanguages);
   mLastRequest->remove(h_Contacts);

   return mLastRequest;
}

void 
PageModePrd::protectedDispatch(std::auto_ptr<SipMessage msg)
{
   if (msg->isRequest())
   {
      onMessageArrived(*msg);
   }
   else
   {
      assert(msg->isResponse());

      int code = msg->header(h_StatusLine).statusCode();

      DebugLog ( << "ClientPagerMessageReq::dispatch(msg)" << msg->brief() );

      assert(mOutboundMsgQueue.empty() == false);
      if (code < 200)
      {
         DebugLog ( << "ClientPagerMessageReq::dispatch - encountered provisional response" << msg->brief() );
      }
      else if (code < 300)
      {
         if(mOutboundMsgQueue.empty() == false)
         {
            delete mOutboundMsgQueue.front().contents;
            mOutboundMsgQueue.pop_front();
            if(mOutboundMsgQueue.empty() == false)
            {
               pageFirstMsgQueued();
            }

            onSuccess(*msg);
         }
      }
      else
      {
         SipMessage errResponse;
         MsgQueue::iterator contents;
         for(contents = mOutboundMsgQueue.begin(); contents != mOutboundMsgQueue.end(); ++contents)
         {
            Contents* p = contents->contents;
            WarningLog ( << "Paging failed" << *p );
            Helper::makeResponse(errResponse, *mRequest, code);
            onFailure(errResponse, std::auto_ptr<Contents>(p));
            contents->contents = 0;
         }
         mOutboundMsgQueue.clear();
      }
   }
}

void
PageModePrd::protectedDispatch(std::auto_ptr<DumTimeout>)
{
}

void
PageModePrd::clearMsgQueued ()
{
   MsgQueue::iterator it = mOutboundMsgQueue.begin();
   for(; contents != mOutboundMsgQueue.end(); ++contents)
   {
      Contents* p = contents->contents;
      delete p;
   }
   mOutboundMsgQueue.clear();
}

void
PageModePrd::pageFirstMsgQueued ()
{
   assert(!mOutboundMsgQueue.empty());

   mLastRequest.header(h_CSeq).sequence()++;
   mLastRequest->setContents(mOutboundMsgQueue.front().contents);
   DumHelper::setOutgoingEncryptionLevel(*mLastRequest, mOutboundMsgQueue.front().encryptionLevel);
   DebugLog(<< "ClientPagerMessage::pageFirstMsgQueued: " << *mLastRequest);
   send(mLastRequest);
}

void
PageModePrd::page(std::auto_ptr<Contents> contents,
                  EncryptionLevel level)
{
    assert(contents.get() != 0);
    bool do_page = mOutboundMsgQueue.empty();
    Item item;
    item.contents = contents.release();
    item.encryptionLevel = level;
    mOutboundMsgQueue.push_back(item);
    if(do_page)
    {
       pageFirstMsgQueued();
    }
}

SipMessage&
PageModePrd::accept(int statusCode)
{   
   //!dcm! -- should any responses include a contact?
   makeResponse(mResponse, mRequest, statusCode);
   mResponse->remove(h_Contacts);   
   return mResponse;
}

SipMessage&
PageModePrd::reject(int statusCode)
{
   //!dcm! -- should any responses include a contact?
   makeResponse(mResponse, mRequest, statusCode);
   return mResponse;
}

void 
ServerPagerMessage::send(SharedPtr<SipMessage> response)
{
   // Use Page to send a request
   assert(response->isResponse());

   send(response);
}

void
PageModePrd::end()
{
   unmanage();
}