#include "resiprocate/SipStack.hxx"
#include "DialogUsageManager.hxx"

using namespace resip;

DialogUsageManager::DialogUsageManager(SipStack& stack) 
   : mStack(stack)
{
   
}

void DialogUsageManager::setProfile(Profile* profile)
{
   mProfile = profile;
}


void DialogUsageManager::setManager(RedirectManager* manager)
{
   mRedirectManager = manager;
}

void 
DialogUsageManager::setManager(ClientAuthManager* manager)
{
   mClientAuthManager = manager;
}

void 
DialogUsageManager::setManager(ServerAuthManager* manager)
{
   mServerAuthManager = manager;
}

void 
DialogUsageManager::setHandler(ClientRegistrationHandler* handler)
{
   mClientRegistrationHandler = handler;
}

void 
DialogUsageManager::setHandler(ServerRegistrationHandler* handler)
{
   mServerRegistrationHandler = handler;
}

void 
DialogUsageManager::setHandler(InvSessionHandler* handler)
{
   mInviteSessionHandler = handler;
}


void 
DialogUsageManager::addHandler(const Data& eventType, ClientSubscriptionHandler*)
{
}

void 
DialogUsageManager::addHandler(const Data& eventType, ServerSubscriptionHandler*)
{
}

void 
DialogUsageManager::addHandler(const Data& eventType, ClientPublicationHandler*)
{
}

void 
DialogUsageManager::addHandler(const Data& eventType, ServerPublicationHandler*)
{
}

void 
DialogUsageManager::addHandler(MethodTypes&, OutOfDialogHandler*)
{
}

void
DialogUsageManager::process()
{
   while (1)
   {
      int seltime = 10;
      
      FdSet fdset; 
      mStack.buildFdSet(fdset);
      fdset.selectMilliSeconds(seltime); 
      mStack.process(fdset);
      
      SipMessage* msg = mStack.receive();
      if (msg)
      {
         if (msg->isRequest())
         {
            if (validateRequest(*msg) && 
                validateTo(*msg) && 
                !mergeRequest(*msg) &&
                mServerAuthManager && mServerAuthManager->handle(*msg))
            {
               processRequest(*msg);
               processResponse(*msg);
            
               

               DialogSet& dialogs = findDialogSet(DialogSetId(*msg));
               if (dialogs.empty())
               {
               }
               

               switch (msg->header(h_RequestLine).getMethod())
               {
                  case INVITE :   // reINVITE
                  case INFO:
                  case UPDATE :
                  case PRACK :
                  case ACK :
                  case BYE :
                  case CANCEL : 
                     dialog.getSession().processSession(msg);
                     break;
                  case REGISTER :
                     dialog.getRegistration().processRegistration(msg);
                     break;
                  case REFER :
                  case SUBSCRIBE :
                  case NOTIFY :
                     break;

                  case UNKNOWN:
                  {
                     InfoLog (<< "Received an unknown method: " << msg->brief());
                     std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 405));
                     mStack.send(*failure);
                     break;
                  }
               }
            }
         }
         else if (msg->isResponse())
         {
            switch (msg->header(h_StatusLine).statusCode())
            {
               // !jf! handle retranmission of INV/200
            }
         }
         
      }

      delete msg;
   }
}

bool
DialogUsageManager::validateRequest(const SipMessage& request)
{
   if (!mProfile.isSchemeSupported(request.header(h_RequestLine).uri().scheme()))
   {
      InfoLog (<< "Received an unsupported scheme: " << request.brief());
      std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 416));
      mStack.send(*failure);
      return false;
   }
   else if (!mProfile.isMethodSupported(request.header(h_RequestLine).getMethod()))
   {
      InfoLog (<< "Received an unsupported method: " << request.brief());
      std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 405));
      failure->header(h_Allows) = mProfile.getAllowedMethods();
      mStack.send(*failure);
      return false;
   }
   else
   {
      Tokens unsupported = mProfile.isSupported(msg.header(h_Requires));
      if (!unsupported.empty())
      {
         InfoLog (<< "Received an unsupported option tag(s): " << request.brief());
         std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 420));
         failure->header(h_Unsupporteds) = unsupported;
         mStack.send(*failure);
         return false;
      }
      else if (msg.exists(h_ContentDisposition))
      {
         if (msg.header(h_ContentDisposition).exists(p_handling) && 
             isEqualNoCase(msg.header(h_ContentDisposition).param(p_handling), Symbols::Required))
         {
            if (!mProfile.isMimeTypeSupported(msg.header(h_ContentType)))
            {
               InfoLog (<< "Received an unsupported mime type: " << request.brief());
               std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 415));
               failure->header(h_Accepts) = mProfile.getSupportedMimeTypes();
               mStack.send(*failure);

               return false;
            }
            else if (!mProfile.isContentEncodingSupported(msg.header(h_ContentEncoding)))
            {
               InfoLog (<< "Received an unsupported mime type: " << request.brief());
               std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 415));
               failure->header(h_AcceptEncoding) = mProfile.getSupportedEncodings();
               mStack.send(*failure);
            }
            else if (!mProfile.isLanguageSupported(msg.header(h_ContentLanguages)))
            {
               InfoLog (<< "Received an unsupported language: " << request.brief());
               std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 415));
               failure->header(h_AcceptLanguages) = mProfile.getSupportedLanguages();
               mStack.send(*failure);
            }
         }
      }
   }
         
   return true;

}

