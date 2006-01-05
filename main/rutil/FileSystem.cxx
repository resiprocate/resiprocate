#include "rutil/FileSystem.hxx"
#include "rutil/Logger.hxx"

#include <cassert>
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


static FileSystem::Directory::iterator staticEnd;


FileSystem::Directory::Directory(const Data& path)
   : mPath(path)
{
}


#ifndef WIN32
// !jf! added this constructor since it was missing - don't know if it is correct
FileSystem::Directory::iterator::iterator() : mNixDir(0), mDirent(0)
{
}


FileSystem::Directory::iterator::iterator(const Directory& dir)
{
   assert(!dir.getPath().empty());   
   //InfoLog(<< "FileSystem::Directory::iterator::iterator: " << dir.getPath());   
   if ((mNixDir = opendir( dir.getPath().c_str() )))
   {
      mDirent = readdir(mNixDir);
      if (mDirent)
      {
         //InfoLog(<< "FileSystem::Directory::iterator::iterator, first file " << mFile);   
         mFile = mDirent->d_name;
      }
   }
   else
   {
      mDirent = 0;
   }
}


FileSystem::Directory::iterator::~iterator()
{
   if (mNixDir)
   {
      closedir(mNixDir);
   }
}


FileSystem::Directory::iterator& 
FileSystem::Directory::iterator::operator++()
{
   if (mDirent)
   {
      mDirent = readdir(mNixDir);
      if (mDirent)
      {
         mFile = mDirent->d_name;
         //InfoLog(<< "FileSystem::Directory::iterator::iterator, next file " << mFile);   
      }
   }
   return *this;
}


bool 
FileSystem::Directory::iterator::operator!=(const iterator& rhs) const
{
   return !(*this == rhs);
}


bool 
FileSystem::Directory::iterator::operator==(const iterator& rhs) const
{
   if (mDirent && rhs.mDirent)
   {
      return **this == *rhs;
   }
   else
   {
      return mDirent == rhs.mDirent;
   }
}


const Data& 
FileSystem::Directory::iterator::operator*() const
{
   return mFile;
}


const Data*
FileSystem::Directory::iterator::operator->() const
{
   return &mFile;
}
#else


FileSystem::Directory::iterator::iterator() :
   mWinSearch(0)
{
}


FileSystem::Directory::iterator::iterator(const Directory& dir)
{
   Data searchPath;
   if (dir.getPath().postfix("/") || dir.getPath().postfix("\\"))
   {
      searchPath = dir.getPath() + Data("*");
   }
   else
   {
      searchPath = dir.getPath() + Data("/*");
   }
   WIN32_FIND_DATAA fileData;
   mWinSearch = FindFirstFileA( searchPath.c_str(), &fileData);
   
   if (mWinSearch == INVALID_HANDLE_VALUE)
   {
      mWinSearch = 0;
   }
   else
   {
      mFile = fileData.cFileName;
   }
}


FileSystem::Directory::iterator::~iterator()
{
   if (mWinSearch)
   {
      FindClose(mWinSearch);
   }
}


FileSystem::Directory::iterator&
FileSystem::Directory::iterator::operator++()
{
   WIN32_FIND_DATAA fileData;

   if (!FindNextFileA(mWinSearch, &fileData))
   {
      if (GetLastError() == ERROR_NO_MORE_FILES)
      {
         FindClose(mWinSearch);
         mWinSearch = 0;
      }
   }
   else
   {
      mFile = fileData.cFileName;
   }  
   return *this;
}


bool 
FileSystem::Directory::iterator::operator!=(const iterator& rhs) const
{
   return !(*this == rhs);
}


bool 
FileSystem::Directory::iterator::operator==(const iterator& rhs) const
{
   if (mWinSearch && rhs.mWinSearch)
   {
      return **this == *rhs;
   }
   else
   {
      return mWinSearch == rhs.mWinSearch;
   }
}


const Data& 
FileSystem::Directory::iterator::operator*() const
{
   return mFile;
}


const Data* 
FileSystem::Directory::iterator::operator->() const
{
   return &mFile;
}
#endif


FileSystem::Directory::iterator FileSystem::Directory::begin() const
{
   return iterator(*this);   
}


FileSystem::Directory::iterator FileSystem::Directory::end() const
{
   return staticEnd;   
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
