
#include <util/Socket.hxx>

#include <iostream> // debug only !ah!

#include <util/Data.hxx>
#include <util/Logger.hxx>
#include <sipstack/Preparse.hxx>
#include <sipstack/HeaderTypes.hxx>
#include <sipstack/SipMessage.hxx>

#define VOCAL_SUBSYSTEM Subsystem::SIP

using namespace Vocal2;

// Table helpers
//  AE(int start, int disposition, const char *p, int next, int workMask);
//  AE(int start, int disposition, int ch, int next, int workMask);

using namespace Vocal2::PreparseStateTable;

Edge *** mTransitionTable = 0;


#if !defined(NDEBUG) && defined(DEBUG)

Data showN(const char * p, size_t l)
{
   Data s;
   
   for(unsigned int i = 0 ; i < l ; i++)
      s += p[i];
   return s;
}

const char *  stateName(PreparseStateTable::State s)
{
   switch (s)
   {
      case NewMsg: return "NewMsg";
      case NewMsgCrLf: return "NewMsgCrLf";
      case StartLine: return "StartLine";
      case StartLineCrLf: return "StartLineCrLf";
      case BuildHdr: return "BuildHdr";
      case EWSPostHdr: return "EWSPostHdr";
      case EWSPostColon: return "EWSPostColon";
      case BuildData: return "BuildData";
      case BuildDataCrLf: return "BuildDataCrLf";
      case CheckCont: return "CheckCont";
      case CheckEndHdr: return "CheckEndHdr";
      case InQ: return "InQ";
      case InQEsc: return "InQEsc";
      case InAng: return "InAng";
      case InAngQ: return "InAngQ";
      case InAngQEsc: return "InAngQEsc";
      case Done: return "Done";
      default: return "**UNK**";
   }
}


Data
workString(int m)
{
   Data s("[");

   if ( m &  actNil) s += " Nil ";
   if ( m &  actAdd) s += " Add ";
   if ( m &  actBack) s += " Back ";
   if ( m &  actFline) s += " Fline ";
   if ( m &  actReset) s += " Reset ";
   if ( m &  actHdr) s += " Hdr ";
   if ( m &  actData) s += " Data ";
   if ( m &  actBad) s += " Bad ";
   if ( m & actEndHdrs) s += " EndHdrs ";
   s += ']';
   return s;
}

#endif

const int X = -1;
const char XC = -1;

void
AE ( PreparseStateTable::State start,
     int disposition,
     int c,
     PreparseStateTable::State next,
     int workMask)
{
   // change back to <0 iff it will work !ah!
   int stateStart = (start==X) ? 0 : start;
   int stateEnd = (start==X) ? nStates : start+1;

   int dispStart = (disposition==X) ? 0 : disposition;
   int dispEnd = (disposition==X) ? nDisp : disposition+1;
   
   int charStart = (c==X) ? 0 : (int)c;
   int charEnd = (c==X) ? nOct : (int)(c+1);

   for(int st = stateStart ; st < stateEnd ; st++)
      for(int di = dispStart ; di < dispEnd ; di++)
	 for(int ch = charStart ; ch < charEnd ; ch++)
	 {
	    Edge& e( mTransitionTable[st][di][ch] );
	    e.nextState = next;
	    e.workMask = workMask;
	 }
}

static void
AE (PreparseStateTable::State start,
    int disposition,
    const char * p,
    PreparseStateTable::State next,
    int workMask)
{
   const char *q = p;
   while(*q)
      AE (start, disposition, (int)*q++, next, workMask);
}

