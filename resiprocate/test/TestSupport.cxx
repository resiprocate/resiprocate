#include "resiprocate/test/TestSupport.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/Preparse.hxx"

#include <iostream>
#include <iomanip>

using namespace std;


const int boxWidth = 4;
namespace Vocal2 
{
class TestSupportPriv
{
private:
  static int chPerRow;
  friend class TestSupport;

public:
  static void fb(int w, char c=' ')
  {
    for(int i = 0 ; i < boxWidth-w ; i++) cout << c;
  };
  static void labels(int len, int row)
  {
    int start = chPerRow*row;
    cout << ' ';
    for(int i = 0; i < chPerRow && start+i < len; i++)
      {
        cout << setw(boxWidth-1) << start+i;
        fb(3);
      }
    cout << endl;
  }
  static void banner(int len, int row)
  {
    int chThisRow = 0;
    
    if (row >= len/chPerRow)
      chThisRow = len%chPerRow;
    else
      chThisRow = chPerRow;
    
    if (chThisRow < 1) return;
    
    cout << "+";
    for(int i = 0 ; i < chThisRow; i++)
      {
        fb(1,'-');
        cout << '+';
      }
    cout << endl;
    return;
  };
  static void data(const char * p , int len, int row)
  {
    
    cout << '|';
    for(int c = 0; c < chPerRow; c++)
      {
        int o = row*chPerRow + c;
        if (o >= len) break;
        char ch = p[o];
        
        if (isalnum(ch) || ispunct(ch) || ch == ' ' )
          {
            cout << ' ' << (char)ch;
               fb(3);
          }
        else if ( ch == '\t' )
          {
            cout << " \\t";
            fb(4);
          }
        else if ( ch >= '\t' || ch <= '\r')
          {
            cout << " \\" << "tnvfr"[ch-'\t'];
               fb(4);
          }
        else
          {
            cout << 'x' << hex << setw(2) << ch << dec;
            fb(4);
          }
        cout << '|';
      }
    cout << endl;
  };
  
};
  
  
void 
TestSupport::prettyPrint(const char * p,size_t len)
{
  
  size_t row = 0;
  if (TestSupportPriv::chPerRow == 0)
    {
      char * p = getenv("COLUMNS");
            if (p)
            {
               TestSupportPriv::chPerRow=strtol(p,0,0)/boxWidth;
            }
            else
            {
               TestSupportPriv::chPerRow = 80/boxWidth;
            }
         }
         
         for ( row = 0 ; row <= len/TestSupportPriv::chPerRow ; row++)
         {
            // do this row's banner
            TestSupportPriv::banner(len,row);
            // do this row's data
            TestSupportPriv::data(p,len,row);
            // do this row's banner
            TestSupportPriv::banner(len,row);
            // do this row's counts
            TestSupportPriv::labels(len,row);
         }
      };

Data
TestSupport::showN(const char * p, size_t l)
{
  Data s;
  for(unsigned int i = 0 ; i < l ; i++)
    {
      s += p[i];
    }
  return s;
}


int TestSupportPriv::chPerRow = 0;

SipMessage*
TestSupport::makeMessage(const Data& data, bool isExternal )
{
   using namespace PreparseConst;
    
   SipMessage* msg = new SipMessage(isExternal ? SipMessage::FromWire : SipMessage::NotFromWire);

   size_t size = data.size();
   char *buffer = new char[size];

   msg->addBuffer(buffer);

   memcpy(buffer,data.data(),size);
   
   Preparse pre;

   if (pre.process(*msg, buffer, size) || pre.isFragmented())
   {
      cerr << "Preparser failed: isfrag=" << pre.isFragmented() << " buff=" << buffer;
      
      delete msg;
      msg = 0;
   }
   else
   {
       size_t used = pre.nBytesUsed();
       assert(pre.nBytesUsed() == pre.nDiscardOffset());
       
       // no pp error
       if (pre.isHeadersComplete() &&
           used < size)
      {
         // body is present .. add it up.
         // NB. The Sip Message uses an overlay (again)
         // for the body. It ALSO expects that the body
         // will be contiguous (of course).
         // it doesn't need a new buffer in UDP b/c there
         // will only be one datagram per buffer. (1:1 strict)
         msg->setBody(buffer+used,size-used);
      }
   }
   return msg;
}



};

