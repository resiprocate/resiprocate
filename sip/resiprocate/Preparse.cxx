#if defined(DEBUG)
#include <iostream.h> // debug only !ah!
#include <ctype.h> // debug only !ah!
#include <string>
#endif

#include <sipstack/Preparse.hxx>

using namespace Vocal2;

// Table helpers
//  AE(int start, int disposition, const char *p, int next, int workMask);
//  AE(int start, int disposition, int ch, int next, int workMask);

using namespace Vocal2::PreparseStateTable;

Edge *** mTransitionTable = 0;

#if defined(DEBUG)
// HACK REMOVE !ah!
#include <iostream>
ostream& showN(ostream& os, const char * p, size_t l)
{
   for(int i = 0 ; i < l ; i++)
      os << p[i];
   return os;
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
      case InQ: return "InQ";
      case InQEsc: return "InQEsc";
      case InAng: return "InAng";
      case InAngQ: return "InAngQ";
      case InAngQEsc: return "InAngQEsc";
      case Done: return "Done";
      default: return "**UNK**";
   }
}

ostream& operator<<(ostream& os, const PreparseStateTable::State s)
{
   return os << stateName(s);
}

 ostream& outStateRange(ostream& os, int s, int e)
 {
    if ( s == e-1)
    {
       os << (State)s;
    }
    else
    {
       os << "[ " << (State)s << " - " << (State)(e-1) << " ]";
    }
    return os ;
 }

//ostream& outStateRange(ostream& os, int s , int e)
//{
//   return os << '['<<s<<'-'<<e<<']';
//}

ostream& printable(ostream& os, char c)
{
   if (isprint(c) && ! isspace(c))
   {
      os << '\'' << c << "\' ";
   }

   os <<  "0x"<< hex << (int)c << dec;
   return os;
   
}

void
showWork(ostream& os, int m)
{
   os << '[';
   if ( m &  actNil) os << " actNil ";
   if ( m &  actAdd) os << " actAdd ";
   if ( m &  actBack) os << " actBack ";
   if ( m &  actFline) os << " actFline ";
   if ( m &  actReset) os << " actReset ";
   if ( m &  actHdr) os << " actHdr ";
   if ( m &  actData) os << " actData ";
   if ( m &  actBad) os << " actBad ";
   os << ']';
}

void
showEdge(const char*msg, State s, Disposition d, char c, Edge& e)
{
   cout << msg;
   cout << stateName(s);
   cout << hex << " (0x" << (int) c << ')';
   
   cout << " -> " << stateName(e.nextState);
   showWork(cout, e.workMask);
}

ostream& outCharRange(ostream& os, int s, int e)
{
   if ( s == e-1)
   {
      printable(os,s);
   }
   else
   {
      os << "[ ";
      printable(os, s) ;
      os << " - " ;
      printable(os, e-1);
      os << " ]";
   }
   return os;

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


   int stateStart = (start==X) ? 0 : start;
   int stateEnd = (start==X) ? nStates : start+1;

   int dispStart = (disposition==X) ? 0 : disposition;
   int dispEnd = (disposition==X) ? nDisp : disposition+1;
   
   int charStart = (c==X) ? 0 : (int)c;
   int charEnd = (c==X) ? nOct : (int)(c+1);

#if defined(DEBUG) && 0
   outStateRange(cout, stateStart, stateEnd) ;
   
   cout   << " -> "
          << stateName(next)
          << " : ";
   outCharRange(cout, charStart, charEnd);
   cout << endl;
#endif
   
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

  cout << "cleared table " << endl;
  // Setup the table with useful transitions.

  Edge *** t = mTransitionTable;
  // AE -- add edge(s)
  // AE ( state, disp, char, newstate, work)


  const int CR = (int)'\r';
  const int LF = (int)'\n';
  const int LAQUOT = (int)'<';
  const int RAQUOT = (int)'>';
  const int QUOT = (int)'\"';
  const int COLON = (int)':';
  const int LSLASH = (int)'\\';

  const char LWS[] = " \t";
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
  AE( CheckCont,X,XC, BuildHdr,actData | actReset | actBack ); // (push 1st then b/u)
  AE( CheckCont,X,LWS,BuildData,actAdd );

  // Disposition sensitive edges

  AE( BuildData,dCommaSep,LAQUOT,InAng,actAdd);

  AE( InAng,X,XC,InAng,actAdd);
  AE( InAng,X,RAQUOT,BuildData,actAdd);
  AE( InAng,X,QUOT,InAngQ,actAdd);

  AE( InAngQ, X, XC, InAngQ, actAdd);
  AE( InAngQ, X, QUOT, InAng, actAdd);
  AE( InAngQ, X, LSLASH, InAngQEsc, actAdd);
  
  // add quotes within la/ra pairs
  // add bare quotes from BuildData
  // add comma transition
}



Preparse::Preparse(const char * buffer, size_t length):
   mBuffer(buffer), mLength(length), mState(PreparseStateTable::NewMsg),
   mPtr(mBuffer), mHeader(0), mHeaderLength(0),
   mAnchorBeg(buffer), mAnchorEnd(buffer),
   mDisposition(dContinuous)
{
  static int initialised = 0;
  if (!initialised)
  {
     initialised = 1;
     InitStatePreparseStateTable();
  }

}
#include <ctype.h>

ostream& showchar(ostream& os, char c)
{
   if (isprint(c))
      os << c;
   else
      os << (int)c;
}
// END HACK REMOVE !ah!
bool
Preparse::process()
{
   cout << "process()" << endl;
   while ((mPtr - mBuffer) < mLength)
   {
      using namespace PreparseStateTable;
      Edge& e(mTransitionTable[mState][mDisposition][*mPtr]);
      
#if defined(DEBUG) && 0
      showEdge("selected edge ", mState, mDisposition, *mPtr, e);
      cout << endl;
#endif
      
      
      if (e.workMask & actAdd)
      {
	 mAnchorEnd = mPtr;
#if defined(DEBUG) && 0
	 cout << "+++Adding char '";
//         showchar(cout, *mPtr);
         showN(cout,mAnchorBeg, mAnchorEnd-mAnchorBeg+1);
         cout << '\'' << endl;
#endif
      }

      if (e.workMask & actHdr)
      {
         mHeader = mAnchorBeg;
         mHeaderLength = mAnchorEnd-mAnchorBeg+1;
#if defined(DEBUG)
         cout << "+++Found Header: \'";
         showN(cout, mHeader, mHeaderLength);
         cout << '\'' << endl;
#endif
      }

      if (e.workMask & actData)
      {
         cout << "+++Data element: \'";
         showN(cout, mAnchorBeg, mAnchorEnd-mAnchorBeg+1);
         cout << '\'' << endl;
         
      }
      if (e.workMask & actFline)
      {
#if defined(DEBUG)
	 cout << "FirstLine(" ;
         showN(cout, mAnchorBeg, mAnchorEnd-mAnchorBeg+1);
         cout << ")" << endl;
#endif
      }
      if (e.workMask & actBack)
      {
#if defined(DEBUG)
         cout << "+++Backing up " << endl;
#endif
         mPtr--;
      }

      mState = e.nextState;
      *mPtr++;

      if (e.workMask & actReset)
      {
	 mAnchorBeg = mAnchorEnd = mPtr;
#if defined(DEBUG) && 0
         cout << "+++Reset anchors." << endl;
#endif
      }

   }
}
