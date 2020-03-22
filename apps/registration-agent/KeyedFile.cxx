
#include <stdexcept>
#include <fstream>

#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "AppSubsystem.hxx"
#include "KeyedFile.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::REGISTRATIONAGENT

using namespace registrationagent;
using namespace resip;
using namespace std;

KeyedFileLine::KeyedFileLine(SharedPtr<KeyedFile> keyedFile, const Data& key)
   : mKeyedFile(keyedFile),
     mKey(key)
{
}

void
KeyedFileLine::onLineRemoved(SharedPtr<KeyedFileLine> sp)
{
   mSharedPtr = sp;
}

const Data&
KeyedFileLine::getKey()
{
   return mKey;
}

BasicKeyedFileLine::BasicKeyedFileLine(SharedPtr<KeyedFile> keyedFile, const Data& key, const vector<Data>& columns)
   : KeyedFileLine(keyedFile, key),
     mColumns(columns)
{
}

const Data&
BasicKeyedFileLine::getParam(const int index)
{
   return mColumns.at(index);
}

void
BasicKeyedFileLine::onFileReload(const std::vector<Data>& columns)
{
   bool changed = false;
   if(columns.size() != mColumns.size())
   {
      changed = true;
   }
   else
   {
      for(int i = 0; i < columns.size(); i++)
      {
         if(columns.at(i) != mColumns.at(i))
         {
            changed = true;
            break;
         }
      }
   }

   if(changed)
   {
      mColumns = columns;
      onLineChanged();
   }
}

KeyedFile::KeyedFile(const Data& filename, SharedPtr<KeyedFileRowHandler> rowHandler, const int minimumColumns, const int maximumColumns)
    : mFilename(filename),
      mRowHandler(rowHandler),
      mMinimumColumns(minimumColumns),
      mMaximumColumns(maximumColumns)
{
}

KeyedFile::~KeyedFile()
{
}

void
KeyedFile::doReload()
{
   readFile();
}

SharedPtr<KeyedFileLine>
KeyedFile::getByKey(const Data& key)
{
   map<Data, SharedPtr<KeyedFileLine> >::iterator it = mLines.find(key);
   if(it != mLines.end())
   {
      return it->second;
   }
   return SharedPtr<KeyedFileLine>();
}

void
KeyedFile::readFile()
{
   Lock readLock(mLinesMutex);

   InfoLog(<< "trying to load file " << mFilename);

   ifstream f(mFilename.c_str());
   if(!f)
   {
      ErrLog(<< "failed to open file: " << mFilename);
      throw runtime_error("Error opening/reading file");
   }

   string sline;
   int lineNo = 0;
   set<Data> keys;
   while(getline(f, sline))
   {
      lineNo++;
      Data line(sline);
      Data key;
      vector<Data> columns;
      ParseBuffer pb(line);

      pb.skipWhitespace();
      const char * anchor = pb.position();
      if(pb.eof() || *anchor == '#') continue;  // if line is a comment or blank then skip it

      // Look for end of key
      pb.skipToOneOf("\r\n\t");
      pb.data(key, anchor);
      if(keys.find(key) != keys.end())
      {
         ErrLog(<< "Key '" << key << "' repeated in file " << mFilename);
         throw runtime_error("Key repeated");
      }

      while(!pb.eof())
      {
         pb.skipChar();
         if(pb.eof())
            break;

         anchor = pb.position();
         pb.skipToOneOf("\r\n\t");
         Data value;
         pb.data(value, anchor);
         columns.push_back(value);
      }
      int columnCount = columns.size() + 1;

      StackLog(<< mFilename << " line " << lineNo << " key '" << key << "' column count " << columnCount);

      if(columnCount < mMinimumColumns)
      {
         ErrLog(<< mFilename << " line " << lineNo << " only has " << columnCount << " columns, need " << mMinimumColumns);
         throw runtime_error("insufficient columns");
      }

      if(mMaximumColumns > 0 && columnCount > mMaximumColumns)
      {
         ErrLog(<< mFilename << " line " << lineNo << " has " << columnCount << " columns, maximum permitted = " << mMaximumColumns);
         throw runtime_error("too many columns");
      }

      if(mLines.find(key) == mLines.end())
      {
         StackLog(<< " key '" << key << "' is new");
         mLines[key] = mRowHandler->onNewLine(mSharedPtr, key, columns);
      }
      else
      {
         StackLog(<< " key '" << key << "' already known");
         mLines[key]->onFileReload(columns);
      }
      keys.insert(key);
   }

   // Process removed lines
   set<Data> removedKeys;
   for(map<Data, SharedPtr<KeyedFileLine> >::iterator it = mLines.begin();
      it != mLines.end(); it++)
   {
      if(keys.find(it->first) == keys.end())
      {
         StackLog(<< mFilename << " removing key '" << it->first << "'");
         removedKeys.insert(it->first);
         it->second->onLineRemoved(it->second);
      }
   }
   for(set<Data>::iterator it = removedKeys.begin();
      it != removedKeys.end(); it++)
   {
      mLines.erase(*it);
   }

   InfoLog(<<"Processed " << lineNo << " lines");
}

std::size_t
KeyedFile::getLineCount()
{
   Lock readLock(mLinesMutex);

   return mLines.size();
}

/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock http://danielpocock.com  All rights reserved.
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
 * ====================================================================
 *
 *
 */