void
PreparseStateTable::InitStatePreparseStateTable()
{
     
   static bool initialised = false;
   
   // !ah! This needs to be mutexd

   if (initialised) return;

   mTransitionTable = new Edge**[nStates];
   for(int i = 0 ; i < nStates ; i++)
   {
      mTransitionTable[i] = new Edge*[nDisp];
      for(int j = 0 ; j < nDisp ; j++)
      {
         mTransitionTable[i][j] = new Edge[nOct];
         for(int k = 0 ; k < nOct ; k ++)
         {
            Edge& e(mTransitionTable[i][j][k]);
            e.nextState = NewMsg;
            e.workMask = 0;
         }
      }
   }

   // Assert that the Symbols package is as we expect.
   // Better than redefining symbols here.
   // This is only done once at initialisation.

   assert(Symbols::CR && strlen(Symbols::CR) == 1);
   assert(Symbols::LF && strlen(Symbols::LF) == 1);
   assert(Symbols::LA_QUOTE && strlen(Symbols::LA_QUOTE) == 1);
   assert(Symbols::RA_QUOTE && strlen(Symbols::RA_QUOTE) == 1);
   assert(Symbols::DOUBLE_QUOTE && strlen(Symbols::DOUBLE_QUOTE) == 1);
   assert(Symbols::COLON && strlen(Symbols::COLON) == 1);
   assert(Symbols::B_SLASH && strlen(Symbols::B_SLASH) == 1);
   assert(Symbols::COMMA && strlen(Symbols::COMMA) == 1);
   assert(Symbols::SPACE && strlen(Symbols::SPACE) == 1);
   assert(Symbols::TAB && strlen(Symbols::TAB) == 1);

   // Setup the table with useful transitions.
   // NOTE: This is done to (1) make them ints (2) make the automatic diagram
   // have reasonable symbol names.  DO NOT put Symbols::XX in the AE() calls.

   const int CR = (int)(*Symbols::CR);
   const int LF = (int)(*Symbols::LF);
   const int LAQUOT = (int)(*Symbols::LA_QUOTE);
   const int RAQUOT = (int)(*Symbols::RA_QUOTE);
   const int QUOT = (int)(*Symbols::DOUBLE_QUOTE);
   
   const int COLON = (int)(*Symbols::COLON);
   const int LSLASH = (int)(*Symbols::B_SLASH);
   const int COMMA = (int)(*Symbols::COMMA);
   const char LWS[3]  =
      {
         (*Symbols::SPACE),
         (*Symbols::TAB),
         0
      }
   ;


   // AE -- add edge(s)
   // AE ( state, disp, char, newstate, work)

   // MUST be AE( format ) so fsm generation will work.
   AE( NewMsg,X,XC,StartLine,actAdd); // all chars
   AE( NewMsg,X,CR,NewMsgCrLf,actNil); // eat CR

   AE( NewMsgCrLf,X,XC,Done,actBad);
   AE( NewMsgCrLf,X,LF,NewMsg,actNil); 

   AE( StartLine,X,XC,StartLine,actAdd);
   AE( StartLine,X,CR,StartLineCrLf,actNil);

   AE( StartLineCrLf,X,XC,Done,actBad);
   AE( StartLineCrLf,X,LF,BuildHdr,actReset | actFline);

   AE( BuildHdr,X,XC, BuildHdr,actAdd);
   AE( BuildHdr,X,LWS,EWSPostHdr,actNil);
   AE( BuildHdr,X,COLON,EWSPostColon,actHdr | actReset);
  
   AE( EWSPostHdr,X,XC,Done,actBad);
   AE( EWSPostHdr,X,LWS,EWSPostHdr,actNil);
   AE( EWSPostHdr,X,COLON,EWSPostColon,actHdr | actReset);

   AE( EWSPostColon,X,XC,BuildData,actAdd);
   AE( EWSPostColon,X,LWS,EWSPostColon,actReset );

   AE( BuildData,X,XC,BuildData,actAdd);
   AE( BuildData,X,CR,BuildDataCrLf,actNil);

   AE( BuildDataCrLf,X,XC,Done,actBad);
   AE( BuildDataCrLf,X,LF,CheckCont,actNil);

   // (push 1st then b/u)
   AE( CheckCont,X,XC, BuildHdr,actData|actReset|actBack );
   AE( CheckCont,X,LWS,BuildData,actAdd );
   
   // Check if double CRLF (end of hdrs)
   AE( CheckCont,X,CR,CheckEndHdr,actData|actReset);
   AE( CheckEndHdr,X,XC,Done,actBad);
   AE( CheckEndHdr,X,LF,Done,actEndHdrs);

   // Disposition sensitive edges

   AE( BuildData,dCommaSep,LAQUOT,InAng,actAdd);

   // Angle Quotes
   AE(InAng,X,XC,InAng,actAdd);
   AE(InAng,X,RAQUOT,BuildData,actAdd);
   AE(InAng,X,QUOT,InAngQ,actAdd);

   AE(InAngQ,X,XC,InAngQ,actAdd);
   AE(InAngQ,X,QUOT,InAng,actAdd);
   AE(InAngQ,X,LSLASH,InAngQEsc,actAdd);

   AE(InAngQEsc,X,XC,InAngQ,actAdd);

   // Bare Quotes
   AE(BuildData,dCommaSep,QUOT,InQ,actAdd);
   AE(InQ,X,XC,InQ,actAdd);
   AE(InQ,X,QUOT,BuildData,actAdd);
   AE(InQ,X,LSLASH,InQEsc,actAdd);
   AE(InQEsc,X,XC,InQ,actAdd);
   
   // add comma transition
   AE(BuildData,dCommaSep,COMMA,EWSPostColon, actData|actReset);
   
   initialised = true;
  
}


Preparse::Preparse(SipMessage& sipMsg):
   mSipMessage(sipMsg),
   mBuffer(0), mLength(0), 
   mDisposition(dContinuous),
   mState(PreparseStateTable::NewMsg),
   mPtr(mBuffer), mHeader(0), mHeaderLength(0),
   mAnchorBeg(0),
   mAnchorEnd(0),
   mDone(false)
{
   InitStatePreparseStateTable();
}

Preparse::Preparse(SipMessage& sipMsg, const char * buffer, size_t length):
   mSipMessage(sipMsg),
   mBuffer(buffer),
   mLength(length), 
   mDisposition(dContinuous),
   mState(PreparseStateTable::NewMsg),
   mPtr(mBuffer),
   mHeader(0),
   mHeaderLength(0),
   mAnchorBeg(buffer),
   mAnchorEnd(buffer),
   mDone(false)
{
   InitStatePreparseStateTable();
}

void
Preparse::addBuffer(const char * buffer, size_t length)
{
   mBuffer = buffer;
   mAnchorBeg = mAnchorEnd = mBuffer = buffer; // will change when wraps
                                               // implemented !ah!
   mHeader = 0;
   mHeaderLength = 0;
   mLength = length;
   mPtr = buffer;
   mDone = false;
#if defined(DEBUG)
   cout << "added buffer" << mBuffer << ' ' << mLength << endl;
#endif
}

#if defined(DEBUG)
#include <ctype.h>

ostream& showchar(ostream& os, char c)
{
   if (isprint(c))
      os << c;
   else
      os << (int)c;

   return os;
   
}
#endif


bool
Preparse::process()
{
   while ( (static_cast<size_t>(mPtr - mBuffer)) < mLength && !mDone)
   {
      //using namespace PreparseStateTable;
      Edge& e(mTransitionTable[mState][mDisposition][*mPtr]);

#if defined(DEBUG)
      DebugLog( << "EDGE " << ::stateName(mState)
                << " (" << (int) *mPtr << ')'
                << " -> " << ::stateName(e.nextState)
                << ::workString(e.workMask) );
#endif      
      
      if (e.workMask & actAdd)
      {
	 mAnchorEnd = mPtr;
#if defined(DEBUG)         
         DebugLog( << "+++Adding char '"
                   << showN(cout,mAnchorBeg, mAnchorEnd-mAnchorBeg+1)
                   << '\'' );
#endif
      }

      if (e.workMask & actHdr) // this edge indicates a header was seen
      {
         mHeader = mAnchorBeg;
         mHeaderLength = mAnchorEnd-mAnchorBeg+1;
         mHeaderType = Headers::getType(mHeader, mHeaderLength);

         // ask header class if this is a header that needs to be split on commas?
         if ( Headers::isCommaTokenizing( mHeaderType ))
         {
            mDisposition = dCommaSep;
         }
         else
         {
            mDisposition = dContinuous;
         }
#if defined(DEBUG)         
         DebugLog(<<"Hdr \'"
                  << showN(mHeader, mHeaderLength)
                  << "\' Type: " << int(mHeaderType) );
#endif
      }

      if (e.workMask & actData) // there is some header data to pass up
      {
         mSipMessage.addHeader(mHeaderType,
                               mHeader,
                               mHeaderLength,
                               mAnchorBeg,
                               mAnchorEnd - mAnchorBeg + 1
            );
#if defined(DEBUG)                   
         DebugLog(<<"DATA \'"
                  << showN(mAnchorBeg, mAnchorEnd - mAnchorBeg + 1)
                  << "\'");
#endif
      }

      if (e.workMask & actFline) // first line complete.
      {
#if defined(DEBUG)
         DebugLog(<<"FLINE \'"
                  << showN(mAnchorBeg, mAnchorEnd - mAnchorBeg + 1)
                  << "\'");
#endif
         mSipMessage.setStartLine(mAnchorBeg, mAnchorEnd - mAnchorBeg + 1);
      }

      if (e.workMask & actBack)
      {
         mPtr--;
      }

      if (e.workMask & actBad)
      {
         DebugLog(<<"BAD");
         mDone = true;
      }

      if (e.workMask & actEndHdrs)
      {
         DebugLog(<<"END_HDR");
         mDone = true;
      }
      
      mState = e.nextState;
      mPtr++;

      if (e.workMask & actReset)
      {
	 mAnchorBeg = mAnchorEnd = mPtr;
      }
      
   }
   return mDone;
   
}
