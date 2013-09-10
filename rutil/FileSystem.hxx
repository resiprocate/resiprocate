#ifndef RESIP_FileSystem_hxx
#define RESIP_FileSystem_hxx

#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"

#if !defined(WIN32)
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#if defined(__ANDROID__)
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <unistd.h>
#include <dirent.h>
#else
#include <direct.h>
#include "rutil/Socket.hxx"
#endif


namespace resip
{

/**
   @brief Provides file-system traversal. Wraps the POSIX/Windows 
   implementation, depending on environment.
*/
class FileSystem
{
   public:

      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg,
                      const Data& file,
                      const int line)
               : BaseException(msg, file, line) {}
         protected:
            virtual const char* name() const { return "ConfigParse::Exception"; }
      };

      class Directory
      {
         public:
            typedef Data value_type;
            typedef value_type* pointer;
            typedef const value_type* const_pointer;
            typedef value_type& reference;
            typedef const value_type& const_reference;

            Directory(const Data& path);
            
            class iterator
            {
               public:
                  iterator(const Directory& dir);
                  iterator();
                  ~iterator();

                  iterator& operator++();
                  bool operator!=(const iterator& rhs) const;
                  bool operator==(const iterator& rhs) const;
                  const Data& operator*() const;
                  const Data* operator->() const;
                  bool is_directory() const;
               private:
#ifdef WIN32
                  HANDLE mWinSearch;
                  bool mIsDirectory;
#else
                  DIR* mNixDir;
                  struct dirent* mDirent;
#endif
                  Data mFile;
                  Data mPath;
                  Data mFullFilename;
            };

            iterator begin() const;
            iterator end() const;
            const Data& getPath() const { return mPath; }
            int create() const;
         private:
            Data mPath;
      };
};   

}

#endif

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
