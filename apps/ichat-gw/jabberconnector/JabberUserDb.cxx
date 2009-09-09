#include "JabberUserDb.hxx"

#include <iostream>
#include <fstream>
#include <iterator>

using namespace gateway;
using namespace std;

namespace gateway 
{

JabberUserDb::JabberUserDb()
   : mDbFilename("ichat-gw.userdb")
{
   parseDbFile(mDbFilename);
}

JabberUserDb::~JabberUserDb()
{
}

void 
JabberUserDb::addSubscribedJID(const std::string& user, const std::string& subscribedJID)
{
   mUsers[user].insert(subscribedJID);
   writeDbFile(mDbFilename);
}

void 
JabberUserDb::removeSubscribedJID(const std::string& user, const std::string& subscribedJID)
{
   bool found = false;
   UserMap::iterator it = mUsers.find(user);
   if(it!=mUsers.end())
   {
      SubscribeSet::iterator it2 = it->second.find(subscribedJID);
      if(it2!=it->second.end())
      {
         found = true;
         it->second.erase(it2);
      }
      // Remove user entirely if no subscriptions left
      if(it->second.size() == 0)
      {
         mUsers.erase(it);
      }
   }
   if(found)
   {
      writeDbFile(mDbFilename);
   }
}

void
JabberUserDb::parseDbFile(const std::string& filename)
{
   ifstream dbFile(filename.c_str());
   string lastUser;
   string sline;                     

   // Get first line and ensure version is present
   if(getline(dbFile, sline))
   {
      if(sline != "v1")  // For now we just read version 1 format
      {
         cerr << "JabberUserDb::parseDbFile: invalid first line in file, expecting version line!" << endl;
         return;
      }
   }
   else
   {
      cerr << "JabberUserDb::parseDbFile: invalid first line in file, expecting version line!" << endl;
      return;
   }
   while(getline(dbFile, sline)) 
   {
      if(sline.size() > 1 &&
         sline.at(0) == '\t' &&
         !lastUser.empty())
      {
         // This should be a local subscription JID
         mUsers[lastUser].insert(sline.substr(1));
         //cout << "'" << lastUser << "': '" << sline.substr(1) << "'" << endl;
      }
      else
      {
         // This should be a user
         lastUser = sline;
      }
   }
}

void
JabberUserDb::writeDbFile(const std::string& filename)
{
   std::string tempfilename = filename + ".tmp";

   // safewrite - write to a temp file, delete original, then move temp file
   ofstream dbFile(tempfilename.c_str(), std::ios_base::out | std::ios_base::trunc);
   if(dbFile.is_open())
   {
      dbFile << "v1\n";
      UserMap::iterator it = mUsers.begin();
      for(;it!=mUsers.end();it++)
      {
         dbFile << it->first << "\n";
         SubscribeSet::iterator it2 = it->second.begin();
         for(;it2!=it->second.end();it2++)
         {
            dbFile << "\t" << *it2 << "\n";
         }
      }
      dbFile.close();
      if(remove(filename.c_str()) != 0)
      {
         cerr << "JabberUserDb::writeDbFile - error removing " << filename << " to update with new data." << endl;
      }
      else
      {
         if(rename(tempfilename.c_str(), filename.c_str()) != 0)
         {
            cerr << "JabberUserDb::writeDbFile - error renaming " << tempfilename << " to " << filename << " to update with new data." << endl;
         }
      }
   }
   else
   {
      cerr << "JabberUserDb::writeDbFile - error opening " << tempfilename << " for writing." << endl;
   }
}

}

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */

