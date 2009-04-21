
#include "CDRHandler.hxx"
#include "Logging.hxx"

using namespace b2bua;

DailyCDRHandler::DailyCDRHandler(const char* basename) : 
  mBasename(basename),
  last_write(0) {
  
}

DailyCDRHandler::~DailyCDRHandler() {
  if(cdrStream.is_open())
    cdrStream.close();
}

void DailyCDRHandler::handleRecord(const std::string& record) {

  // First check the time
  updateTime();

  // Now write to file
  cdrStream << record << std::endl;
  cdrStream.flush();
}

int DailyCDRHandler::day_number(struct tm* tm) {
  return (tm->tm_year + 1900) * 10000 + ((tm->tm_mon + 1) * 100) + tm->tm_mday;
}

void DailyCDRHandler::updateTime() {
  time_t now;
  time(&now);
  struct tm* tm = gmtime(&now);
  int x = day_number(tm);
  if(x > last_write) {
    last_write = x;
    initFile(tm);
  }
}

void DailyCDRHandler::initFile(struct tm* tm) {
  if(cdrStream.is_open()) {
    cdrStream.close();
  }
  char buf[200];
  sprintf(buf, "%s-%04d-%02d-%02d.csv", mBasename.c_str(), tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
  cdrStream.open(buf, std::ios::out | std::ios::app);
  if(!cdrStream.is_open()) {
    B2BUA_LOG_ERR("Failed to open CDR file");
    throw;
  }
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

