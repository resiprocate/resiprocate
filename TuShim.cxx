#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#ifndef WIN32

void 
TuShim::processRequest(SipMessage* msg)
{
   // validate Via, To, From, Call-ID, CSeq
   // looking for a dialog will validate the Call-ID
   // need to find the dialog to validate the CSeq


   // validate Via
   // if Via is garbage or not present, add to the discard list
   // TO DO!  
   // do we already do this in the transaction layer?

   // verify we have values for To/From uris
   // TO DO!

   // still need generic request processing (method, content, require, to, scheme checks)

   SipMessage response;
   DialogSet set;
   Dialog2 dialog;
   SIPSession session;
   Subscription sub;
   Registration reg;
   Data remoteTag;

   remoteTag = msg.hdr(h_From).param(p_tag);

   set = findDialogSet(msg.hdr(h_CallId).value());

   if (set != 0)
   {
      dialog = set.findDialog(remoteTag);
      if (dialog != 0)  // if we found the dialogSet
      {
         if (remoteTag == dialog.getTag())
         {
            // found the dialog (perfect match)
            // now check cseq
            if (! dialog.cseqIsValid(msg.hdr(h_CSeq).sequence(), dialog.getRemoteCSeq()))
            {
               // log that the stack is ignoring a message with a bad Cseq
               delete msg;
               return;
            }
            
            // pass the request to the correct state machine

            switch ( msg.method() ) {
               case INVITE :   // reINVITE
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
                  sub = findSubscription( msg.hdr(h_Event).value(),
                                          msg.hdr(h_Event).param(p_id) );
                  if (sub != 0)
                  {
                     response = sub.processRequest(msg);  
                  }
                  else
                  {
                     if (msg.method() == NOTIFY)
                     {
                        response = makeResponse(msg, 481);
                     }
                     else
                     {
                        response = sub.processRequest(msg);
                     }
                  }
                  break;
               default : // OPTIONS, INFO, MESSAGE, PUBLISH
                  response = dialog.processNonStateRequest();
            }
         }
         else // found dialog with remoteTag not set yet
         {
            if (msg.method() == NOTIFY)
            {
               // a NOTIFY is the only request that can
               // create a full dialog (when the 2xx from the
               // SUBSCRIBE arrives *after* the NOTIFY

               sub = findSubscription( msg.hdr(h_Event).value(), 
                                       msg.hdr(h_Event).param(p_id) );
               if (sub != 0)
               {
                  dialog.setTag( remoteTag );
                  dialog.setRemoteCSeq( msg.hdr(h_CSeq).sequence() );
                  response = sub.processRequest(msg);
               }
               else
               {            
                  makeResponse(msg,481);
               }
            }
            else
            {
               makeResponse(msg,481);
            }
         }
      }
      else  // dialog not found, create a new dialog in this dialogSet?
      {
         if (msg.method() == NOTIFY)
         {
            sub createSubscription(msg);
            response = sub.processReqest(msg);
         }
         else
         {
            response = makeResponse(msg, 481);
         }
      } 
   }
   else  // dialogSet not found, create a new dialog in a new dialogSet?
   {
      switch ( msg.method() ) {
         case MESSAGE :
         case OPTIONS :
         case PUBLISH :
            processNonStateRequest();
            break;
         case INVITE :
            session createSession(msg);
            response = session.processSession(msg);
            break;
         case REFER :
         case SUBSCRIBE :
            sub createSubscription(msg);
            response = sub.processSubscription(msg);
            break;
         case REGISTER :
            reg createRegistration(msg);
            response = reg.processRegistration(msg);
            break;
         default :
            response = makeResponse(msg, 481);
      }
   }
   response.send();
   delete msg;
}

