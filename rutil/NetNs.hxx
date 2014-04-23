#if !defined(RESIP_NETNS_HXX) && defined(USE_NETNS)
#define RESIP_NETNS_HXX 

#include <vector>
#include "rutil/HashMap.hxx"
#include "rutil/BaseException.hxx"

namespace resip
{

class NetNs 
{
   /**
    * Class containing utilities for working with netns (network name spaces).
    * netns is supported in linux kernal 3.X on some distros.  netns in the 
    * kernel, provides the ability to have multiple network stacks.  As the
    * sockets interfaces are not changed, one uses setns to context switch
    * between the different named netns to create a socket in the desired
    * network stack.
    *
    **/
   
   public:

      class Exception : public BaseException
      {
      public:
         Exception(const Data& message, const Data& fileName, int lineNumber) :
             BaseException(message, fileName, lineNumber){};

         virtual const char* name() const {return("NetNs::Exception");};
      };

      virtual ~NetNs();

      // @ brief sets the kernel netns for the current thread
      static int setNs(const Data& netNs);
      /**
       * Set the netns (network namespace) for the current thread to
       * the given namespace name.
       *
       * @param netns[in] - name of the netns to switch to
       *
       * @returns netns id from NetNs dictionary if success
       *
       * @throws Exception if unable to open netns or switch to the
       *         specified netns
       *
       */

      // @brief gets public netns namespace names
      static int getPublicNetNs(std::vector<Data>& netNsNames);
      /**
       * Looks up in the system, the list of public netns namespace
       * names.  This may be different than the set of names in the
       * NetNs class maintained dictionary which only tracks netns names
       * that have been used by setNs.
       *
       * @param[in/out] netNsNames - vector containing system public netns
       *       names.
       *
       * @returns number of names found
       *
       **/

      // @brief Get the netns id for the given name in the netns dictonary.
      static int getNetNsId(const Data& netNsName, bool shouldAdd = false);
      /**
       *  Optionally adds the netns name to the dictonary.
       *
       *  @param netNsName[in] - name of the netns to look up in the dictonary
       *
       *  @param shouldAdd[in] - if false, throws exception if name is not
       *         already in the dictionary.  If true, will add the name to
       *         the dictionary if not found.
       *
       *  @returns netns id
       *
       **/

      // @brief construct netns device name to open for name
      static void makeNetNsDeviceName(const Data& netNs, Data& deviceName);
      /**
       * Constructs full pathname for netns name which is to be opened
       * for setting the netns.
       *
       * @param netNs[in] - netns namespace name
       * @param deviceName[out] - full pathname to open for setting netns
       *
       **/

      // @brief Opens the file for the named netns
      static int openNetNs(const Data& netNs);
      /**
       * Opens the file associated with the named netns
       *
       * @param netns[in] - name of the netns to attempt opening
       *
       * @returns a valid file descriptor or error
       *
       **/

      // @brief Checks if the named netns exists
      static bool netNsExists(const Data& netNs);
      /**
       * Tests if the file for the named netns can be opened.
       * Closes the file if successful.
       *
       * @param netns[in] - name of the netns to attempt opening
       *
       * @returns true/false if named netns exists and can be opened.
       *
       **/

      // @brief Find the netns name for the given netns dictionary id
      static const Data& getNetNsName(int netNsId);
      /**
       *  Looks for the netns name in the netns dictionary.  Throws exception
       *  if not found.
       *
       *  @returns netns name
       **/

   private:
      // Currently this is just a container for static methods.  So
      // no reason to actually construct one of these.
      NetNs();
      
      NetNs(const NetNs& );
      NetNs& operator=(const NetNs&);

      static void initDictionary();

      static HashMap<int, Data> sIdDictionaryToNetNs; ///< Map from netns id to netns name
      static HashMap<Data, int> sNetNsDictionaryToId; ///< Map from netns name to netns id
};

}

#endif

/* ====================================================================
 *
 * Copyright (c) Daniel Petrie, SIPez LLC.  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * vi: set shiftwidth=3 expandtab:
 */