bool
DialogUsageManager::validateTo(const SipMessage& request)
{
   // !jf! check that the request is targeted at me!
   // will require support in profile
   return true;
}

bool
DialogUsageManager::mergeRequest(const SipMessage& request)
{
   // merge requrests here
   if ( !msg->header(h_To).param(p_tag).exists() )
   {
      DialogSet& dialogs = findDialogSet(DialogSetId(*msg));
      for (DialogSet::const_iterator i=dialogs.begin(); i!=dialogs.end(); i++)
      {
         if (i->shouldMerge(request))
         {
            InfoLog (<< "Merging request for: " << request.brief());
            
            std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 482));
            mStack.send(*failure);
            
            return true;
         }
      }
   }
   return false;
}
void
DialogUsageManager::processRequest(const SipMessage& request)
{
   if ( !request.header(h_To).param(p_tag).exists() )
   {
      switch (request.header(h_RequestLine).getMethod())
      {
         case ACK:
            DebugLog (<< "Discarding request: " << request.brief());
            break;
                        
         case PRACK:
         case BYE:
         case UPDATE:
            //case INFO: // !rm! in an ideal world
            //case NOTIFY: // !rm! in an ideal world
         {
            std::auto_ptr<SipMessage> failure(Helper::makeResponse(request, 481));
            mStack.send(*failure);
            break;
         }

         case INVITE:  // new INVITE
         {
            ServerInvSession* session = new ServerInvSession(*this, request);
            break;
         }
         case CANCEL:
         {
            // find the appropropriate ServerInvSession
            DialogSet& dialogs = findDialogSet(DialogSetId(request));
            dialogs.cancel(request);
            break;
         }
         case SUBSCRIBE:
         case REFER: // out-of-dialog REFER
         {
            ServerSubscription* sub = new ServerSubscription(*this, request);
            break;
         }
         case REGISTER:
         {
            ServerRegistration* reg = new ServerRegistration(*this, request);
            break;
         }
         case PUBLISH:
         {
            ServerPublication* pub = new ServerPublication(*this, request);
            break;                       
         }
         case MESSAGE :
         case OPTIONS :
         case INFO :   // handle non-dialog (illegal) INFOs
         case NOTIFY : // handle unsolicited (illegal) NOTIFYs
         {
            // make a non-dialog BaseUsage
            ServerOutOfDialogReq* req = new ServerOutOfDialogReq(*this, request);
            break;
         }
         default:
            assert(0);
            break;
      }
   }
   else // in a specific dialog
   {
      switch (request.header(h_RequestLine).getMethod())
      {
         case REGISTER:
         {
            std::auto_ptr<SipMessage> failure(Helper::makeResponse(request, 400, "rjs says, Go to hell"));
            mStack.send(*failure);
            break;
         }
         
         default:
         {
            Dialog& dialog = findDialog(DialogId(request));
            dialog.process(DialogId(request));
         }
      }
   }
}

Dialog&  
DialogUsageManager::findDialog(DialogId id)
{

    HashMap<DialogSetId, DialogSet>::const_iterator it;

    DialogSetId setId = id.getDialogSetId();
    if ((it = mDialogSetMap.find(setId)) == mDialogSetMap.end())
    {
        /**  @todo: return empty object (?) */
    }
    DialogSet dialogSet = it->second();
    Dialog dialog;
    if ((dialog = dialogSet.find(id)) == NULL)
    {
        /**  @todo: return empty object (?) */
    }
    return *dialog;
    
}
 
DialogSet&
DialogUsageManager::findDialogSet( DialogSetId id )
{
    HashMap<DialogSetId, DialogSet>::const_iterator it;

    if ((it = mDialogSetMap.find(id)) == mDialogSetMap.end())
    {
        /**  @todo: return empty object (?) **/
    }
    return it->first();

}

BaseCreator&
DialogUsageManager::findCreator(DialogId id)
{
    DialogSetId setId = id.getDialogSetId();
    DialogSet dialogSet = findDialogSet(setId);
    BaseCreator creator = dialogset.getCreator();
    if (creator == NULL)
    {
        /** @todo; return empty object (?) */
    }
    return (*creator);
    
}

