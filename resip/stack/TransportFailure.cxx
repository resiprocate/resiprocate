#include "TransportFailure.hxx"

using namespace resip;

TransportFailure::TransportFailure(const Data& transactionId, FailureReason f, int subCode)
   : mTransactionId(transactionId),
     mFailureReason(f), mFailureSubCode(subCode)
{
}

const Data&
TransportFailure::getTransactionId() const
{
   return mTransactionId;
}

bool
TransportFailure::isClientTransaction() const
{
   // !jf! not strictly true
   return true;
}

EncodeStream&
TransportFailure::encodeBrief(EncodeStream& str) const
{
   return str << "TransportFailure: " << mTransactionId;
}

EncodeStream&
TransportFailure::encode(EncodeStream& strm) const
{
   return encodeBrief(strm);
}

const char*
TransportFailure::failureReasonToString(FailureReason failureReason)
{
   switch (failureReason)
   {
   case TransportFailure::None:
      return "None";
   case TransportFailure::TransportNoExistConn:
      return "TransportNoExistConn";
   case TransportFailure::Failure:
      return "Failure";
   case TransportFailure::TransportNoSocket:
      return "TransportNoSocket";
   case TransportFailure::TransportBadConnect:
      return "TransportBadConnect";
   case TransportFailure::TransportShutdown:
      return "TransportShutdown";
   case TransportFailure::ConnectionUnknown:
      return "ConnectionUnknown";
   case TransportFailure::ConnectionException:
      return "ConnectionException";
   case TransportFailure::NoTransport:
      return "NoTransport";
   case TransportFailure::NoRoute:
      return "NoRoute";
   case TransportFailure::CertNameMismatch:
      return "CertNameMismatch";
   case TransportFailure::CertValidationFailure:
      return "CertValidationFailure";
   default:
      resip_assert(false);
      return "UNKNOWN";
   }
}

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 *
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 * vi: set shiftwidth=3 expandtab:
 */
