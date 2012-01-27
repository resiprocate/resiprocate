/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_PARAMETER_HXX)
#define RESIP_PARAMETER_HXX 

#include "rutil/Data.hxx"
#include <iosfwd>
#include "resip/stack/ParameterTypeEnums.hxx"


namespace resip
{
/**
   @ingroup sip_grammar
   @brief This is the base type for a header or URI parameter.  Basic 
      support for serializing and identifying the parameter is available here.
   @class Parameter   
   @see ParameterTypes
**/
class Parameter
{
   public:
      /**
        @brief constructor; sets the type; otherwise empty
        @param type This is an enum that can take any of the values covered
         in the ParameterTypes class
        */
      Parameter(ParameterTypes::Type type);

      /**
        @brief empty destructor
        */
      virtual ~Parameter() {}
      
      /**
        @brief getter for the type
        */
      inline ParameterTypes::Type getType() const
      {
         return mType;
      }

      /**
        @brief getter for the name. does a lookup in the 
         ParameterTypes::ParameterNames[] array for the type and returns it.
         @note Only overridden in UnknownParameter. Might be worth refactoring 
            someday.
        */
      virtual const Data& getName() const
      {
         return ParameterTypes::ParameterNames[mType];
      }

      /**
        @brief pure virtual for making a copy of this
        */
      virtual Parameter* clone() const = 0;

      /**
        @brief pure virtual function for sending data to the ostream.
         implemented by inheriting classes based on the data that they hold.
        */
      virtual std::ostream& encode(std::ostream& stream) const = 0;

      /**
        @brief getter to see whether parameter is enclosed in quotes eg. "foo"
         This is only used in DataParameter. Everything else uses this 
         virtual function.
        */
      virtual bool isQuoted() const { return false; }
      /**
        @brief set whether you want the parameter to be enclosed in quotes 
         eg. "foo" This is only used in DataParameter. 
         Everything else uses this virtual function.
        */
      virtual void setQuoted(bool /*b*/) { };

   protected:
      /**
        @brief copy constructor; copies the type but does nothing else
         by default.
        */
      Parameter(const Parameter& other) : mType(other.mType) {};

      /**
        @brief assignment constructor; implemented by inheriting classes.
        */
      Parameter& operator=(const Parameter& other);

   private:
      ParameterTypes::Type mType;
};

}

#endif

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
