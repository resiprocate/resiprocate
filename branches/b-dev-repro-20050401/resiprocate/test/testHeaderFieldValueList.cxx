#include "resiprocate/StringSubComponent.hxx"
#include "resiprocate/SubComponentList.hxx"
#include "resiprocate/HeaderFieldValueList.hxx"
#include <iostream>

using namespace resip;
using namespace std;

int main(int argc, char** argv)
{

  HeaderFieldValueList *h = new HeaderFieldValueList;
  cout << "Empty list: " << endl;
  cout << *h << endl;

  HeaderFieldValue *myHeader1 = new HeaderFieldValue("foobar", 6);
  HeaderFieldValue *myHeader2 = new HeaderFieldValue("weekend", 7);
  
  h->push_front(myHeader1);
  h->push_front(myHeader2);
   
  cout << "List with two elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  myHeader1 = new HeaderFieldValue("asdfgh", 6);
  myHeader2 = new HeaderFieldValue("qwertyu", 7);

  h->push_front(myHeader1);
  h->push_front(myHeader2);

  cout << "List with four elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  myHeader1 = new HeaderFieldValue("poifds", 6);
  myHeader2 = new HeaderFieldValue("nmbhers", 7);

  h->push_back(myHeader1);
  h->push_back(myHeader2);

  cout << "List with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  HeaderFieldValueList *i = new HeaderFieldValueList(*h);

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with six elements: " << endl;
  cout << *i;
  cout << "Front of i " << *(i->first) << endl;
  cout << "Back of i " << *(i->last) << endl;
  cout << endl;

  i->pop_front();

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with five elements: " << endl;
  cout << *i;
  cout << "Front of i " << *(i->first) << endl;
  cout << "Back of i " << *(i->last) << endl;
  cout << endl;

  i->pop_front();

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with four elements: " << endl;
  cout << *i;
  cout << "Front of i " << *(i->first) << endl;
  cout << "Back of i " << *(i->last) << endl;
  cout << endl;

  i->pop_front();
  i->pop_front();
  i->pop_front();

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with one element: " << endl;
  cout << *i;
  cout << "Front of i " << *(i->first) << endl;
  cout << "Back of i " << *(i->last) << endl;
  cout << endl;

  i->pop_front();

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with zero element: " << endl;
  cout << *i;
  cout << "Front of i " << (i->first) << endl;
  cout << "Back of i " << (i->last) << endl;
  cout << endl;

  myHeader1 = new HeaderFieldValue("jgjgjg", 6);
  myHeader2 = new HeaderFieldValue("asasasa", 7);

  i->push_front(myHeader1);

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with one element: " << endl;
  cout << *i;
  cout << "Front of i " << *(i->first) << endl;
  cout << "Back of i " << *(i->last) << endl;
  cout << endl;



  i->push_front(myHeader2);

  cout << "List h with six elements: " << endl;
  cout << *h;
  cout << "Front of h " << *(h->first) << endl;
  cout << "Back of h " << *(h->last) << endl;
  cout << endl;

  cout << "List i with two elements: " << endl;
  cout << *i;
  cout << "Front of i " << *(i->first) << endl;
  cout << "Back of i " << *(i->last) << endl;
  cout << endl;

  



   /*
   SubComponentList* p = new SubComponentList;
   cout << *p << endl;
   assert(p->find(SubComponent::TTL) == 0);
   delete p;

   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   cout << *p << endl;
   assert(p->find(SubComponent::TTL)->getType() == SubComponent::TTL);
   delete p;

   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   p->insert(new StringSubComponent(SubComponent::Transport, "bar"));
   cout << *p << endl;
   assert(p->find(SubComponent::TTL)->getType() == SubComponent::TTL);
   assert(p->find(SubComponent::Transport)->getType() == SubComponent::Transport);
   delete p;

   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::Method, "baz"));
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   p->insert(new StringSubComponent(SubComponent::Transport, "bar"));
   cout << *p << endl;
   assert(p->find(SubComponent::TTL)->getType() == SubComponent::TTL);
   assert(p->find(SubComponent::Transport)->getType() == SubComponent::Transport);
   assert(p->find(SubComponent::Method)->getType() == SubComponent::Method);
   delete p;

   cout << "Finished Insertion Tests." << endl;
   
   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   p->erase(SubComponent::TTL);
   cout << *p << endl;
   assert(p->find(SubComponent::TTL) == 0);
   delete p;

   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   p->insert(new StringSubComponent(SubComponent::Transport, "bar"));
   p->erase(SubComponent::TTL);
   cout << *p << endl;
   assert(p->find(SubComponent::TTL) == 0);
   delete p;

   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::Method, "baz"));
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   p->insert(new StringSubComponent(SubComponent::Transport, "bar"));
   p->erase(SubComponent::TTL);
   cout << *p << endl;
   assert(p->find(SubComponent::TTL) == 0);
   delete p;

   cout << "Deep copy test." << endl;

   p = new SubComponentList();
   p->insert(new StringSubComponent(SubComponent::Method, "baz"));
   p->insert(new StringSubComponent(SubComponent::TTL, "foo"));
   p->insert(new StringSubComponent(SubComponent::Transport, "bar"));
   
   cout << *p << endl;

   SubComponentList* p2 = new SubComponentList(*p);

   delete p;

   cout << *p2 << endl;

   delete p2;
   */


    return 0;
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
