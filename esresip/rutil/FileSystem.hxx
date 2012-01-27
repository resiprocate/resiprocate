/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#ifndef RESIP_FileSystem_hxx
#define RESIP_FileSystem_hxx

#include "rutil/Data.hxx"

#if !defined(WIN32)
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <dirent.h>
#else
#include "rutil/Socket.hxx"
#endif


namespace resip
{

/**
   @brief Provides file-system traversal. Wraps the POSIX/Windows 
   implementation, depending on environment.
   @example
   @code
    #include "rutil/FileSystem.hxx"
    #include <iostream>

    using namespace resip;
    using namespace std;

    int main()
    {
    #ifdef WIN32
       FileSystem::Directory dir("c:\\windows\\");
    #else
       FileSystem::Directory dir("/usr/bin/");
    #endif
       for (FileSystem::Directory::iterator it = dir.begin(); 
            it != dir.end(); ++it) 
       {
          cerr << *it << endl;
       }
       return 0;
    }
   @endcode
*/
class FileSystem
{
   public:
       /**
       @brief Represents a directory on a filesystem and presents an
       OS independent traversal method.
       @details Represents a directoy on the file system. The
       Directory::iterator class is provided for traversal over
       the contents of the directory in an stl like manner.
       @todo add failure avenues other than asserts
       */
      class Directory
      {
         public:
            typedef Data value_type;
            typedef value_type* pointer;
            typedef const value_type* const_pointer;
            typedef value_type& reference;
            typedef const value_type& const_reference;
            /**
            @brief construct a directory representation of the filesystem directory at
            a given path
            @param path the path to the directory
            */
            Directory(const Data& path);
            /**
            @brief used to iterate through a directory listing
            @details an iterator with an stl like interface for
            traversing over the contetns of a directory
            @note this is a forward only iterator
            */
            class iterator
            {
               public:
                  /**
                  @brief create an iterator over the contents of a directory
                  @param dir the directory to iterate over the contents of
                  */
                  iterator(const Directory& dir);
                  /**
                  @brief brief constructor
                  */
                  iterator();
                  /**
                  @brief destructor
                  */
                  ~iterator();
                  /**
                  @brief iterate to the next directory member
                  @return a reference to itself
                  */
                  iterator& operator++();
                  /**
                  @brief compares itself to another iterator for inequality
                  @param rhs the iterator on the right hand side of the inequality
                  @return true if and only if the two iterators are not at the same
                    directory member
                  */
                  bool operator!=(const iterator& rhs) const;
                  /**
                  @brief compares itself to another iterator for equality
                  @param rhs the iterator on the right hand side of the equality
                  @return true if and only if the two iterators are at the same
                    directory member
                  */
                  bool operator==(const iterator& rhs) const;
                  /**
                  @brief fetch the name of the file that iterator currently points to
                  @return the name of the file that the iterator currently points to
                  */
                  const Data& operator*() const;
                  /**
                  @brief fetch the name of the file that iterator currently points to
                  @return the name of the file that the iterator currently points to
                  */
                  const Data* operator->() const;
               private:
#ifdef WIN32
                  HANDLE mWinSearch;
#else
                  DIR* mNixDir;
                  struct dirent* mDirent;
#endif
                  Data mFile;
            };
            /**
            @brief returns an iterator to the beginning of the list of directory contents
            @note when begin() == end() the list is empty
            */
            iterator begin() const;
            /**
            @brief returns an iterator after the end of the list of directory contents
            @note when begin() == end() the list is empty
            */
            iterator end() const;
            /**
            @brief get the string representation of the path to the directory
            @return the path to the directory
            */
            const Data& getPath() const { return mPath; }
         private:
            Data mPath;
      };
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
