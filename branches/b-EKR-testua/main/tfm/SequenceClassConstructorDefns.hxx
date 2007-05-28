
SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             TestEndPoint::ExpectBase* e89,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
   addExpect(e89);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             TestEndPoint::ExpectBase* e89,
                             TestEndPoint::ExpectBase* e90,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
   addExpect(e89);
   addExpect(e90);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             TestEndPoint::ExpectBase* e89,
                             TestEndPoint::ExpectBase* e90,
                             TestEndPoint::ExpectBase* e91,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
   addExpect(e89);
   addExpect(e90);
   addExpect(e91);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             TestEndPoint::ExpectBase* e89,
                             TestEndPoint::ExpectBase* e90,
                             TestEndPoint::ExpectBase* e91,
                             TestEndPoint::ExpectBase* e92,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
   addExpect(e89);
   addExpect(e90);
   addExpect(e91);
   addExpect(e92);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             TestEndPoint::ExpectBase* e89,
                             TestEndPoint::ExpectBase* e90,
                             TestEndPoint::ExpectBase* e91,
                             TestEndPoint::ExpectBase* e92,
                             TestEndPoint::ExpectBase* e93,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
   addExpect(e89);
   addExpect(e90);
   addExpect(e91);
   addExpect(e92);
   addExpect(e93);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             TestEndPoint::ExpectBase* e89,
                             TestEndPoint::ExpectBase* e90,
                             TestEndPoint::ExpectBase* e91,
                             TestEndPoint::ExpectBase* e92,
                             TestEndPoint::ExpectBase* e93,
                             TestEndPoint::ExpectBase* e94,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
   addExpect(e89);
   addExpect(e90);
   addExpect(e91);
   addExpect(e92);
   addExpect(e93);
   addExpect(e94);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             TestEndPoint::ExpectBase* e89,
                             TestEndPoint::ExpectBase* e90,
                             TestEndPoint::ExpectBase* e91,
                             TestEndPoint::ExpectBase* e92,
                             TestEndPoint::ExpectBase* e93,
                             TestEndPoint::ExpectBase* e94,
                             TestEndPoint::ExpectBase* e95,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
   addExpect(e89);
   addExpect(e90);
   addExpect(e91);
   addExpect(e92);
   addExpect(e93);
   addExpect(e94);
   addExpect(e95);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             TestEndPoint::ExpectBase* e89,
                             TestEndPoint::ExpectBase* e90,
                             TestEndPoint::ExpectBase* e91,
                             TestEndPoint::ExpectBase* e92,
                             TestEndPoint::ExpectBase* e93,
                             TestEndPoint::ExpectBase* e94,
                             TestEndPoint::ExpectBase* e95,
                             TestEndPoint::ExpectBase* e96,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
   addExpect(e89);
   addExpect(e90);
   addExpect(e91);
   addExpect(e92);
   addExpect(e93);
   addExpect(e94);
   addExpect(e95);
   addExpect(e96);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             TestEndPoint::ExpectBase* e89,
                             TestEndPoint::ExpectBase* e90,
                             TestEndPoint::ExpectBase* e91,
                             TestEndPoint::ExpectBase* e92,
                             TestEndPoint::ExpectBase* e93,
                             TestEndPoint::ExpectBase* e94,
                             TestEndPoint::ExpectBase* e95,
                             TestEndPoint::ExpectBase* e96,
                             TestEndPoint::ExpectBase* e97,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
   addExpect(e89);
   addExpect(e90);
   addExpect(e91);
   addExpect(e92);
   addExpect(e93);
   addExpect(e94);
   addExpect(e95);
   addExpect(e96);
   addExpect(e97);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             TestEndPoint::ExpectBase* e89,
                             TestEndPoint::ExpectBase* e90,
                             TestEndPoint::ExpectBase* e91,
                             TestEndPoint::ExpectBase* e92,
                             TestEndPoint::ExpectBase* e93,
                             TestEndPoint::ExpectBase* e94,
                             TestEndPoint::ExpectBase* e95,
                             TestEndPoint::ExpectBase* e96,
                             TestEndPoint::ExpectBase* e97,
                             TestEndPoint::ExpectBase* e98,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
   addExpect(e89);
   addExpect(e90);
   addExpect(e91);
   addExpect(e92);
   addExpect(e93);
   addExpect(e94);
   addExpect(e95);
   addExpect(e96);
   addExpect(e97);
   addExpect(e98);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             TestEndPoint::ExpectBase* e89,
                             TestEndPoint::ExpectBase* e90,
                             TestEndPoint::ExpectBase* e91,
                             TestEndPoint::ExpectBase* e92,
                             TestEndPoint::ExpectBase* e93,
                             TestEndPoint::ExpectBase* e94,
                             TestEndPoint::ExpectBase* e95,
                             TestEndPoint::ExpectBase* e96,
                             TestEndPoint::ExpectBase* e97,
                             TestEndPoint::ExpectBase* e98,
                             TestEndPoint::ExpectBase* e99,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
   addExpect(e89);
   addExpect(e90);
   addExpect(e91);
   addExpect(e92);
   addExpect(e93);
   addExpect(e94);
   addExpect(e95);
   addExpect(e96);
   addExpect(e97);
   addExpect(e98);
   addExpect(e99);
}

