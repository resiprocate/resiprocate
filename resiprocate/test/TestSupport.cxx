#include "resiprocate/test/TestSupport.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

#include <iostream>
#include <iomanip>


using namespace std;


const int boxWidth = 4;
namespace resip 
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
   return SipMessage::make(data, isExternal);
}

};


