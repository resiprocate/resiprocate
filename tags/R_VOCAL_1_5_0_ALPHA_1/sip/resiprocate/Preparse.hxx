#if !defined(PREPARSE_HXX)
#define PREPARSE_HXX

#include <sys/types.h>
#include <limits.h>

#include <sipstack/HeaderTypes.hxx>


namespace Vocal2
{
   class SipMessage;            // fwd decl

   namespace PreparseStateTable
   {
      // Our actions

      const int actNil   = 0;
      const int actAdd   = (1 << 0);
      const int actBack  = (1 << 1);
      const int actFline = (1 << 2);
      const int actReset = (1 << 3);
      const int actHdr   = (1 << 4);
      const int actData  = (1 << 5);
      const int actBad   = (1 << 6);
      const int actEndHdrs = (1 << 7);

  
      typedef enum {
         NewMsg = 0,
         NewMsgCrLf,
         StartLine,
         StartLineCrLf,
         BuildHdr,
         EWSPostHdr,
         EWSPostColon,
         BuildData,
         BuildDataCrLf,
         CheckCont,
         CheckEndHdr,
         InQ,
         InQEsc,
         InAng,
         InAngQ,
         InAngQEsc,
         Done,
         lastStateMarker
      } State;

      typedef enum {
         dCommaSep,
         dContinuous,
         lastDispositionMarker
      } Disposition;

      const int nStates = lastStateMarker;
      const int nDisp   = 2;
      const int nOct    = UCHAR_MAX+1;
    
      /// Our sizes
    
      typedef struct 
      {
            State nextState;
            int workMask;
      } Edge;
  
      void InitStatePreparseStateTable();

   }


   class Preparse
   {
      public:
         Preparse(SipMessage& sipMsg, const char * buffer, size_t length);
         Preparse(SipMessage& sipMsg);
        
         bool process();

         void addBuffer(const char * buffer, size_t length);

      private:
         SipMessage& mSipMessage;
                                // we are pre-parsing this message

         const char * mBuffer;	// the char buffer
         size_t mLength;		// 

         PreparseStateTable::Disposition mDisposition;
         // the disposition of this machine, a function
         // of the mHeader enum
      
         PreparseStateTable::State mState;
         // the state of the machine we are in

         const char * mPtr;		// the current traversal pointer
      
         const char * mHeader;	// the current Header string
         size_t mHeaderLength;  // length of header
         Headers::Type mHeaderType; // the enum for the currently
                                // active header
         

         const char * mAnchorBeg;		// A curious place we anchored.
         // The location of the last actReset.
         const char * mAnchorEnd;

         bool mDone;
        
   };
 
}

#endif
