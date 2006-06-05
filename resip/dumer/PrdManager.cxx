#include "resip/dumer/PrdManager.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

PrdManager::PrdManager(SipStack& stack, 
                       SharedPtr<MasterProfile> profile, 
                       bool createDefaultFeatures) : 
   TransactionUser(RegisterForTransactionTermination) // !jf! conn term
   mStack(stack),
   mMasterProfile(profile)
{
   mStack.registerTransactionUser(*this);
}


PrdManager::~PrdManager()
{
}


void
PrdManager::requestTerminate()
{
   InfoLog (<< "request shutdown");   
   // this instructs the TU to no longer accept new transactions. There is no
   // event in response to this. 
   mStack.requestTransactionUserShutdown(*this);
   
   // !jf! go through all PRDs in some order and request that they go away. 
}

void
PrdManager::onTerminated()
{
   requestShutdown();
}

void 
PrdManager::requestShutdown()
{
   InfoLog (<< "request shutdown");
   // !jf! assert that there are no PRDs in the PrdManager

   // an event will come back from SipStack when this is completed. 
   // TransactionUserMessage->type() == TransactionUserMessage::TransactionUserRemoved
   mShutdownState = RemovingTransactionUser;
   mStack.unregisterTransactionUser(*this);
}

const Data& 
PrdManager::name() const
{
   static Data n("PrdManager");
   return n;
}

SipStack& 
PrdManager::getSipStack()
{
   return mStack;
}

void 
PrdManager::unmanage(Prd& prd)
{
   mFifo.post(new UnmanagePrdManagerCommand(*this, prd.getId()));
}

void 
PrdManager::send(SharedPtr<SipMessage> msg)
{
   std::auto_ptr<SipMessage> toSend(static_cast<SipMessage*>(msg->clone()));
   mFifo.post(new SendPrdManagerCommand(*this, toSend));
}

SharedPtr<MasterProfile> 
PrdManager::getMasterProfile()
{
   return mMasterProfile;
}

void 
PrdManager::post(std::auto_ptr<PrdCommand> cmd, Postable& postable, unsigned long timeMs)
{
   RussianDollPrdManagerCommand* rd = new RussianDollPrdManagerCommand(cmd, postable);
   mStack.postMS(*(cmd->release()), timeMs, *this);
}

void 
PrdManager::setInviteSessionFactory(std::auto_ptr<InviteSessionFactory> f)
{
   assert(!mInviteSessionFactory.get());
   mInviteSessionFactory = f;
   mAllowedMethods.insert(INVITE);
   mAllowedMethods.insert(INFO);
   mAllowedMethods.insert(ACK);
   mAllowedMethods.insert(CANCEL);
}

void 
PrdManager::setSubscriptionFactory(const Data& eventType, 
                                   std::auto_ptr<ServerSubscriptionFactory> f)
{
   assert(mSubscriptionFactory[eventType].get() == 0);
   mSubscriptionFactory[eventType] = f;
   mAllowedMethods.insert(SUBSCRIBE);
   mAllowedMethods.insert(NOTIFY);
}

void 
PrdManager::setRegistrationFactory(std::auto_ptr<ServerRegistrationFactory> f)
{
   assert(mRegistrationFactory.get() == 0);
   mRegistrationFactory = f;
   mAllowedMethods.insert(REGISTER);
}

void 
PrdManager::setPublicationFactory(const Data& eventType, 
                                  std::auto_ptr<ServerPublicationFactory> f)
{
   assert(mPublicationFactory[eventType].get() == 0);
   mPublicationFactory[eventType] = f;
   mAllowedMethods.insert(PUBLISH);
}

void 
PrdManager::setPrdServerTransactionFactory(MethodTypes method, 
                                           std::auto_ptr<PrdServerTransactionFactory> f)
{
   assert (method != REGISTER);
   assert (method != SUBSCRIBE);
   assert (method != INVITE);
   assert (method != ACK);
   assert (method != CANCEL);
   assert (method != PUBLISH);
   assert (method != NOTIFY);
   assert (method != MESSAGE);
   
   assert(mPrdServerTransactionFactory[method].get() == 0);
   mPrdServerTransactionFactory[method] = f;
   mAllowedMethods.insert(method);
}

void 
PrdManager::setPageModePrdFactory(std::auto_ptr<PageModePrdFactory> f)
{
   assert(mPageModePrdFactory.get() == 0);
   mPageModePrdFactory = f;
   mAllowedMethods.insert(MESSAGE);
}

bool 
PrdManager::process()
{
   if (mFifo.messageAvailable())
   {
      internalProcess(std::auto_ptr<Message>(mFifo.getNext()));
   }
   return mFifo.messageAvailable();
}

void
PrdManager::internalProcess(std::auto_ptr<Message> msg)
{
   // After a Stack ShutdownMessage has been received, don't do anything else in dum
   if (mShutdownState == Shutdown)
   {
      return;
   }
   
   TransactionUserMessage* tuMsg = dynamic_cast<TransactionUserMessage*>(msg.get());
   if (tuMsg)
   {
      InfoLog (<< "TU unregistered ");
      assert(mShutdownState == RemovingTransactionUser);
      assert(tuMsg->type() == TransactionUserMessage::TransactionUserRemoved);
      mShutdownState = Shutdown;
      onShutdown();
      return;
   }
   
   PrdManagerCommand* rud = dynamic_cast<PrdManagerCommand*>(msg.get());
   if (prd)
   {
      (*prd)();
      return;
   }
   
   KeepAliveTimeout* keepAliveMsg = dynamic_cast<KeepAliveTimeout*>(msg.get());
   if (keepAliveMsg)
   {
      if (mKeepAliveManager.get())
      {
         mKeepAliveManager->process(*keepAliveMsg);
      }
      return;      
   }

   ConnectionTerminated* terminated = dynamic_cast<ConnectionTerminated*>(msg.get());
   if (terminated)
   {
      DebugLog(<< "connection terminated message");
      if (mConnectionTerminatedEventDispatcher.dispatch(msg.get()))
      {
         msg.release();
      }
      return;
   }

   std::auto_ptr<SipMessage>(static_cast<SipMessage>(msg.release()));
   
   TransactionTerminated* tterm = dynamic_cast<TransactionTerminated*>(msg.get());
   if (tterm)
   {
      if (!tterm->isClientTransaction())
      {
         mCancelMap.erase(tterm->getTransactionId());
         mMergedRequestsByTransaction.erase(tterm->getTransactionId());
      }
   }
   
   // !jf! run the incoming features here
   
   incomingProcess(msg);
}

void 
PrdManager::incomingProcess(std::auto_ptr<Message> msg)
{
   std::auto_ptr<SipMessage> sipMsg(dynamic_cast<SipMessage>(msg.release()));
   if (sipMsg.get())
   {
      if (sipMsg->isRequest())
      {
         // Validate Request URI
         if( !validateRequestURI(*sipMsg) )
         {
            DebugLog (<< "Failed RequestURI validation " << *sipMsg);
            return;
         }

         // Continue validation on all requests, except ACK and CANCEL
         if(sipMsg->header(h_RequestLine).method() != ACK &&
            sipMsg->header(h_RequestLine).method() != CANCEL)
         {
            if( !validateRequiredOptions(*sipMsg) )
            {
               DebugLog (<< "Failed required options validation " << *sipMsg);
               return;
            }
            if( getMasterProfile()->validateContentEnabled() && !validateContent(*sipMsg) )
            {
               DebugLog (<< "Failed content validation " << *sipMsg);
               return;
            }
            if( getMasterProfile()->validateAcceptEnabled() && !validateAccept(*sipMsg) )
            {
               DebugLog (<< "Failed accept validation " << *sipMsg);
               return;
            }
         }

         if (!request.header(h_To).exists(p_tag) && mergeRequest(*sipMsg) )
         {
            InfoLog (<< "Merged request: " << *sipMsg);
            return;
         }
         processRequest(sipMsg);
      }
      else
      {
         processResponse(sipMsg);
      }
   }
   catch(BaseException& e)
   {
      //unparseable, bad 403 w/ 2543 trans it from FWD, etc
      ErrLog(<<"Illegal message rejected: " << e.getMessage());
   }
}


