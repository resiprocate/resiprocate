#include "resiprocate/MessageWaitingContents.hxx"
#include "resiprocate/HeaderFieldValue.hxx"
#include <iostream>

using namespace resip;
using namespace std;

int
main()
{
   {
      const Data txt("s\r\nk");
      ParseBuffer pb(txt.data(), txt.size());
      pb.skipChar();

      assert(*skipSipLWS(pb) == '\r');
   }

   {
      const Data txt("s\r\n\r\nk");
      ParseBuffer pb(txt.data(), txt.size());
      pb.skipChar();

      assert(skipSipLWS(pb) == txt.data()+1);
   }

   {
      const Data txt("Messages-Waiting: yes\r\n"
                     "Message-Account: sip:alice@vmail.example.com\r\n"
                     "Voice-Message: 4/8 (1/2)\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "simple-message-summary");
      MessageWaitingContents mwb(&hfv, type);

      assert(mwb.header(mw_account).scheme() == "sip");
      assert(mwb.header(mw_account).user() == "alice");
      assert(mwb.header(mw_account).host() == "vmail.example.com");
      
      assert(mwb.header(mw_voice).newCount() == 4);
      assert(mwb.header(mw_voice).oldCount() == 8);
      assert(mwb.header(mw_voice).urgent() == true);
      assert(mwb.header(mw_voice).urgentNewCount() == 1);
      assert(mwb.header(mw_voice).urgentOldCount() == 2);

      assert(mwb.exists(mw_fax) == false);
   }

   {
      const Data txt("Messages-Waiting: yes\r\n"
                     "Message-Account: sip:alice@vmail.example.com\r\n"
                     "Voice-Message: 4/8 (1/2)\r\n"
                     "Fax-Message: 2/1\r\n"
                     "\r\n"
                     "Something: fine choice\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "simple-message-summary");
      MessageWaitingContents mwb(&hfv, type);

      assert(mwb.header(mw_account).scheme() == "sip");
      assert(mwb.header(mw_account).user() == "alice");
      assert(mwb.header(mw_account).host() == "vmail.example.com");
      
      assert(mwb.header(mw_voice).newCount() == 4);
      assert(mwb.header(mw_voice).oldCount() == 8);
      assert(mwb.header(mw_voice).urgent() == true);
      assert(mwb.header(mw_voice).urgentNewCount() == 1);
      assert(mwb.header(mw_voice).urgentOldCount() == 2);

      assert(mwb.header(mw_fax).newCount() == 2);
      assert(mwb.header(mw_fax).oldCount() == 1);
      assert(mwb.header(mw_fax).urgent() == false);
      
      assert(mwb.header(Data("Something")) == Data("fine choice"));

      assert(mwb.exists(mw_fax) == true);
   }      

   cerr << "All OK" << endl;
}

