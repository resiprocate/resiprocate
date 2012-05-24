#include "CurlHttpProvider.hxx"
#include "resip/stack/external/HttpGetMessage.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "rutil/Logger.hxx"

#include <curl/curl.h>

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace resip;

CurlHttpProvider::CurlHttpProvider()
{
   curl_global_init(CURL_GLOBAL_DEFAULT);
}

CurlHttpProvider::~CurlHttpProvider()
{
}

void 
CurlHttpProvider::get(const GenericUri& target, const Data& tid, TransactionUser& tu)
{
   //fix cleanup logic, self deletes for now
   RequestThread* rt = new RequestThread(target, tid, tu);
   rt->run();   
}

CurlHttpProvider* 
CurlHttpProviderFactory::createHttpProvider()
{
   return new CurlHttpProvider();
}

CurlHttpProvider::RequestThread::RequestThread(const GenericUri& target, 
                             const Data& tid, 
                             TransactionUser& tu) :
   mTarget(target),
   mTid(tid),
   mTransactionUser(tu),
   mX509Blob(),
   mStream(mX509Blob)
{}

CurlHttpProvider::RequestThread::~RequestThread()
{
}

void 
CurlHttpProvider::RequestThread::thread()
{
   CURL *curl = curl_easy_init();
   if (!curl)
   {
       HttpGetMessage* res = new HttpGetMessage(mTid, false, Data::Empty, Mime());
       mTransactionUser.post(res);
   }

   curl_easy_setopt(curl, CURLOPT_URL, mTarget.uri().c_str());
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, RequestThread::curlCallback);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mStream);
   curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
   
   int code = 404;
   curl_easy_perform(curl);
#if ( LIBCURL_VERSION_NUM > 0x070a07 )
   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
#else
   curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &code);
#endif
   InfoLog ( << "Curl returned: " << code);   

    if (code / 100 == 2) 
    {
        //.dcm.  vodoo to trick the lazy parsers, should add convenience methods to
        //clean this up
        Mime contentType; 
        {
           char* contentTypeString;
           curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentTypeString);

           InfoLog ( << "Content type from curl: " << contentTypeString);
           
           if (contentTypeString)
           {
              HeaderFieldValue tmp(contentTypeString, strlen(contentTypeString));
              Mime tempMime(&tmp, Headers::ContentType);
              contentType = tempMime;
              delete contentTypeString;          
           }
        }
        mStream.flush();        
        HttpGetMessage* res = new HttpGetMessage(mTid, true, mX509Blob, contentType);
        mTransactionUser.post(res);
    }
    else
    {
       HttpGetMessage* res = new HttpGetMessage(mTid, false, Data::Empty, Mime());
       mTransactionUser.post(res);
    }
    curl_easy_cleanup(curl);

    delete this;
}

int 
CurlHttpProvider::RequestThread::curlCallback(void *ptr, size_t size, size_t nmemb, void *stream)
{
   DataStream* ds = reinterpret_cast<DataStream*>(stream);
   assert(size == 1);
   ds->write(reinterpret_cast<const char*>(ptr), nmemb);
   InfoLog ( << "Wrote: " << nmemb << " to stream" );   
   return nmemb;
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