SequenceClass::SequenceClass(int lineNumber,
                             bool execute,
                             ActionBase* action,
                             TestEndPoint::ExpectBase* e1,
                             TestEndPoint::ExpectBase* e2,
                             TestEndPoint::ExpectBase* e3,
                             TestEndPoint::ExpectBase* e4,
                             TestEndPoint::ExpectBase* e5,
                             TestEndPoint::ExpectBase* e6,
                             TestEndPoint::ExpectBase* e7,
                             TestEndPoint::ExpectBase* e8,
                             TestEndPoint::ExpectBase* e9,
                             TestEndPoint::ExpectBase* e10,
                             TestEndPoint::ExpectBase* e11,
                             TestEndPoint::ExpectBase* e12,
                             TestEndPoint::ExpectBase* e13,
                             TestEndPoint::ExpectBase* e14,
                             TestEndPoint::ExpectBase* e15,
                             TestEndPoint::ExpectBase* e16,
                             TestEndPoint::ExpectBase* e17,
                             TestEndPoint::ExpectBase* e18,
                             TestEndPoint::ExpectBase* e19,
                             TestEndPoint::ExpectBase* e20,
                             TestEndPoint::ExpectBase* e21,
                             TestEndPoint::ExpectBase* e22,
                             TestEndPoint::ExpectBase* e23,
                             TestEndPoint::ExpectBase* e24,
                             TestEndPoint::ExpectBase* e25,
                             TestEndPoint::ExpectBase* e26,
                             TestEndPoint::ExpectBase* e27,
                             TestEndPoint::ExpectBase* e28,
                             TestEndPoint::ExpectBase* e29,
                             TestEndPoint::ExpectBase* e30,
                             TestEndPoint::ExpectBase* e31,
                             TestEndPoint::ExpectBase* e32,
                             TestEndPoint::ExpectBase* e33,
                             TestEndPoint::ExpectBase* e34,
                             TestEndPoint::ExpectBase* e35,
                             TestEndPoint::ExpectBase* e36,
                             TestEndPoint::ExpectBase* e37,
                             TestEndPoint::ExpectBase* e38,
                             TestEndPoint::ExpectBase* e39,
                             TestEndPoint::ExpectBase* e40,
                             TestEndPoint::ExpectBase* e41,
                             TestEndPoint::ExpectBase* e42,
                             TestEndPoint::ExpectBase* e43,
                             TestEndPoint::ExpectBase* e44,
                             TestEndPoint::ExpectBase* e45,
                             TestEndPoint::ExpectBase* e46,
                             TestEndPoint::ExpectBase* e47,
                             TestEndPoint::ExpectBase* e48,
                             TestEndPoint::ExpectBase* e49,
                             TestEndPoint::ExpectBase* e50,
                             TestEndPoint::ExpectBase* e51,
                             TestEndPoint::ExpectBase* e52,
                             TestEndPoint::ExpectBase* e53,
                             TestEndPoint::ExpectBase* e54,
                             TestEndPoint::ExpectBase* e55,
                             TestEndPoint::ExpectBase* e56,
                             TestEndPoint::ExpectBase* e57,
                             TestEndPoint::ExpectBase* e58,
                             TestEndPoint::ExpectBase* e59,
                             TestEndPoint::ExpectBase* e60,
                             TestEndPoint::ExpectBase* e61,
                             TestEndPoint::ExpectBase* e62,
                             TestEndPoint::ExpectBase* e63,
                             TestEndPoint::ExpectBase* e64,
                             TestEndPoint::ExpectBase* e65,
                             TestEndPoint::ExpectBase* e66,
                             TestEndPoint::ExpectBase* e67,
                             TestEndPoint::ExpectBase* e68,
                             TestEndPoint::ExpectBase* e69,
                             TestEndPoint::ExpectBase* e70,
                             TestEndPoint::ExpectBase* e71,
                             TestEndPoint::ExpectBase* e72,
                             TestEndPoint::ExpectBase* e73,
                             TestEndPoint::ExpectBase* e74,
                             TestEndPoint::ExpectBase* e75,
                             TestEndPoint::ExpectBase* e76,
                             TestEndPoint::ExpectBase* e77,
                             TestEndPoint::ExpectBase* e78,
                             TestEndPoint::ExpectBase* e79,
                             TestEndPoint::ExpectBase* e80,
                             TestEndPoint::ExpectBase* e81,
                             TestEndPoint::ExpectBase* e82,
                             TestEndPoint::ExpectBase* e83,
                             TestEndPoint::ExpectBase* e84,
                             TestEndPoint::ExpectBase* e85,
                             TestEndPoint::ExpectBase* e86,
                             TestEndPoint::ExpectBase* e87,
                             TestEndPoint::ExpectBase* e88,
                             TestEndPoint::ExpectBase* e89,
                             TestEndPoint::ExpectBase* e90,
                             TestEndPoint::ExpectBase* e91,
                             TestEndPoint::ExpectBase* e92,
                             TestEndPoint::ExpectBase* e93,
                             TestEndPoint::ExpectBase* e94,
                             TestEndPoint::ExpectBase* e95,
                             TestEndPoint::ExpectBase* e96,
                             TestEndPoint::ExpectBase* e97,
                             TestEndPoint::ExpectBase* e98,
                             TestEndPoint::ExpectBase* e99,
                             TestEndPoint::ExpectBase* e100,
                             unsigned int hangAroundTimeMs,
                             boost::shared_ptr<SequenceSet> set)
    : mContainerBottom(0),
      mAction(action),
      mExpects(),
      mHangAroundTimeMs(hangAroundTimeMs),
      mSet(set), 
      mLineNumber(lineNumber),
      mFailed(false),
      mParent(0), 
      mBranchCount(0),
      mAfterAction(0),
      mTimingOut(false),
      mTimerId(-7)
{
   if (execute)
   {
      set->mSequences.push_back(this);
   }
   addExpect(e1);
   addExpect(e2);
   addExpect(e3);
   addExpect(e4);
   addExpect(e5);
   addExpect(e6);
   addExpect(e7);
   addExpect(e8);
   addExpect(e9);
   addExpect(e10);
   addExpect(e11);
   addExpect(e12);
   addExpect(e13);
   addExpect(e14);
   addExpect(e15);
   addExpect(e16);
   addExpect(e17);
   addExpect(e18);
   addExpect(e19);
   addExpect(e20);
   addExpect(e21);
   addExpect(e22);
   addExpect(e23);
   addExpect(e24);
   addExpect(e25);
   addExpect(e26);
   addExpect(e27);
   addExpect(e28);
   addExpect(e29);
   addExpect(e30);
   addExpect(e31);
   addExpect(e32);
   addExpect(e33);
   addExpect(e34);
   addExpect(e35);
   addExpect(e36);
   addExpect(e37);
   addExpect(e38);
   addExpect(e39);
   addExpect(e40);
   addExpect(e41);
   addExpect(e42);
   addExpect(e43);
   addExpect(e44);
   addExpect(e45);
   addExpect(e46);
   addExpect(e47);
   addExpect(e48);
   addExpect(e49);
   addExpect(e50);
   addExpect(e51);
   addExpect(e52);
   addExpect(e53);
   addExpect(e54);
   addExpect(e55);
   addExpect(e56);
   addExpect(e57);
   addExpect(e58);
   addExpect(e59);
   addExpect(e60);
   addExpect(e61);
   addExpect(e62);
   addExpect(e63);
   addExpect(e64);
   addExpect(e65);
   addExpect(e66);
   addExpect(e67);
   addExpect(e68);
   addExpect(e69);
   addExpect(e70);
   addExpect(e71);
   addExpect(e72);
   addExpect(e73);
   addExpect(e74);
   addExpect(e75);
   addExpect(e76);
   addExpect(e77);
   addExpect(e78);
   addExpect(e79);
   addExpect(e80);
   addExpect(e81);
   addExpect(e82);
   addExpect(e83);
   addExpect(e84);
   addExpect(e85);
   addExpect(e86);
   addExpect(e87);
   addExpect(e88);
   addExpect(e89);
   addExpect(e90);
   addExpect(e91);
   addExpect(e92);
   addExpect(e93);
   addExpect(e94);
   addExpect(e95);
   addExpect(e96);
   addExpect(e97);
   addExpect(e98);
   addExpect(e99);
   addExpect(e100);
}


// Copyright 2005 Purplecomm, Inc
/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