void
PrdManager::processRequest(std::auto_ptr<SipMessage> request)
{
   DebugLog ( << "DialogUsageManager::processRequest: " << request.brief());

   if (mShutdownState != Running && mShutdownState != ShutdownRequested)
   {
      WarningLog (<< "Ignoring a request since we are shutting down " << request->brief());

      SipMessage failure;
      makeResponse(failure, *request, 480, "UAS is shutting down");
      sendResponse(failure);
      return;
   }
   
   // CANCEL processing as well as handle requests that having matching Prd in
   // the mPrdMap
   {
      PrdId id(*request);
      if (request->header(h_RequestLine).getMethod() == CANCEL)
      {
         CancelMap::iterator i = mCancelMap.find(request.getTransactionId());
         if (i != mCancelMap.end())
         {
            id = mPrdMap.find(i->second);
         }
         else
         {
            SipMessage failure;
            makeResponse(failure, *request, 481, "No matching transaction for CANCEL");
            sendResponse(failure);
            return;
         }
      }

      PrdMap::iterator target = mPrdMap.find(id);
      if (target != mPrdMap.end())
      {
         SharedPtr<Prd> prd(target->second);
         SipMessagePrdCommand* cmd = new SipMessagePrdCommand(prd, request, *this);
         target->first->post(cmd);
         return;
      }
   }
   
   switch (request->header(h_RequestLine).method())
   {
      case PUBLISH:
         processPublish(*request);
         return;

      case MESSAGE:
      {
         std::pair<Data,Data> id = std::make_pair(Data::from(request.header(h_RequestLine).uri()),
                                                  request.header(h_From).uri().getAor());
         
         PageModePrdMap::iterator target = mPageModePrdMap.find(id);
         if (target != mPageModePrdMap.end())
         {
            SharedPtr<Prd> prd(target->second);
            SipMessagePrdCommand* cmd = new SipMessagePrdCommand(prd, request, *this);
            target->first->post(cmd);
         }
         else
         {
            SharedPtr<Prd> prd = mPageModePrdFactory->create(request)->manage();
            assert(prd->get());
            SipMessagePrdCommand* cmd = new SipMessagePrdCommand(prd, request, *this);
            prd->getPostable().post(cmd);
         }
         return;
      }
      
      case REGISTER:
         SharedPtr<Prd> prd = mRegistrationFactory->create(request)->manage();
         assert(prd->get());
         SipMessagePrdCommand* cmd = new SipMessagePrdCommand(prd, request, *this);
         prd->getPostable().post(cmd);
         return;

      case INVITE:
      case SUBSCRIBE:
         // !jf! do something here
         break;

      case CANCEL:
         assert(0);
         break;
         
      case ACK:
         // drop it on the floor since no matching Prd
         break;
         
         
      default:
      {
         SharedPtr<Prd> prd = mPrdServerTransactionFactory->create(request)->manage();
         assert(prd->get());
         SipMessagePrdCommand* cmd = new SipMessagePrdCommand(prd, request, *this);
         prd->getPostable().post(cmd);
         return;
      }

         
   }
   

   assert(mAppDialogSetFactory.get());
   // !jf! note, the logic was reversed during ye great merge of March of Ought 5
   if (toTag ||
       findDialogSet(DialogSetId(request)))
   {
      switch (request->header(h_RequestLine).getMethod())
      {
         case REGISTER:
         {
            SipMessage failure;
            makeResponse(failure, request, 400, "Registration requests can't have To: tags.");
            failure.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
            sendResponse(failure);
            break;
         }

         default:
         {
            DialogSet* ds = findDialogSet(DialogSetId(request));
            if (ds == 0)
            {
               if (request->header(h_RequestLine).method() != ACK)
               {
                  SipMessage failure;
                  makeResponse(failure, request, 481);
                  failure.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
                  InfoLog (<< "Rejected request (which was in a dialog) " << request->brief());
                  sendResponse(failure);
               }
               else
               {
                  InfoLog (<< "ACK doesn't match any dialog" << request->brief());
               }
            }
            else
            {
               InfoLog (<< "Handling in-dialog request: " << request->brief());
               ds->dispatch(request);
            }
         }
      }
   }
   else
   {
      switch (request->header(h_RequestLine).getMethod())
      {
         case ACK:
            DebugLog (<< "Discarding request: " << request->brief());
            break;

         case PRACK:
         case BYE:
         case UPDATE:
         case INFO: // !rm! in an ideal world
         {
            SipMessage failure;
            makeResponse(failure, request, 481);
            failure.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
            sendResponse(failure);
            break;
         }
         case CANCEL:
         {
            // find the appropropriate ServerInvSession
            CancelMap::iterator i = mCancelMap.find(request->getTransactionId());
            if (i != mCancelMap.end())
            {
               i->second->dispatch(request);
            }
            else
            {
               InfoLog (<< "Received a CANCEL on a non-existent transaction ");
               SipMessage failure;
               makeResponse(failure, request, 481);
               sendResponse(failure);
            }
            break;
         }
         case PUBLISH:
            assert(false);
         case SUBSCRIBE:
            if (!checkEventPackage(request))
            {
               InfoLog (<< "Rejecting request (unsupported package) " 
                        << request->brief());
               return;
            }
         case NOTIFY : // handle unsolicited (illegal) NOTIFYs
         case INVITE:   // new INVITE
         case REFER:    // out-of-dialog REFER
            //case INFO :    // handle non-dialog (illegal) INFOs
         case OPTIONS : // handle non-dialog OPTIONS
         case MESSAGE :
         case REGISTER:
         {
            {
               DialogSetId id(request);
               //cryptographically dangerous
               assert(mDialogSetMap.find(id) == mDialogSetMap.end());
            }
            if (mDumShutdownHandler)
            {
               SipMessage forbidden;
               makeResponse(forbidden, request, 480);
               forbidden.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
               sendResponse(forbidden);
               return;
            }
            try
            {
               DialogSet* dset =  new DialogSet(request, *this);

               DebugLog ( << "*********** Calling AppDialogSetFactory *************"  );
               AppDialogSet* appDs = mAppDialogSetFactory->createAppDialogSet(*this, request);
               appDs->mDialogSet = dset;
               dset->setUserProfile(appDs->selectUASUserProfile(request));
               dset->mAppDialogSet = appDs;

               DebugLog ( << "************* Adding DialogSet ***************" );
               DebugLog ( << "Before: " << Inserter(mDialogSetMap) );
               mDialogSetMap[dset->getId()] = dset;
               DebugLog ( << "After: Req" << Inserter(mDialogSetMap) );

               dset->dispatch(request);
            }
            catch (BaseException& e)
            {
               SipMessage failure;
               makeResponse(failure, request, 400, e.getMessage());
               failure.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
               sendResponse(failure);
            }

            break;
         }
         case RESPONSE:
         case SERVICE:
            assert(false);
            break;
         case UNKNOWN:
         case MAX_METHODS:
            assert(false);
            break;
      }
   }
}

