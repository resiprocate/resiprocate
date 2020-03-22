#ifndef KEYEDFILE_HXX
#define KEYEDFILE_HXX

#include <rutil/ConfigParse.hxx>
#include <rutil/Data.hxx>
#include <rutil/Lock.hxx>
#include <rutil/SharedPtr.hxx>

namespace registrationagent {

class KeyedFile;

class KeyedFileLine
{
public:
   KeyedFileLine(resip::SharedPtr<KeyedFile> keyedFile, const resip::Data& key);
   virtual ~KeyedFileLine() {};
   const resip::Data& getKey();
   virtual const int paramCount() = 0;
   virtual const resip::Data& getParam(const int index) = 0;
   virtual void onLineRemoved(resip::SharedPtr<KeyedFileLine> sp);
   virtual void onFileReload(const std::vector<resip::Data>& columns) = 0;
protected:
   void readyForDeletion() { mSharedPtr.reset(); };
private:
   resip::SharedPtr<KeyedFile> mKeyedFile;
   const resip::Data mKey;
   resip::SharedPtr<KeyedFileLine> mSharedPtr;
};

class BasicKeyedFileLine : public KeyedFileLine
{
public:
   BasicKeyedFileLine(resip::SharedPtr<KeyedFile> keyedFile, const resip::Data& key, const std::vector<resip::Data>& columns);
   virtual ~BasicKeyedFileLine() {};
   virtual const int paramCount() { return mColumns.size(); };
   virtual const resip::Data& getParam(const int index);
   virtual void onFileReload(const std::vector<resip::Data>& columns);
   virtual void onLineChanged() = 0;
private:
   std::vector<resip::Data> mColumns;
   
};

class KeyedFileRowHandler
{
public:
   virtual resip::SharedPtr<KeyedFileLine> onNewLine(resip::SharedPtr<KeyedFile> keyedFile, const resip::Data& key, const std::vector<resip::Data>& columns) = 0;
};

class KeyedFile
{

public:
   KeyedFile(const resip::Data& filename, resip::SharedPtr<KeyedFileRowHandler> rowHandler, const int minimumColumns = 1, const int maximumColumns = -1);
   virtual ~KeyedFile();
   void setSharedPtr(resip::SharedPtr<KeyedFile> sp) { mSharedPtr = sp; };

   virtual void doReload();

   resip::SharedPtr<KeyedFileLine> getByKey(const resip::Data& key);

   std::size_t getLineCount();

private:
   void readFile();

   resip::Data mFilename;
   resip::SharedPtr<KeyedFileRowHandler> mRowHandler;
   int mMinimumColumns, mMaximumColumns;
   resip::SharedPtr<KeyedFile> mSharedPtr;

   resip::Mutex mLinesMutex;
   std::map<resip::Data, resip::SharedPtr<KeyedFileLine> > mLines;
};

} // namespace

#endif

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
