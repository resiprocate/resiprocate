/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Timer.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/MsgHeaderScanner.hxx"
#include "resip/stack/Helper.hxx"
#  include <sys/time.h>
#  include <unistd.h>
#include <sys/types.h>


int main()
{
   int runs=10000;

   char* inviteBuffers[runs];
   char* ringBuffers[runs];
   char* answerBuffers[runs];
   char* ackBuffers[runs];

   unsigned int allocBegin=clock();
   for(int i=0;i<runs;i++)
   {
      inviteBuffers[i]= resip::MsgHeaderScanner::allocateBuffer(4092);
      ringBuffers[i] = resip::MsgHeaderScanner::allocateBuffer(4092);
      answerBuffers[i] = resip::MsgHeaderScanner::allocateBuffer(4092);
      ackBuffers[i] = resip::MsgHeaderScanner::allocateBuffer(4092);
   }
   unsigned int allocEnd=clock();
   
   unsigned int allocTime=allocEnd-allocBegin;
   

   const char* inviteTemp ="INVITE sip:derek@127.0.0.7:6066;transport=UDP SIP/2.0\r\n"
         "Via: SIP/2.0/UDP 127.0.0.1:5060;branch=z9hG4bK-d87543-41cebf136c4ed406-1--d87543-;rport\r\n"
         "Via: SIP/2.0/udp 127.0.0.3:6062;rport=6062;branch=z9hG4bK-d87543-3758a57a5f9002b9-1--d87543-\r\n"
         "Max-Forwards: 20\r\n"
         "Record-Route: <sip:repro@127.0.0.1:5060;transport=UDP;lr>\r\n"
         "Contact: <sip:jason@127.0.0.3:6062;transport=UDP>\r\n"
         "To: <sip:derek@localhost>\r\n"
         "From: <sip:jason@localhost>;tag=4e34399c\r\n"
         "Call-ID: 44b3f52a3879a644NzJmZGJmZTFhOWUwYjBkZGU4YzRjNDkwZDQ2NTU0NjQ.\r\n"
         "CSeq: 2 INVITE\r\n"
         "Proxy-Authorization: Digest username=\"jason\",realm=\"localhost\",nonce=\"1147103058:ec6915a9f945fc708379525c655841ec\",uri=\"sip:derek@localhost\",response=\"99c9fa72b40954ed84affa65854cd1c5\",cnonce=\"foo\",nc=00000015,qop=auth-int,algorithm=MD5\r\n"
         "Content-Length: 0\r\n"
         "\r\n\r\n";
         
   const char* ringTemp = "SIP/2.0 180 Ringing\r\n"
         "Via: SIP/2.0/UDP 127.0.0.1:5060;branch=z9hG4bK-d87543-41cebf136c4ed406-1--d87543-;rport=5060\r\n"
         "Via: SIP/2.0/udp 127.0.0.3:6062;rport=6062;branch=z9hG4bK-d87543-3758a57a5f9002b9-1--d87543-\r\n"
         "Record-Route: <sip:repro@127.0.0.1:5060;transport=UDP;lr>\r\n"
         "Contact: <sip:derek@127.0.0.7:6066;transport=UDP>\r\n"
         "To: <sip:derek@localhost>;tag=55b941ca\r\n"
         "From: <sip:jason@localhost>;tag=4e34399c\r\n"
         "Call-ID: 44b3f52a3879a644NzJmZGJmZTFhOWUwYjBkZGU4YzRjNDkwZDQ2NTU0NjQ.\r\n"
         "CSeq: 2 INVITE\r\n"
         "Content-Length: 0\r\n"
         "\r\n\r\n";

   const char* answerTemp= "SIP/2.0 200 OK\r\n"
         "Via: SIP/2.0/UDP 127.0.0.1:5060;branch=z9hG4bK-d87543-41cebf136c4ed406-1--d87543-;rport=5060\r\n"
         "Via: SIP/2.0/udp 127.0.0.3:6062;rport=6062;branch=z9hG4bK-d87543-3758a57a5f9002b9-1--d87543-\r\n"
         "Record-Route: <sip:repro@127.0.0.1:5060;transport=UDP;lr>\r\n"
         "Contact: <sip:derek@127.0.0.7:6066;transport=UDP>\r\n"
         "To: <sip:derek@localhost>;tag=55b941ca\r\n"
         "From: <sip:jason@localhost>;tag=4e34399c\r\n"
         "Call-ID: 44b3f52a3879a644NzJmZGJmZTFhOWUwYjBkZGU4YzRjNDkwZDQ2NTU0NjQ.\r\n"
         "CSeq: 2 INVITE\r\n"
         "Content-Length: 0\r\n"
         "\r\n\r\n";

   const char* ackTemp="ACK sip:derek@127.0.0.7:6066;transport=UDP SIP/2.0\r\n"
         "Via: SIP/2.0/udp 127.0.0.3:6062;branch=z9hG4bK-d87543-2e8330e8494dccbd-1--d87543-;rport\r\n"
         "Max-Forwards: 70\r\n"
         "Route: <sip:repro@127.0.0.1:5060;transport=UDP;lr>\r\n"
         "Contact: <sip:jason@127.0.0.3:6062;transport=UDP>\r\n"
         "To: <sip:derek@localhost>;tag=55b941ca\r\n"
         "From: <sip:jason@localhost>;tag=4e34399c\r\n"
         "Call-ID: 44b3f52a3879a644NzJmZGJmZTFhOWUwYjBkZGU4YzRjNDkwZDQ2NTU0NjQ.\r\n"
         "CSeq: 2 ACK\r\n"
         "Proxy-Authorization: Digest username=\"jason\",realm=\"localhost\",nonce=\"1147103058:ec6915a9f945fc708379525c655841ec\",uri=\"sip:derek@localhost\",response=\"99c9fa72b40954ed84affa65854cd1c5\",cnonce=\"foo\",nc=00000015,qop=auth-int,algorithm=MD5\r\n"
         "Content-Length: 0\r\n"
         "\r\n\r\n";
         
   int inviteLen=strlen(inviteTemp);
   int ringLen=strlen(ringTemp);
   int answerLen=strlen(answerTemp);
   int ackLen=strlen(ackTemp);

   resip::Data inviteData(inviteTemp);
   resip::Data ringData(ringTemp);
   resip::Data answerData(answerTemp);
   resip::Data ackData(ackTemp);

   unsigned int copyBegin=clock();
   for(int i=0;i<runs;i++)
   {
      memcpy(inviteBuffers[i],inviteTemp,inviteLen);
      memcpy(ringBuffers[i],ringTemp,ringLen);
      memcpy(answerBuffers[i],answerTemp,answerLen);
      memcpy(ackBuffers[i],ackTemp,ackLen);
   }
   unsigned int copyEnd=clock();
   
   unsigned copyTime=copyEnd-copyBegin;
   
   resip::SipMessage* invites[runs];
   resip::SipMessage* rings[runs];
   resip::SipMessage* answers[runs];
   resip::SipMessage* acks[runs];
   
   unsigned int constructSipMessagesBegin=clock();
   for(int i=0;i<runs;i++)
   {
      invites[i]=new resip::SipMessage;
      rings[i]=new resip::SipMessage;
      answers[i]=new resip::SipMessage;
      acks[i]=new resip::SipMessage;
   }
   unsigned int constructSipMessagesEnd=clock();

   unsigned int constructSipMessagesTime=constructSipMessagesEnd-constructSipMessagesBegin;
   
   
   unsigned int addBuffersBegin=clock();
   for(int i=0; i<runs;i++)
   {
      invites[i]->addBuffer(inviteBuffers[i]);
      rings[i]->addBuffer(ringBuffers[i]);
      answers[i]->addBuffer(answerBuffers[i]);
      acks[i]->addBuffer(ackBuffers[i]);
   }
   unsigned int addBuffersEnd=clock();

   unsigned int addBuffersTime=addBuffersEnd-addBuffersBegin;
   
   std::cout << "Preparsing buffers." << std::endl;

   resip::MsgHeaderScanner mMsgHeaderScanner;

   char *unprocessedCharPtr;

   unsigned int preparseBegin=clock();
   for(int i=0;i<runs;i++)
   {
      mMsgHeaderScanner.prepareForMessage(invites[i]);
      
      assert(mMsgHeaderScanner.scanChunk(inviteBuffers[i],
                                      inviteLen,
                                      &unprocessedCharPtr) ==
            resip::MsgHeaderScanner::scrEnd);
            
      mMsgHeaderScanner.prepareForMessage(rings[i]);

      assert(mMsgHeaderScanner.scanChunk(ringBuffers[i],
                                      ringLen,
                                      &unprocessedCharPtr) ==
            resip::MsgHeaderScanner::scrEnd);
      
      
      mMsgHeaderScanner.prepareForMessage(answers[i]);

      assert(mMsgHeaderScanner.scanChunk(answerBuffers[i],
                                      answerLen,
                                      &unprocessedCharPtr) ==
            resip::MsgHeaderScanner::scrEnd);
            
      mMsgHeaderScanner.prepareForMessage(acks[i]);

      assert(mMsgHeaderScanner.scanChunk(ackBuffers[i],
                                      ackLen,
                                      &unprocessedCharPtr) ==
            resip::MsgHeaderScanner::scrEnd);
            
   }
   unsigned int preparseEnd=clock();
   
   unsigned int preparseTime=preparseEnd-preparseBegin;



   unsigned int validateBegin=clock();
   for(int i=0;i<runs;i++)
   {
      resip::Helper::validateMessage(*invites[i]);
      resip::Helper::validateMessage(*rings[i]);
      resip::Helper::validateMessage(*answers[i]);
      resip::Helper::validateMessage(*acks[i]);
   }
   unsigned int validateEnd=clock();
   
   unsigned int validateTime=validateEnd-validateBegin;

   unsigned int validate2Begin=clock();
   for(int i=0;i<runs;i++)
   {
      resip::Helper::validateMessage(*invites[i]);
      resip::Helper::validateMessage(*rings[i]);
      resip::Helper::validateMessage(*answers[i]);
      resip::Helper::validateMessage(*acks[i]);
   }
   unsigned int validate2End=clock();
   
   unsigned int validate2Time=validate2End-validate2Begin;

   
   resip::Data destBuffer("blah",10000);
   destBuffer.clear();

   std::cout << std::endl <<"Calibrating for Creation/Destruction of oDataStreams." << std::endl;

   //!bwc! for calibration.
   unsigned int oDataStreamCreateDestroyBegin=clock();
   
   for(int i=0;i<runs;i++)
   {
      {
         resip::oDataStream destStr(destBuffer);
      }
      destBuffer.clear();
      if(i%1000==0){std::cout << ".";}
   }
   
   unsigned int oDataStreamCreateDestroyEnd=clock();

   unsigned int testingOverhead=oDataStreamCreateDestroyEnd-oDataStreamCreateDestroyBegin;


   std::cout << std::endl << "Using encode()" << std::endl;

   //!bwc! Time using encode()
   unsigned int encodeBegin=clock();
   
   for(int i=0;i<runs;i++)
   {
      {
         resip::oDataStream destStr(destBuffer);
         invites[i]->encode(destStr);
         rings[i]->encode(destStr);
         answers[i]->encode(destStr);
         acks[i]->encode(destStr);
      }
      destBuffer.clear();
      if(i%1000==0){std::cout << ".";}
   }
   
   unsigned int encodeEnd=clock();
   
   unsigned int encodeTime=encodeEnd-encodeBegin;
  
   std::cout << std::endl <<"Using raw oDataStream" << std::endl; 
   
   unsigned int rawDataStreamBegin=clock();
   for(int i=0;i<runs;i++)
   {
      {
         resip::oDataStream destStr(destBuffer);
         destStr << inviteData;
         destStr << ringData;
         destStr << answerData;
         destStr << ackData;
      }
      destBuffer.clear();
      if(i%1000==0){std::cout << ".";}
   }
   
   unsigned int rawDataStreamEnd=clock();
   
   unsigned int rawDataStreamTime=rawDataStreamEnd-rawDataStreamBegin;
   
   std::cout << std::endl <<"Using memcopy" << std::endl;
   
   unsigned int memCopyBegin=clock();
   for(int i=0;i<runs;i++)
   {
      destBuffer += inviteData;
      destBuffer += ringData;
      destBuffer += answerData;
      destBuffer += ackData;
      destBuffer.clear();
      if(i%1000==0){std::cout << ".";}
   }
   
   std::cout << std::endl;
   
   unsigned int memCopyEnd=clock();
   
   unsigned int memCopyTime=memCopyEnd-memCopyBegin;
   
   std::cout << std::endl << "Copying SipMessages" << std::endl;

   resip::SipMessage* newInvites[runs];
   resip::SipMessage* newRings[runs];
   resip::SipMessage* newAnswers[runs];
   resip::SipMessage* newAcks[runs];
   
   
   unsigned int msgCopyBegin=clock();
   
   for(int i=0;i<runs;i++)
   {
      newInvites[i]=dynamic_cast<resip::SipMessage*>(invites[i]->clone());
      newRings[i]=dynamic_cast<resip::SipMessage*>(rings[i]->clone());
      newAnswers[i]=dynamic_cast<resip::SipMessage*>(answers[i]->clone());
      newAcks[i]=dynamic_cast<resip::SipMessage*>(acks[i]->clone());
      if(i%1000==0){std::cout << ".";}
   }
   unsigned int msgCopyEnd=clock();
   
   unsigned int msgCopyTime=msgCopyEnd-msgCopyBegin;

   unsigned int msgCopyDeleteBegin=clock();
   for(int i=0;i<runs;i++)
   {
      delete newInvites[i];
      delete newRings[i];
      delete newAnswers[i];
      delete newAcks[i];
   }
   unsigned int msgCopyDeleteEnd=clock();
   
   unsigned int msgCopyDeleteTime=msgCopyDeleteEnd-msgCopyDeleteBegin;

#if 0
   resip::RequestLine* newReqLines[runs];
   
   unsigned int requestLineCloneBegin=clock();
   for(int i=0;i<runs;i++)
   {
      newReqLines[i]=dynamic_cast<resip::RequestLine*>(invites[i]->header(resip::h_RequestLine).clone());
      newReqLines[i]=dynamic_cast<resip::RequestLine*>(acks[i]->header(resip::h_RequestLine).clone());
   }
   unsigned int requestLineCloneEnd=clock();
   
   unsigned int requestLineCloneTime=requestLineCloneEnd-requestLineCloneBegin;
   
   
   resip::HeaderFieldValueList* newViaInvites[runs];
   resip::HeaderFieldValueList* newViaRings[runs];
   resip::HeaderFieldValueList* newViaAnswers[runs];
   resip::HeaderFieldValueList* newViaAcks[runs];

   unsigned int ViaCloneBegin=clock();
   for(int i=0;i<runs;i++)
   {
      newViaInvites[i]= new resip::HeaderFieldValueList(*invites[i]->mHeaders[resip::Headers::Via]);
      newViaRings[i]= new resip::HeaderFieldValueList(*rings[i]->mHeaders[resip::Headers::Via]);
      newViaAnswers[i]= new resip::HeaderFieldValueList(*answers[i]->mHeaders[resip::Headers::Via]);
      newViaAcks[i]= new resip::HeaderFieldValueList(*acks[i]->mHeaders[resip::Headers::Via]);
   }
   unsigned int ViaCloneEnd=clock();
   
   unsigned int ViaCloneTime=ViaCloneEnd-ViaCloneBegin;
   
   resip::HeaderFieldValueList* newToInvites[runs];
   resip::HeaderFieldValueList* newToRings[runs];
   resip::HeaderFieldValueList* newToAnswers[runs];
   resip::HeaderFieldValueList* newToAcks[runs];

   unsigned int ToCloneBegin=clock();
   for(int i=0;i<runs;i++)
   {
      newToInvites[i]= new resip::HeaderFieldValueList(*invites[i]->mHeaders[resip::Headers::To]);
      newToRings[i]= new resip::HeaderFieldValueList(*rings[i]->mHeaders[resip::Headers::To]);
      newToAnswers[i]= new resip::HeaderFieldValueList(*answers[i]->mHeaders[resip::Headers::To]);
      newToAcks[i]= new resip::HeaderFieldValueList(*acks[i]->mHeaders[resip::Headers::To]);
   }
   unsigned int ToCloneEnd=clock();
   
   unsigned int ToCloneTime=ToCloneEnd-ToCloneBegin;
   
   resip::HeaderFieldValueList* newFromInvites[runs];
   resip::HeaderFieldValueList* newFromRings[runs];
   resip::HeaderFieldValueList* newFromAnswers[runs];
   resip::HeaderFieldValueList* newFromAcks[runs];

   unsigned int FromCloneBegin=clock();
   for(int i=0;i<runs;i++)
   {
      newFromInvites[i]= new resip::HeaderFieldValueList(*invites[i]->mHeaders[resip::Headers::From]);
      newFromRings[i]= new resip::HeaderFieldValueList(*rings[i]->mHeaders[resip::Headers::From]);
      newFromAnswers[i]= new resip::HeaderFieldValueList(*answers[i]->mHeaders[resip::Headers::From]);
      newFromAcks[i]= new resip::HeaderFieldValueList(*acks[i]->mHeaders[resip::Headers::From]);
   }
   unsigned int FromCloneEnd=clock();
   
   unsigned int FromCloneTime=FromCloneEnd-FromCloneBegin;
   
   resip::HeaderFieldValueList* newCSeqInvites[runs];
   resip::HeaderFieldValueList* newCSeqRings[runs];
   resip::HeaderFieldValueList* newCSeqAnswers[runs];
   resip::HeaderFieldValueList* newCSeqAcks[runs];

   unsigned int CSeqCloneBegin=clock();
   for(int i=0;i<runs;i++)
   {
      newCSeqInvites[i]= new resip::HeaderFieldValueList(*invites[i]->mHeaders[resip::Headers::CSeq]);
      newCSeqRings[i]= new resip::HeaderFieldValueList(*rings[i]->mHeaders[resip::Headers::CSeq]);
      newCSeqAnswers[i]= new resip::HeaderFieldValueList(*answers[i]->mHeaders[resip::Headers::CSeq]);
      newCSeqAcks[i]= new resip::HeaderFieldValueList(*acks[i]->mHeaders[resip::Headers::CSeq]);
   }
   unsigned int CSeqCloneEnd=clock();
   
   unsigned int CSeqCloneTime=CSeqCloneEnd-CSeqCloneBegin;
   
   resip::HeaderFieldValueList* newCallIDInvites[runs];
   resip::HeaderFieldValueList* newCallIDRings[runs];
   resip::HeaderFieldValueList* newCallIDAnswers[runs];
   resip::HeaderFieldValueList* newCallIDAcks[runs];

   unsigned int CallIDCloneBegin=clock();
   for(int i=0;i<runs;i++)
   {
      newCallIDInvites[i]= new resip::HeaderFieldValueList(*invites[i]->mHeaders[resip::Headers::CallID]);
      newCallIDRings[i]= new resip::HeaderFieldValueList(*rings[i]->mHeaders[resip::Headers::CallID]);
      newCallIDAnswers[i]= new resip::HeaderFieldValueList(*answers[i]->mHeaders[resip::Headers::CallID]);
      newCallIDAcks[i]= new resip::HeaderFieldValueList(*acks[i]->mHeaders[resip::Headers::CallID]);
   }
   unsigned int CallIDCloneEnd=clock();
   
   unsigned int CallIDCloneTime=CallIDCloneEnd-CallIDCloneBegin;
   
   resip::HeaderFieldValueList* newMaxForwardsInvites[runs];
   resip::HeaderFieldValueList* newMaxForwardsAcks[runs];

   unsigned int MaxForwardsCloneBegin=clock();
   for(int i=0;i<runs;i++)
   {
      newMaxForwardsInvites[i]= new resip::HeaderFieldValueList(*invites[i]->mHeaders[resip::Headers::MaxForwards]);
      newMaxForwardsAcks[i]= new resip::HeaderFieldValueList(*acks[i]->mHeaders[resip::Headers::MaxForwards]);
   }
   unsigned int MaxForwardsCloneEnd=clock();
   
   unsigned int MaxForwardsCloneTime=MaxForwardsCloneEnd-MaxForwardsCloneBegin;
   
   resip::HeaderFieldValueList* newRecordRouteInvites[runs];
   resip::HeaderFieldValueList* newRecordRouteRings[runs];
   resip::HeaderFieldValueList* newRecordRouteAnswers[runs];
   resip::HeaderFieldValueList* newRouteAcks[runs];

   unsigned int RecordRouteCloneBegin=clock();
   for(int i=0;i<runs;i++)
   {
      newRecordRouteInvites[i]= new resip::HeaderFieldValueList(*invites[i]->mHeaders[resip::Headers::RecordRoute]);
      newRecordRouteRings[i]= new resip::HeaderFieldValueList(*rings[i]->mHeaders[resip::Headers::RecordRoute]);
      newRecordRouteAnswers[i]= new resip::HeaderFieldValueList(*answers[i]->mHeaders[resip::Headers::RecordRoute]);
      newRouteAcks[i]= new resip::HeaderFieldValueList(*acks[i]->mHeaders[resip::Headers::Route]);
   }
   unsigned int RecordRouteCloneEnd=clock();
   
   unsigned int RecordRouteCloneTime=RecordRouteCloneEnd-RecordRouteCloneBegin;
   
   resip::HeaderFieldValueList* newContactInvites[runs];
   resip::HeaderFieldValueList* newContactRings[runs];
   resip::HeaderFieldValueList* newContactAnswers[runs];
   resip::HeaderFieldValueList* newContactAcks[runs];

   unsigned int ContactCloneBegin=clock();
   for(int i=0;i<runs;i++)
   {
      newContactInvites[i]= new resip::HeaderFieldValueList(*invites[i]->mHeaders[resip::Headers::Contact]);
      newContactRings[i]= new resip::HeaderFieldValueList(*rings[i]->mHeaders[resip::Headers::Contact]);
      newContactAnswers[i]= new resip::HeaderFieldValueList(*answers[i]->mHeaders[resip::Headers::Contact]);
      newContactAcks[i]= new resip::HeaderFieldValueList(*acks[i]->mHeaders[resip::Headers::Contact]);
   }
   unsigned int ContactCloneEnd=clock();
   
   unsigned int ContactCloneTime=ContactCloneEnd-ContactCloneBegin;
   
   resip::HeaderFieldValueList* newProxyAuthorizationInvites[runs];
   resip::HeaderFieldValueList* newProxyAuthorizationAcks[runs];

   unsigned int ProxyAuthorizationCloneBegin=clock();
   for(int i=0;i<runs;i++)
   {
      newProxyAuthorizationInvites[i]= new resip::HeaderFieldValueList(*invites[i]->mHeaders[resip::Headers::ProxyAuthorization]);
      newProxyAuthorizationAcks[i]= new resip::HeaderFieldValueList(*acks[i]->mHeaders[resip::Headers::ProxyAuthorization]);
   }
   unsigned int ProxyAuthorizationCloneEnd=clock();
   
   unsigned int ProxyAuthorizationCloneTime=ProxyAuthorizationCloneEnd-ProxyAuthorizationCloneBegin;
   
   resip::HeaderFieldValueList* newContentLengthInvites[runs];
   resip::HeaderFieldValueList* newContentLengthRings[runs];
   resip::HeaderFieldValueList* newContentLengthAnswers[runs];
   resip::HeaderFieldValueList* newContentLengthAcks[runs];

   unsigned int ContentLengthCloneBegin=clock();
   for(int i=0;i<runs;i++)
   {
      newContentLengthInvites[i]= new resip::HeaderFieldValueList(*invites[i]->mHeaders[resip::Headers::ContentLength]);
      newContentLengthRings[i]= new resip::HeaderFieldValueList(*rings[i]->mHeaders[resip::Headers::ContentLength]);
      newContentLengthAnswers[i]= new resip::HeaderFieldValueList(*answers[i]->mHeaders[resip::Headers::ContentLength]);
      newContentLengthAcks[i]= new resip::HeaderFieldValueList(*acks[i]->mHeaders[resip::Headers::ContentLength]);
   }
   unsigned int ContentLengthCloneEnd=clock();
   
   unsigned int ContentLengthCloneTime=ContentLengthCloneEnd-ContentLengthCloneBegin;

   unsigned int MaxForwardsArrayAccessBegin=clock();
   for(int i=0;i<runs;i++)
   {
      *invites[i]->mHeaders[resip::Headers::MaxForwards];
      *acks[i]->mHeaders[resip::Headers::MaxForwards];
   }
   unsigned int MaxForwardsArrayAccessEnd=clock();
   
   unsigned int MaxForwardsArrayAccessTime=MaxForwardsArrayAccessEnd-MaxForwardsArrayAccessBegin;
#endif
// ?bwc? Wasn't being used... something wrong here?
//   const char* shortBuffer="Max-Forwards: 70\r\n";

   char* shortBuffers[runs*4];
   
   unsigned int shortBufferCreateBegin=clock();
   for(int i=0;i<runs*4;i++)
   {
      shortBuffers[i]=new char[18];
   }
   unsigned int shortBufferCreateEnd=clock();
   
   unsigned int shortBufferCreateTime=shortBufferCreateEnd-shortBufferCreateBegin;


   char* shortSourceBuffers[runs*4];
   for(int i=0;i<runs*4;i++)
   {
      shortSourceBuffers[i]=new char[18];
   }

   
   unsigned int shortBufferCopyBegin=clock();
   for(int i=0;i<runs*4;i++)
   {
      memcpy(shortBuffers[i],shortSourceBuffers[i],18);
   }
   unsigned int shortBufferCopyEnd=clock();
   
   unsigned int shortBufferCopyTime=shortBufferCopyEnd-shortBufferCopyBegin;

   unsigned int shortBufferDeleteBegin=clock();
   for(int i=0;i<runs*4;i++)
   {
      delete [] shortBuffers[i];
   }
   unsigned int shortBufferDeleteEnd=clock();
   
   unsigned int shortBufferDeleteTime=shortBufferDeleteEnd-shortBufferDeleteBegin;
   
   
   resip::HeaderFieldValueList* hfvls[runs*4];
   
   unsigned int hfvlConstructBegin=clock();
   for(int i=0;i<runs*4;i++)
   {
      hfvls[i]=new resip::HeaderFieldValueList;
   }
   unsigned int hfvlConstructEnd=clock();
   
   unsigned int hfvlConstructTime=hfvlConstructEnd-hfvlConstructBegin;
   
   resip::HeaderFieldValueList* hfvlClones[runs*4];
   
   unsigned int hfvlCloneBegin=clock();
   for(int i=0;i<runs*4;i++)
   {
      hfvlClones[i]=new resip::HeaderFieldValueList(*hfvls[i]);
   }
   unsigned int hfvlCloneEnd=clock();
   
   unsigned int hfvlCloneTime=hfvlCloneEnd-hfvlCloneBegin;
   
   
   std::vector<resip::HeaderFieldValue*>* vectors[runs*4];
   unsigned int vectorConstructBegin=clock();
   for(int i=0;i<runs*4;i++)
   {
      vectors[i]=new std::vector<resip::HeaderFieldValue*>;
   }
   unsigned int vectorConstructEnd=clock();
   
   unsigned int vectorConstructTime=vectorConstructEnd-vectorConstructBegin;
   
   unsigned int vectorDestructBegin=clock();
   for(int i=0;i<runs*4;i++)
   {
      delete vectors[i];
   }
   unsigned int vectorDestructEnd=clock();
   
   unsigned int vectorDestructTime=vectorDestructEnd-vectorDestructBegin;
   
   unsigned int vectorConstruct0Begin=clock();
   for(int i=0;i<runs*4;i++)
   {
      vectors[i]=new std::vector<resip::HeaderFieldValue*>(0);
   }
   unsigned int vectorConstruct0End=clock();
   
   unsigned int vectorConstruct0Time=vectorConstruct0End-vectorConstruct0Begin;
   
   unsigned int vectorDestruct0Begin=clock();
   for(int i=0;i<runs*4;i++)
   {
      delete vectors[i];
   }
   unsigned int vectorDestruct0End=clock();
   
   unsigned int vectorDestruct0Time=vectorDestruct0End-vectorDestruct0Begin;
   
   
   unsigned int msgDeleteBegin=clock();
   for(int i=0;i<runs;i++)
   {
      delete invites[i];
      delete rings[i];
      delete answers[i];
      delete acks[i];
   }
   unsigned int msgDeleteEnd=clock();
   
   unsigned int msgDeleteTime=msgDeleteEnd-msgDeleteBegin;

   unsigned int hfvlDeleteBegin=clock();
   for(int i=0;i<runs*4;i++)
   {
      delete hfvls[i];
   }
   unsigned int hfvlDeleteEnd=clock();
   
   unsigned int hfvlDeleteTime=hfvlDeleteEnd-hfvlDeleteBegin;
   
   resip::HeaderFieldValue* hfvs[runs*4];
   
   unsigned int hfvConstructBegin=clock();
   for(int i=0;i<runs*4;i++)
   {
      hfvs[i]=new resip::HeaderFieldValue;
   }
   unsigned int hfvConstructEnd=clock();
   
   unsigned int hfvConstructTime=hfvConstructEnd-hfvConstructBegin;
   
   resip::HeaderFieldValue* hfvClones[runs*4];
   
   unsigned int hfvCloneBegin=clock();
   for(int i=0;i<runs*4;i++)
   {
      hfvClones[i]=new resip::HeaderFieldValue(*hfvs[i]);
   }
   unsigned int hfvCloneEnd=clock();
   
   unsigned int hfvCloneTime=hfvCloneEnd-hfvCloneBegin;

   
   unsigned int dataConstructDestructBegin=clock();
   for(int i=0;i<runs*40;i++)
   {
      resip::Data test(i);
   }
   unsigned int dataConstructDestructEnd=clock();
   
   unsigned int dataConstructDestructTime=dataConstructDestructEnd-dataConstructDestructBegin;

   unsigned int dataHeapConstructDestructBegin=clock();
   for(int i=0;i<runs*40;i++)
   {
      resip::Data* test = new resip::Data(i);
      delete test;
   }
   unsigned int dataHeapConstructDestructEnd=clock();
   
   unsigned int dataHeapConstructDestructTime=dataHeapConstructDestructEnd-dataHeapConstructDestructBegin;

   unsigned int dataConstructCopyDestructBegin=clock();
   for(int i=0;i<runs*40;i++)
   {
      resip::Data test(i);
      resip::Data copy(test);
   }
   unsigned int dataConstructCopyDestructEnd=clock();
   
   unsigned int dataConstructCopyDestructTime=dataConstructCopyDestructEnd-dataConstructCopyDestructBegin;


   std::cout << "Time for " << 4*runs << " buffer allocations: " << allocTime<< std::endl;
   std::cout << "Time for " << 4*runs << " buffer copies: " << copyTime<< std::endl;
   std::cout << "Time for " << 4*runs << " SipMessage constructions: " << constructSipMessagesTime<< std::endl;
   std::cout << "Time for " << 4*runs << " buffer adds (to SipMessages) " << addBuffersTime<< std::endl;
   std::cout << "Time for " << 4*runs << " preparses: " << preparseTime<< std::endl;
   std::cout << "Time for " << 4*runs << " basic validations: " << validateTime<< std::endl;
   std::cout << "Time for " << 4*runs << " basic validations(again, already parsed): " << validate2Time<< std::endl;
   std::cout << "Time for " << 4*runs << " creations/destructions of oDataStreams: " <<testingOverhead << std::endl;
   std::cout << "Time for " << 4*runs<< " invocations of encode: " <<encodeTime << std::endl;
   std::cout << "Time for "  << 4*runs<< " encodes using raw data stream: " <<rawDataStreamTime << std::endl;
   std::cout << "Time for " << 4*runs << " encodes using memcopy (implementation of operator += in Data): " << memCopyTime<< std::endl;
   std::cout << "Time for " << 4*runs << " clones of SipMessage: " << msgCopyTime << std::endl;
#if 0
   std::cout << "Time for " << 2*runs << " clones of h_RequestLine: " << requestLineCloneTime << std::endl;
   std::cout << "Time for " << 4*runs << " clones of Via: " << ViaCloneTime << std::endl;
   std::cout << "Time for " << 4*runs << " clones of To: " << ToCloneTime << std::endl;
   std::cout << "Time for " << 4*runs << " clones of From: " << FromCloneTime << std::endl;
   std::cout << "Time for " << 4*runs << " clones of CSeq: " << CSeqCloneTime << std::endl;
   std::cout << "Time for " << 4*runs << " clones of CallID: " << CallIDCloneTime << std::endl;
   std::cout << "Time for " << 4*runs << " clones of Contact: " << ContactCloneTime << std::endl;
   std::cout << "Time for " << 4*runs << " clones of Route/RecordRoute: " << RecordRouteCloneTime << std::endl;
   std::cout << "Time for " << 2*runs << " clones of ProxyAuthorization: " << ProxyAuthorizationCloneTime << std::endl;
   std::cout << "Time for " << 4*runs << " clones of ContentLength: " << ContentLengthCloneTime << std::endl;
   std::cout << "Time for " << 2*runs << " clones of MaxForwards: " << MaxForwardsCloneTime << std::endl;
   std::cout << "Time for " << 2*runs << " array accesses of MaxForwards: " << MaxForwardsArrayAccessTime << std::endl;
#endif
   std::cout << "Time for " << 4*runs << " creations of small(18 byte) char buffers: " << shortBufferCreateTime << std::endl;
   std::cout << "Time for " << 4*runs << " copies of small(18 byte) char buffers: " << shortBufferCopyTime << std::endl;
   std::cout << "Time for " << 4*runs << " deletions of small(18 byte) char buffers: " << shortBufferDeleteTime << std::endl;
   std::cout << "Time for " << 4*runs << " creations of empty HeaderFieldValueLists: " << hfvlConstructTime << std::endl;
   std::cout << "Time for " << 4*runs << " clones of empty HeaderFieldValueLists: " << hfvlCloneTime << std::endl;
   std::cout << "Time for " << 4*runs << " deletions of empty HeaderFieldValueLists: " << hfvlDeleteTime << std::endl;
   std::cout << "Time for " << 4*runs << " creations of empty HeaderFieldValues: " << hfvConstructTime << std::endl;
   std::cout << "Time for " << 4*runs << " clones of empty HeaderFieldValues: " << hfvCloneTime << std::endl;
   std::cout << "Time for " << 4*runs << " deletions of (cloned) SipMessage: " << msgCopyDeleteTime << std::endl;
   std::cout << "Time for " << 4*runs << " deletions of (original) SipMessage: " << msgDeleteTime << std::endl;
   std::cout << "Time for " << 4*runs << " creations of default vector<HeaderFieldValue*>: " << vectorConstructTime << std::endl;
   std::cout << "Time for " << 4*runs << " deletions of default vector<HeaderFieldValue*>: " << vectorDestructTime << std::endl;
   std::cout << "Time for " << 4*runs << " creations of vector<HeaderFieldValue*>(0): " << vectorConstruct0Time << std::endl;
   std::cout << "Time for " << 4*runs << " deletions of vector<HeaderFieldValue*>(0): " << vectorDestruct0Time << std::endl;
   std::cout << "Time for " << 40*runs << " stack creations/destructions of empty Data: " << dataConstructDestructTime << std::endl;
   std::cout << "Time for " << 40*runs << " heap creations/destructions of empty Data: " << dataHeapConstructDestructTime << std::endl;
   std::cout << "Time for " << 40*runs << " stack creations/copy construct/destructions of empty Data: " << dataConstructCopyDestructTime << std::endl;

   return 0;
   
}

/* Copyright 2007 Estacado Systems */
