/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "resip/stack/Compression.hxx"

#ifdef USE_SIGCOMP
#include <osc/StateHandler.h>
#include <osc/Stack.h>
#include <osc/DeflateCompressor.h>
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//resip::Compression resip::Compression::Disabled(resip::Compression::NONE);

resip::Compression&
resip::Compression::Disabled()
{
    static resip::Compression* disabled = new resip::Compression(resip::Compression::NONE);
    return *disabled;
} // of Disabled

/**
  @brief Constructor for SigComp compression parameters. 
  
  All parameters are given reasonable defaults. 
  Users are encouraged to understand the parameters
  described in RFC 3320 before changing any of the values from their
  defaults. Further, it should be noted that certain values for the
  memory sizes are disallowed in certain networks -- for example,
  in IMS networks, state memory size must be at least 4096.

  @param algorithm          Specifies the compression algorithm to be used
                            by the SigComp stack.

  @param stateMemorySize    Controls the UDVM state memory size, as defined
                            in RFC 3320. Must be one of: 2048, 4096,
                            8192, 16384, 32768, 65536, or 131072.

  @param cyclesPerBit       Sets the maximum number of cycles per bit,
                            as defined in RFC 3320. Must be one of:
                            16, 32, 64, or 128.

  @param decompressionMemorySize
                            Controls the UDVM decompression memory
                            size, as defined in RFC 3320. Must be
                            one of: 2048, 4096, 8192, 16384, 32768,
                            65536, or 131072.

  @param sigcompId          Application-specified compartment ID used
                            to represent this network element.  Most
                            applications can leave this blank (=
                            Data::Empty), which will cause a random
                            UUID to be generated.  This ID should
                            be formatted as a formal UUID, surrounded
                            by '<' and '>' characters.  If the stack
                            is being used in a network server that
                            is a member of a redundant cluster that
                            shares all state -- including SigComp
                            state -- then this ID must be the same
                            across all servers in a cluster.

*/
resip::Compression::Compression(resip::Compression::Algorithm algorithm,
                                int stateMemorySize,
                                int cyclesPerBit,
                                int decompressionMemorySize,
                                Data sigcompId)
  : mAlgorithm(algorithm), mStateHandler(0), mSigcompId(sigcompId)
{
#ifdef USE_SIGCOMP
  if (algorithm != NONE)
  {
    mStateHandler = new osc::StateHandler(stateMemorySize,
                                          cyclesPerBit,
                                          decompressionMemorySize);
    mStateHandler->useSipDictionary();

    if (sigcompId == Data::Empty)
    {
      mSigcompId = "<";
      mSigcompId += Random::getVersion4UuidUrn();
      mSigcompId += ">";
    }
  }
  DebugLog (<< "Set SigcompId to " << mSigcompId);
#else
  mAlgorithm = NONE;
  DebugLog (<< "COMPRESSION SUPPORT NOT COMPILED IN");
#endif
  DebugLog (<< "Compression configuration object created; algorithm = "
            << static_cast<int>(mAlgorithm) );
}

resip::Compression::~Compression()
{
#ifdef USE_SIGCOMP
  delete mStateHandler;
#endif
}

/**
   @internal
   @brief Called by transports at initialization to add the correct
   SigComp compressors to a SigComp stack. 

   @param stack The SigComp stack to which the compressor(s)
                are to be added.

   @note This method should not be called by the application.
*/

void
resip::Compression::addCompressorsToStack(osc::Stack *stack)
{
#ifdef USE_SIGCOMP
  switch(getAlgorithm())
  {
    case DEFLATE:
      DebugLog (<< "Adding Deflate Compressor");
      stack->addCompressor(new osc::DeflateCompressor(getStateHandler()));
      break;

    default:
      WarningLog (<< "Invalid compressor specified! Using deflate Compressor");
      stack->addCompressor(new osc::DeflateCompressor(getStateHandler()));

    case NONE:
      DebugLog (<< "Compression disabled: not adding any compressors");
      break;

  }
#else
   DebugLog (<< "Compression not compiled in: not adding any compressors");
#endif
}

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
 * The Vovida Software License, Version 1.0 
*
* Copyright (c) 2002-2005 Vovida Networks, Inc.  All rights reserved.
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