void 
TuShim::processResponse(SipMessage* msg)
{
   // verify Via here
   // check for valid To/From uris

   remoteTag = msg.hdr(h_To).param(p_tag);

   set = findDialogSet(msg.hdr(h_CallId).value());

   if (set != 0)  // if we found the dialogSet
   {
      dialog = set.findDialog(remoteTag);
      if (dialog !=0)  
      {
         if (remoteTag == dialog.getTag())
         {
            // we found the dialog exactly
            // find out the appropriate state machine and send to it 
            switch ( msg.hdr(h_CSeq).method()) {
               case INVITE :
               case CANCEL :
               case BYE :
               case PRACK :
               case UPDATE :
               case INFO :
                  dialog.getSession().processResponse(msg);
                  break;
               case REFER :
               case SUBSCRIBE : 
               case NOTIFY :
                  sub = dialog.findSubscription( msg.hdr(h_Event).value(),
                                          msg.hdr(h_Event).param(p_id) );
                  if (sub != 0)
                  {
                     sub.processResponse(msg);
                  }
                  // else ignore
                  break;
               case REGISTER : 
                  dialog.getRegistration().processResponse(msg);
                  break;
               case MESSAGE :
               case PUBLISH :
               case OPTIONS :
                  dialog.processNonDialogResponse(msg);
                  break;
               case ACK :
                  // should never get a response to an ACK
               default :
                  // ignore
            }
         }
         else  // found the dialog but the remoteTag isn't set yet
         {
            switch ( msg.hdr(h_Cseq).method()) {
               case INVITE :
                  dialog.setTag(remoteTag);
                  dialog.setRemoteCSeq( msg.hdr(h_CSeq).sequence() );
                  dialog.getSession().processResponse(msg);
                  break;
               case REFER :
               case SUBSCRIBE :
                  // find apropos subscription, send there
                  sub = findSubscription( msg.hdr(h_Event).value(),
                                          msg.hdr(h_Event).param(p_id) );
                  if (sub != 0)
                  {
                     dialog.setTag( remoteTag );
                     dialog.setRemoteCSeq( msg.hdr(h_CSeq).sequence() );
                     sub.processResponse(msg);
                  }
                  // else ignore
                  break;
               case REGISTER :
                  dialog.setTag(remoteTag);
                  dialog.setRemoteCSeq( msg.hdr(h_CSeq).sequence() );
                  dialog.getRegistration().processResponse(msg);
                  break;
               default :
                  // ignore bogus/stray response
            }
         }
      }
      else // found dialogSet but not the specific dialog
      {
         if (msg.hdr(h_CSeq).method() == INVITE)
         {
            // must have forked and received multiple dialog-creating responses
            // verify that the state of other state machines is consistent with this
            // if everything is good, create a new dialog and session state machine

            session = set.firstDialog().getSession();
            if (session != 0)
            {
               switch (session.getState() ) {
                  case EarlyState :
                     createDialog(msg);
                     break;
                  case ConfirmedState :
                     if (msg.responseCode >= 200 && msg.responseCode < 299)
                     {
                        createDialog(msg);
                     }
                     // else ignore
                     break;
                  default :
                     // ignore
               }
            }
            else // no session for this dialog!
            {
               // ignore
            }
         }
         else  // no dialog for this non-INVITE response
         {
            // ignore
         }
      }
      else // didn't find dialogSet
      {
         // very bad, ignore
      }
   }
   else // didn't find dialogSet, completely stray response
   {
      // ignore
   }
   delete msg;
}

Dialog2 TuShim::createDialog( const Data callId, const Data localTag, 
                              const Data remoteTag, 
                              bool wilcardRemoteTag = false)
{
   DialogSet set;

   set = findDialogSet( callId );

   return createDialog( set, callId, localTag, remoteTag, wildcardRemoteTag );
}

Dialog2 TuShim::createDialog( const DialogSet set, const Data callId,
                      const Data localTag, constData remoteTag, 
                      bool wildcardRemoteTag = false );
{
   Dialog2 dialog;
   dialog = 0;

   if (set == 0)
   {
      set = createDialogSet( callId, localTag );
   }

   if (set != 0)
   {
      dialog = set.createDialog (callId, localTag, remoteTag, 
                                   wildcardRemoteTag );
   }
   return dialog;
}


#endif