void
DialogUsageManager::processResponse(std::auto_ptr<SipMessage> response)
{
   if (response->header(h_CSeq).method() != CANCEL)
   {
      DialogSet* ds = findDialogSet(DialogSetId(response));

      if (ds)
      {
         DebugLog ( << "DialogUsageManager::processResponse: " << response->brief());
         ds->dispatch(response);
      }
      else
      {
         InfoLog (<< "Throwing away stray response: " << response->brief());
      }
   }
}
   
bool
DialogUsageManager::validateRequestURI(const SipMessage& request)
{
   // RFC3261 - 8.2.1
   if (mAllowedMethods.count(request.header(h_RequestLine).getMethod()) == 0)
   {
      InfoLog (<< "Received an unsupported method: " << request.brief());

      SipMessage failure;
      makeResponse(failure, request, 405);
      failure.header(h_Allows) = getMasterProfile()->getAllowedMethods();
      sendResponse(failure);

      return false;
   }

   // RFC3261 - 8.2.2
   if (!getMasterProfile()->isSchemeSupported(request.header(h_RequestLine).uri().scheme()))
   {
      InfoLog (<< "Received an unsupported scheme: " << request.brief());
      SipMessage failure;
      makeResponse(failure, request, 416);
      sendResponse(failure);

	  return false;
   }

   return true;
}

