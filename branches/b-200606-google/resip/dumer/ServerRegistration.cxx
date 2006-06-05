#include "ServerRegistration.hxx"

void
ServerRegistration::accept(ContactRecordList& contacts,
                           int statusCode)
{
   accept(contacts, Helper::makeResponse(*mRequest,statusCode));
}

void
ServerRegistration::accept(ContactRecordList& contacts,
                           std::auto_ptr<SipMessage>* ok)
{
   assert (ok.get());
   ok->remove(h_Contacts);
   InfoLog( << "Accepting registration for " << mAor);

   time_t now;
   time(&now);


   ContactRecordList::iterator i;
   NameAddr contact;

   for (i = contacts.begin(); i != contacts.end(); i++)
   {
      if (i->expires - now <= 0)
      {
         continue;
      }
      contact.uri() = i->uri;
      contact.param(p_expires) = int(i->expires - now);
      ok->header(h_Contacts).push_back(contact);
      if(i->useQ)
      {
         ok->header(h_Contacts).back().param(p_q)=i->q;
      }
   }

  mPrdManager->send(ok);   

  unmanage();
}

void
ServerRegistration::reject(int statusCode)
{
   InfoLog( << "rejected a registration " << mAor 
            << " with statusCode=" << statusCode );

   mPrdManager->send(Helper::makeResponse(*mRequest,statusCode));

   unmanage();
}

void
ServerRegistration::dispatch(std::auto_ptr<SipMessage> msg)
{
   assert(msg->isRequest());
   mRequest = msg;

   int globalExpires = 0;

   if (msg->exists(h_Expires))
   {
      globalExpires = msg->header(h_Expires).value();
   }
   else
   {
      globalExpires = 3600;
   }

   // If no conacts are present in the request, this is simply a query.
   if (!msg->exists(h_Contacts))
   {
     onQuery();
     return;
   }

   
   ParserContainer<NameAddr> contactList(msg->header(h_Contacts));
   ParserContainer<NameAddr>::iterator i;
   int expires;
   time_t now;
   time(&now);
   const Data &callId = msg->header(h_callId).value();
   int cseq = msg->header(h_cseq).sequence();

   for(i = contactList.begin(); i != contactList.end(); i++)
   {
     if (i->exists(p_expires))
     {
       expires = i->param(p_expires);
     }
     else
     {
       expires = globalExpires;
     }

     // Check for "Contact: *" style deregistration
     if (i->isAllContacts())
     {
       if (contactList.size() > 1 || expires != 0)
       {
          SharedPtr<SipMessage> failure(new SipMessage);
          mDum.makeResponse(*failure, msg, 400, "Invalid use of 'Contact: *'");
          mDum.send(failure);
          delete(this);
          return;
       }

       onRemoveAll(callId, cseq);
       return;
     }

     static ExtensionParameter p_cid("cid");
     i->uri().param(p_cid) = Data(msg->getSource().connectionId);
      
     // Check to see if this is a removal.
     if (expires == 0)
     {
       if (operation == REFRESH)
       {
         operation = REMOVE;
       }
     }
     // Otherwise, it's an addition or refresh.
     else
     {
       RegistrationPersistenceManager::update_status_t status;
       InfoLog (<< "Adding " << mAor << " -> " << i->uri());
       if (status == RegistrationPersistenceManager::CONTACT_CREATED)
       {
         operation = ADD;
       }
     }
   }


 //////////////////////////////////////////////////////////////////////  
   onRemoveAll(callId, cseq);
   onUpdate(callId, cseq, writes, removes);
}

void
ServerRegistration::dispatch(std::auto_ptr<DumTimeout> request)
{
}
