#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

#include "resip/stack/ParserCategories.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Uri.hxx"

#include "TestSupport.hxx"

static void fuzzNameAddr(const resip::Data& buffer)
{
  try
  {
    resip::NameAddr illegal(buffer);
  }
  catch (resip::ParseException)
  {
  }
}

static void fuzzSdp(const unsigned char *data, unsigned long size)
{
  resip::HeaderFieldValue hfv(reinterpret_cast<const char*>(data), size);
  resip::Mime type("application", "sdp");
  resip::SdpContents sdp(hfv, type);
  try
  {
    sdp.checkParsed();
  }
  catch (resip::ParseException)
  {
  }
}

static void fuzzSip(const resip::Data& buffer)
{
  try
  {
    std::auto_ptr<resip::SipMessage> msg(resip::TestSupport::makeMessage(buffer));
  }
  catch (resip::ParseException)
  {
  }
}

static void fuzzUri(const resip::Data& buffer)
{
  try
  {
    resip::Uri uri(buffer);
  }
  catch (resip::ParseException)
  {
  }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv) {
  resip::Log::initialize(resip::Log::Cout, resip::Log::None, *argv[0]);
  return 0;
}

// Entrypoint for Clang's libfuzzer
extern "C" int LLVMFuzzerTestOneInput(const unsigned char *data,
                                      unsigned long size) {

  const resip::Data buffer(resip::Data::Share, reinterpret_cast<const char*>(data), size);

  fuzzNameAddr(buffer);
  fuzzSdp(data, size);
  fuzzSip(buffer);
  fuzzUri(buffer);

  return 0;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
 */