bool
DialogUsageManager::validateRequiredOptions(const SipMessage& request)
{
   // RFC 2162 - 8.2.2
   if(request.exists(h_Requires) &&                 // Don't check requires if method is ACK or CANCEL
      (request.header(h_RequestLine).getMethod() != ACK ||
       request.header(h_RequestLine).getMethod() != CANCEL))
   {
      Tokens unsupported = getMasterProfile()->getUnsupportedOptionsTags(request.header(h_Requires));
	  if (!unsupported.empty())
	  {
	     InfoLog (<< "Received an unsupported option tag(s): " << request.brief());

         SipMessage failure;
         makeResponse(failure, request, 420);
         failure.header(h_Unsupporteds) = unsupported;
         sendResponse(failure);

         return false;
	  }
   }

   return true;
}

bool
DialogUsageManager::validateContent(const SipMessage& request)
{
   // RFC3261 - 8.2.3
   // Don't need to validate content headers if they are specified as optional in the content-disposition
   if (!(request.exists(h_ContentDisposition) &&
	     request.header(h_ContentDisposition).exists(p_handling) &&
	     isEqualNoCase(request.header(h_ContentDisposition).param(p_handling), Symbols::Optional)))
   {
	  if (request.exists(h_ContentType) && !getMasterProfile()->isMimeTypeSupported(request.header(h_RequestLine).method(), request.header(h_ContentType)))
      {
         InfoLog (<< "Received an unsupported mime type: " << request.header(h_ContentType) << " for " << request.brief());

         SipMessage failure;
         makeResponse(failure, request, 415);
         failure.header(h_Accepts) = getMasterProfile()->getSupportedMimeTypes(request.header(h_RequestLine).method());
         sendResponse(failure);

         return false;
      }

	  if (request.exists(h_ContentEncoding) && !getMasterProfile()->isContentEncodingSupported(request.header(h_ContentEncoding)))
      {
         InfoLog (<< "Received an unsupported mime type: " << request.header(h_ContentEncoding) << " for " << request.brief());
         SipMessage failure;
         makeResponse(failure, request, 415);
         failure.header(h_AcceptEncodings) = getMasterProfile()->getSupportedEncodings();
         sendResponse(failure);

         return false;
      }

      if (getMasterProfile()->validateContentLanguageEnabled() &&
          request.exists(h_ContentLanguages) && !getMasterProfile()->isLanguageSupported(request.header(h_ContentLanguages)))
      {
         InfoLog (<< "Received an unsupported language: " << request.header(h_ContentLanguages).front() << " for " << request.brief());

         SipMessage failure;
         makeResponse(failure, request, 415);
         failure.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
         sendResponse(failure);

         return false;
      }
   }

   return true;
}

bool
DialogUsageManager::validateAccept(const SipMessage& request)
{
   MethodTypes method = request.header(h_RequestLine).method();
   // checks for Accept to comply with SFTF test case 216
   if(request.exists(h_Accepts))
   {
      for (Mimes::const_iterator i = request.header(h_Accepts).begin();
           i != request.header(h_Accepts).end(); i++)
      {
         if (getMasterProfile()->isMimeTypeSupported(method, *i))
         {
            return true;  // Accept header passes validation if we support as least one of the mime types
         }
      }
   }
   // If no Accept header then application/sdp should be assumed for certain methods
   else if(method == INVITE ||
           method == OPTIONS ||
           method == PRACK ||
           method == UPDATE)
   {
      if (getMasterProfile()->isMimeTypeSupported(request.header(h_RequestLine).method(), Mime("application", "sdp")))
      {
         return true;
      }
   }
   else
   {
      // Other method without an Accept Header
      return true;
   }

   InfoLog (<< "Received unsupported mime types in accept header: " << request.brief());
   SipMessage failure;
   makeResponse(failure, request, 406);
   failure.header(h_Accepts) = getMasterProfile()->getSupportedMimeTypes(method);
   sendResponse(failure);
   return false;
}


bool
PrdManager::mergeRequest(const SipMessage& request)
{
   assert(request.isRequest());
   assert(request.isExternal());
   assert(!request.header(h_To).exists(p_tag));

   if (mMergedRequests.count(MergedRequestKey(request)))
   {
      SipMessage failure;
      makeResponse(failure, request, 482, "Merged Request");
      failure.header(h_AcceptLanguages) = getMasterProfile()->getSupportedLanguages();
      sendResponse(failure);
      return true;
   }

   return false;
}
