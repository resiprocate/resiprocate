
#include <sip2/Preparse.hxx>

using namespace Vocal2;

  // Table helpers
//  AE(int start, int disposition, const char *p, int next, int workMask);
//  AE(int start, int disposition, int ch, int next, int workMask);

using namespace Vocal2::PreparseStateTable;

Edge *** mTransitionTable = 0;

void
AE( PreparseStateTable::State start,
    int disposition,
    int c,
    PreparseStateTable::State next,
    int workMask)
{


   int stateStart = (start<0) ? 0 : start;
   int stateEnd = (start<0) ? nStates : start+1;

   int dispStart = (disposition<0) ? 0 : disposition;
   int dispEnd = (disposition<0) ? nDisp : disposition+1;
   
   int charStart = c<0 ? 0 : c;
   int charEnd = c<0 ? nOct : c+1;

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
AE(PreparseStateTable::State start,
   int disposition,
   const char * p,
   PreparseStateTable::State next,
   int workMask)
{
   const char *q = p;
   while(*q)
      AE(start, disposition, *q++, next, workMask);
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

  // Setup the table with useful transitions.

  Edge *** t = mTransitionTable;
  // AE -- add edge(s)
  // AE ( state, disp, char, newstate, work)

  const int X = -1 ; // dont care

  const int CR = (int)'\r';
  const int LF = (int)'\n';
  const int LAQUOT = (int)'<';
  const int RAQUOT = (int)'>';
  const int QUOT = (int)'\"';
  const int COLON = (int)':';


  const char LWS[] = " \t";

  AE( NewMsg, X, X, StartLine , actAdd); // all chars
  AE( NewMsg, X, CR, NewMsgCrLf, actNil); // eat CR

  AE( NewMsgCrLf, X, X, Done, actBad);
  AE( NewMsgCrLf, X, LF, NewMsg, actNil); 

  AE( StartLine, X, X, StartLine, actAdd);
  AE( StartLine, X, CR, StartLineCrLf, actNil);

  AE( StartLineCrLf, X, X, Done, actBad);
  AE( StartLineCrLf, X, LF, BuildHdr, actReset | actFline);

  AE( BuildHdr, X, X,  BuildHdr, actAdd);
  AE( BuildHdr, X, LWS, EWSPostHdr, actNil);
  AE( BuildHdr, X, COLON, EWSPostColon, actHdr | actReset);
  
  AE( EWSPostHdr, X, X, Done, actBad);
  AE( EWSPostHdr, X, LWS, EWSPostHdr, actNil);
  AE( EWSPostHdr, X, COLON, EWSPostColon, actHdr | actReset);

  AE( EWSPostColon, X, X, BuildData, actAdd);
  AE( EWSPostColon, X, LWS, EWSPostColon, actReset );

  AE( BuildData, X, X, BuildData, actAdd);
  AE( BuildData, X, CR, BuildDataCrLf, actNil);

  AE( BuildDataCrLf, X, X, Done, actBad);
  AE( BuildDataCrLf, X, LF, CheckCont, actNil);
  AE( CheckCont, X, X,  BuildHdr, actData | actBack ); // (push 1st then b/u)
  AE( CheckCont, X, LWS, BuildData, actAdd );

  // Disposition sensitive edges

  AE( BuildData, dCommaSep, LAQUOT, InAng, actAdd);

  AE( InAng, X, X, InAng, actAdd);
  AE( InAng, X, RAQUOT, BuildData, actAdd);

  // add quotes within la/ra pairs
  // add bare quotes from BuildData
  // add comma transition


}



Preparse::Preparse(const char * buffer, size_t length):
   mBuffer(buffer), mLength(length), mState(PreparseStateTable::NewMsg),
   mPtr(mBuffer), mHeader(0), mHeaderLength(0), mAnchorBeg(0), mAnchorEnd(0),
   mDisposition(dContinuous)
{
   ;
}
// HACK REMOVE !ah!
#include <iostream>
ostream& showN(ostream& os, const char * p, size_t l)
{
   for(int i = 0 ; i < l ; i++)
      os << p[i];
   return os;
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
   while ((mPtr - mBuffer) < mLength)
   {
      using namespace PreparseStateTable;
      Edge& e(mTransitionTable[mState][mDisposition][*mPtr]);
      
      if (e.workMask | actAdd)
      {
	 cout << "Adding char " << showchar(cout, *mPtr) << endl;
	 mAnchorEnd = mPtr;
      }
      if (e.workMask | actFline)
      {
	 cout << "FirstLine(" <<  showN(cout, mAnchorBeg,mPtr-mAnchorBeg)
	      << ")" << endl;
      }
      if (e.workMask | actReset)
      {
	 mAnchorBeg = mAnchorEnd = mPtr;
      }

      mState = e.nextState;
      
   }
}
