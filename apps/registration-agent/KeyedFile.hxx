#ifndef KEYEDFILE_HXX
#define KEYEDFILE_HXX

#include <rutil/ConfigParse.hxx>
#include <rutil/Data.hxx>
#include <rutil/Lock.hxx>

#include <memory>

namespace registrationagent {

class KeyedFile;

class KeyedFileLine
{
public:
   KeyedFileLine(std::shared_ptr<KeyedFile> keyedFile, const resip::Data& key);
   virtual ~KeyedFileLine() {};
   const resip::Data& getKey();
   virtual const int paramCount() = 0;
   virtual const resip::Data& getParam(const int index) = 0;
   virtual void onLineRemoved(std::shared_ptr<KeyedFileLine> sp);
   virtual void onFileReload(const std::vector<resip::Data>& columns) = 0;
protected:
   void readyForDeletion() { mSharedPtr.reset(); };
private:
   std::shared_ptr<KeyedFile> mKeyedFile;
   const resip::Data mKey;
   std::shared_ptr<KeyedFileLine> mSharedPtr;
};

class BasicKeyedFileLine : public KeyedFileLine
{
public:
   BasicKeyedFileLine(std::shared_ptr<KeyedFile> keyedFile, const resip::Data& key, const std::vector<resip::Data>& columns);
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
   virtual std::shared_ptr<KeyedFileLine> onNewLine(std::shared_ptr<KeyedFile> keyedFile, const resip::Data& key, const std::vector<resip::Data>& columns) = 0;
};

class KeyedFile
{

public:
   KeyedFile(const resip::Data& filename, std::shared_ptr<KeyedFileRowHandler> rowHandler, const int minimumColumns = 1, const int maximumColumns = -1);
   virtual ~KeyedFile();
   void setSharedPtr(std::shared_ptr<KeyedFile> sp) { mSharedPtr = sp; };

   virtual void doReload();

   std::shared_ptr<KeyedFileLine> getByKey(const resip::Data& key);

   std::size_t getLineCount();

private:
   void readFile();

   resip::Data mFilename;
   std::shared_ptr<KeyedFileRowHandler> mRowHandler;
   int mMinimumColumns, mMaximumColumns;
   std::shared_ptr<KeyedFile> mSharedPtr;

   resip::Mutex mLinesMutex;
   std::map<resip::Data, std::shared_ptr<KeyedFileLine> > mLines;
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
