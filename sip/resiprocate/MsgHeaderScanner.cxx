#include <resiprocate/MsgHeaderScanner.hxx>

#include <limits.h>
#ifdef RESIP_MSG_HEADER_SCANNER_DEBUG
#include <stdio.h>
#endif
#include <resiprocate/HeaderTypes.hxx>
#include <resiprocate/SipMessage.hxx>

namespace resip {

///////////////////////////////////////////////////////////////////////////////

/*
Any character could be used as the chunk terminating sentinel, as long as
it would otherwise be character category "other".  The null character
was chosen because it is unlikely to occur naturally.
*/
enum { chunkTermSentinelChar = '\0' };

enum CharCategoryEnum {
  ccChunkTermSentinel,
  ccOther,
  ccFieldName,
  ccWhiteSpace,
  ccColon,
  ccDoubleQuotationMark,
  ccLeftAngleBracket,
  ccRightAngleBracket,
  ccBackSlash,
  ccComma,
  ccCarriageReturn,
  ccLineFeed,
  numCharCategories
};
typedef char CharCategory;

struct CharInfo {
  CharCategory                       category;
  MsgHeaderScanner::TextPropBitMask  textPropBitMask;
};

static CharInfo charInfoArray[UCHAR_MAX] = { {ccOther, 0} };

static
void
initCharInfoArray()
{
  // Init categories.
  for(const char *charPtr = "abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.!%*_+`'~";
      *charPtr;
      ++charPtr) {
    charInfoArray[*charPtr].category = ccFieldName;
  }//for
  charInfoArray[' '].category  = ccWhiteSpace;
  charInfoArray['\t'].category = ccWhiteSpace;
  charInfoArray[':'].category  = ccColon;
  charInfoArray['"'].category  = ccDoubleQuotationMark;
  charInfoArray['<'].category  = ccLeftAngleBracket;
  charInfoArray['>'].category  = ccRightAngleBracket;
  charInfoArray['\\'].category  = ccBackSlash;
  charInfoArray[','].category  = ccComma;
  charInfoArray['\r'].category = ccCarriageReturn;
  charInfoArray['\n'].category = ccLineFeed;
  // Assert: "chunkTermSentinelChar"'s category is still the default "ccOther".
  charInfoArray[chunkTermSentinelChar].category = ccChunkTermSentinel;
  // Init text property bit masks.
  charInfoArray['\r'].textPropBitMask =
      MsgHeaderScanner::tpbmContainsLineBreak;
  charInfoArray['\n'].textPropBitMask =
      MsgHeaderScanner::tpbmContainsLineBreak;
  charInfoArray[' '].textPropBitMask =
      MsgHeaderScanner::tpbmContainsWhiteSpace;
  charInfoArray['\t'].textPropBitMask =
      MsgHeaderScanner::tpbmContainsWhiteSpace;
  charInfoArray['\\'].textPropBitMask =
      MsgHeaderScanner::tpbmContainsBackSlash;
  charInfoArray['%'].textPropBitMask = MsgHeaderScanner::tpbmContainsPercent;
  charInfoArray[';'].textPropBitMask = MsgHeaderScanner::tpbmContainsSemiColon;
  charInfoArray['('].textPropBitMask = MsgHeaderScanner::tpbmContainsParen;
  charInfoArray[')'].textPropBitMask = MsgHeaderScanner::tpbmContainsParen;
}

///////////////////////////////////////////////////////////////////////////////

/*
States marked '1' scan normal values.  States marked 'N' scan multi-values.
*/
enum StateEnum {
  sMsgStart,
  sHalfLineBreakAtMsgStart,
  sScanStatusLine,
  sHalfLineBreakAfterStatusLine,
  sAfterLineBreakAfterStatusLine,
  sScanFieldName,
  sScanWhiteSpaceAfter1FieldName,
  sScanWhiteSpaceAfterNFieldName,
  sScanWhiteSpaceOr1Value,
  sScanWhiteSpaceOrNValue,
  sHalfLineBreakInWhiteSpaceBefore1Value,
  sHalfLineBreakInWhiteSpaceBeforeNValue,
  sAfterLineBreakInWhiteSpaceBefore1Value,
  sAfterLineBreakInWhiteSpaceBeforeNValue,
  sScan1Value,
  sScanNValue,
  sHalfLineBreakIn1Value,
  sHalfLineBreakInNValue,
  sAfterLineBreakIn1Value,
  sAfterLineBreakInNValue,
  sScanNValueInQuotes,
  sAfterEscCharInQuotesInNValue,
  sHalfLineBreakInQuotesInNValue,
  sAfterLineBreakInQuotesInNValue,
  sScanNValueInAngles,
  sHalfLineBreakInAnglesInNValue,
  sAfterLineBreakInAnglesInNValue,
  sHalfLineBreakAfterLineBreak,
  numStates
};
typedef char State;

// For each '1' state, the 'N' state is "deltaOfNStateFrom1State" larger.
enum { deltaOfNStateFrom1State = 1 };

///////////////////////////////////////////////////////////////////////////////

enum TransitionActionEnum {
  taNone,
  taTermStatusLine,         // The current character terminates the status
                            //     line.
  taTermFieldName,          // The current character terminates a field name.
                            //     If the field supports multi-values, shift
                            //     the state machine into multi-value scanning.
  taBeyondEmptyValue,       // The current character terminates an empty value.
                            //     Implies taStartText.
  taTermValueAfterLineBreak,// The previous two characters are a linebreak
                            //      terminating a value.  Implies taStartText.
  taTermValue,              // The current character terminates a value.
  taStartText,              // The current character starts a text unit.
                            //     (The status line, a field name, or a value.)
  taEndHeader,              // The current character terminates the header.
  taChunkTermSentinel,      // Either the current character terminates the
                            //    current chunk or it is an ordinary character.
  taError                   // The input is erroneous.
};
typedef char TransitionAction;

///////////////////////////////////////////////////////////////////////////////

struct TransitionInfo {
  TransitionAction  action;
  State             nextState;
};

static TransitionInfo stateMachine[numStates][numCharCategories];

inline
void
specTransition(State             state,
               CharCategory      charCategory,
               TransitionAction  action,
               State             nextState)
{
    stateMachine[state][charCategory].action = action;
    stateMachine[state][charCategory].nextState = nextState;
}

static
void
specDefaultTransition(State             state,
                      TransitionAction  action,
                      State             nextState)
{
  for (int charCategory = 0;
           charCategory < numCharCategories;
               ++charCategory) {
    specTransition(state, charCategory, action, nextState);
  }//for
  specTransition(state, ccCarriageReturn, taError, state);
  specTransition(state, ccLineFeed, taError, state);
  specTransition(state, ccChunkTermSentinel, taChunkTermSentinel, state);
}

static
void
specHalfLineBreakState(State  halfLineBreakState,
                       State  afterLineBreakState)
{
  specDefaultTransition(halfLineBreakState, taError, halfLineBreakState);
  specTransition(halfLineBreakState, ccLineFeed, taNone, afterLineBreakState);
}

/*
Single-value (1) scanning and multi-value (N) scanning involves several nearly
identical states.
"stateDelta" is either 0 or "deltaOfNStateFrom1State".
*/
static
void
specXValueStates(int  stateDelta)
{
  specDefaultTransition(sScanWhiteSpaceAfter1FieldName + stateDelta,
                        taError,
                        sScanWhiteSpaceAfter1FieldName + stateDelta);
  specTransition(sScanWhiteSpaceAfter1FieldName + stateDelta,
                 ccWhiteSpace,
                 taNone,
                 sScanWhiteSpaceAfter1FieldName + stateDelta);
  specTransition(sScanWhiteSpaceAfter1FieldName + stateDelta,
                 ccColon,
                 taNone,
                 sScanWhiteSpaceOr1Value + stateDelta);
  specDefaultTransition(sScanWhiteSpaceOr1Value + stateDelta,
                        taStartText,
                        sScan1Value + stateDelta);
  specTransition(sScanWhiteSpaceOr1Value + stateDelta,
                 ccWhiteSpace,
                 taNone,
                 sScanWhiteSpaceOr1Value + stateDelta);
  if (stateDelta == deltaOfNStateFrom1State) {
    specTransition(sScanWhiteSpaceOr1Value + stateDelta,
                   ccComma,
                   taError,
                   sScanWhiteSpaceOr1Value + stateDelta);
    specTransition(sScanWhiteSpaceOr1Value + stateDelta,
                   ccLeftAngleBracket,
                   taStartText,
                   sScanNValueInAngles);
    specTransition(sScanWhiteSpaceOr1Value + stateDelta,
                   ccDoubleQuotationMark,
                   taStartText,
                   sScanNValueInQuotes);
  }
  specTransition(sScanWhiteSpaceOr1Value + stateDelta,
                 ccCarriageReturn,
                 taNone,
                 sHalfLineBreakInWhiteSpaceBefore1Value + stateDelta);
  specHalfLineBreakState(sHalfLineBreakInWhiteSpaceBefore1Value + stateDelta,
                         sAfterLineBreakInWhiteSpaceBefore1Value + stateDelta);
  specDefaultTransition(sAfterLineBreakInWhiteSpaceBefore1Value + stateDelta,
                        taError,
                        sAfterLineBreakInWhiteSpaceBefore1Value + stateDelta);
  specTransition(sAfterLineBreakInWhiteSpaceBefore1Value + stateDelta,
                 ccFieldName,
                 taBeyondEmptyValue,
                 sScanFieldName);
  specTransition(sAfterLineBreakInWhiteSpaceBefore1Value + stateDelta,
                 ccWhiteSpace,
                 taNone,
                 sScanWhiteSpaceOr1Value + stateDelta);
  specTransition(sAfterLineBreakInWhiteSpaceBefore1Value + stateDelta,
                 ccCarriageReturn,
                 taBeyondEmptyValue,
                 sHalfLineBreakAfterLineBreak);
  specDefaultTransition(sScan1Value + stateDelta,
                        taNone,
                        sScan1Value + stateDelta);
  if (stateDelta == deltaOfNStateFrom1State) {
    specTransition(sScan1Value + stateDelta,
                   ccComma,
                   taTermValue,
                   sScanWhiteSpaceOr1Value + stateDelta);
    specTransition(sScan1Value + stateDelta,
                   ccLeftAngleBracket,
                   taNone,
                   sScanNValueInAngles);
    specTransition(sScan1Value + stateDelta,
                   ccDoubleQuotationMark,
                   taNone,
                   sScanNValueInQuotes);
  }
  specTransition(sScan1Value + stateDelta,
                 ccCarriageReturn,
                 taNone,
                 sHalfLineBreakIn1Value + stateDelta);
  specHalfLineBreakState(sHalfLineBreakIn1Value + stateDelta,
                         sAfterLineBreakIn1Value + stateDelta);
  specDefaultTransition(sAfterLineBreakIn1Value + stateDelta,
                        taError,
                        sAfterLineBreakIn1Value + stateDelta);
  specTransition(sAfterLineBreakIn1Value + stateDelta,
                 ccFieldName,
                 taTermValueAfterLineBreak,
                 sScanFieldName);
  specTransition(sAfterLineBreakIn1Value + stateDelta,
                 ccWhiteSpace,
                 taNone,
                 sScan1Value + stateDelta);
  specTransition(sAfterLineBreakIn1Value + stateDelta,
                 ccCarriageReturn,
                 taTermValueAfterLineBreak,
                 sHalfLineBreakAfterLineBreak);
}

static
void
initStateMachine()
{
  // By convention, error transitions maintain the same state.
  specDefaultTransition(sMsgStart, taStartText, sScanStatusLine);
  specTransition(sMsgStart,
                 ccCarriageReturn,
                 taNone,
                 sHalfLineBreakAtMsgStart);
  specHalfLineBreakState(sHalfLineBreakAtMsgStart, sMsgStart);
  specDefaultTransition(sScanStatusLine, taNone, sScanStatusLine);
  specTransition(sScanStatusLine,
                 ccCarriageReturn,
                 taTermStatusLine,
                 sHalfLineBreakAfterStatusLine);
  specHalfLineBreakState(sHalfLineBreakAfterStatusLine,
                         sAfterLineBreakAfterStatusLine);
  specDefaultTransition(sAfterLineBreakAfterStatusLine,
                        taError,
                        sAfterLineBreakAfterStatusLine);
  specTransition(sAfterLineBreakAfterStatusLine,
                 ccFieldName,
                 taStartText,
                 sScanFieldName);
  specTransition(sAfterLineBreakAfterStatusLine,
                 ccWhiteSpace,
                 taError,
                 sAfterLineBreakAfterStatusLine);
  specTransition(sAfterLineBreakAfterStatusLine,
                 ccCarriageReturn,
                 taNone,
                 sHalfLineBreakAfterLineBreak);
  specDefaultTransition(sScanFieldName, taError, sScanFieldName);
  specTransition(sScanFieldName, ccFieldName, taNone, sScanFieldName);
  specTransition(sScanFieldName,
                 ccWhiteSpace,
                 taTermFieldName,
                 sScanWhiteSpaceAfter1FieldName);
  specTransition(sScanFieldName,
                 ccColon,
                 taTermFieldName,
                 sScanWhiteSpaceOr1Value);
  specXValueStates(0);
  specXValueStates(deltaOfNStateFrom1State);
  specDefaultTransition(sScanNValueInQuotes, taNone, sScanNValueInQuotes);
  specTransition(sScanNValueInQuotes,
                 ccDoubleQuotationMark,
                 taNone,
                 sScanNValue);
  specTransition(sScanNValueInQuotes,
                 ccBackSlash,
                 taNone,
                 sAfterEscCharInQuotesInNValue);
  specTransition(sScanNValueInQuotes,
                 ccCarriageReturn,
                 taNone,
                 sHalfLineBreakInQuotesInNValue);
  specDefaultTransition(sAfterEscCharInQuotesInNValue,
                        taNone,
                        sScanNValueInQuotes);
  specHalfLineBreakState(sHalfLineBreakInQuotesInNValue,
                         sAfterLineBreakInQuotesInNValue);
  specDefaultTransition(sAfterLineBreakInQuotesInNValue,
                        taError,
                        sAfterLineBreakInQuotesInNValue);
  specTransition(sAfterLineBreakInQuotesInNValue,
                 ccWhiteSpace,
                 taNone,
                 sScanNValueInQuotes);
  specDefaultTransition(sScanNValueInAngles, taNone, sScanNValueInAngles);
  specTransition(sScanNValueInAngles,
                 ccRightAngleBracket,
                 taNone,
                 sScanNValue);
  specTransition(sScanNValueInAngles,
                 ccCarriageReturn,
                 taNone,
                 sHalfLineBreakInAnglesInNValue);
  specHalfLineBreakState(sHalfLineBreakInAnglesInNValue,
                         sAfterLineBreakInAnglesInNValue);
  specDefaultTransition(sAfterLineBreakInAnglesInNValue,
                        taError,
                        sAfterLineBreakInAnglesInNValue);
  specTransition(sAfterLineBreakInAnglesInNValue,
                 ccWhiteSpace,
                 taNone,
                 sScanNValueInAngles);
  specHalfLineBreakState(sHalfLineBreakAfterLineBreak, sMsgStart);
  // Most half-line-break states do nothing when they read a line feed,
  // but sHalfLineBreakAfterLineBreak must end the message header scanning.
  specTransition(sHalfLineBreakAfterLineBreak,
                 ccLineFeed,
                 taEndHeader,
                 sMsgStart); // Arbitrary but possibly handy.
}

///////////////////////////////////////////////////////////////////////////////

#ifdef RESIP_MSG_HEADER_SCANNER_DEBUG // {

static
void
printText(const char *  text,
          unsigned int  textLength)
{
  const char *charPtr = text;
  for (int counter = 0; counter < textLength; ++charPtr, ++counter) {
    char c = *charPtr;
    if (c == '\r') {
      printf("\\r");
    } else if (c == '\n') {
      printf("\\n");
    } else if (c == '\t') {
      printf("\\t");
    } else if (c == '\0') {
      printf("\\0");
    } else {
      putchar(c);
    }
  }//for
}

static
void
printStateTransition(State             state,
                     char              character,
                     TransitionAction  transitionAction)
{
  const char *stateName;
  switch (state) {
    case sMsgStart:
      stateName = "sMsgStart";
      break;
    case sHalfLineBreakAtMsgStart:
      stateName = "sHalfLineBreakAtMsgStart";
      break;
    case sScanStatusLine:
      stateName = "sScanStatusLine";
      break;
    case sHalfLineBreakAfterStatusLine:
      stateName = "sHalfLineBreakAfterStatusLine";
      break;
    case sAfterLineBreakAfterStatusLine:
      stateName = "sAfterLineBreakAfterStatusLine";
      break;
    case sScanFieldName:
      stateName = "sScanFieldName";
      break;
    case sScanWhiteSpaceAfter1FieldName:
      stateName = "sScanWhiteSpaceAfter1FieldName";
      break;
    case sScanWhiteSpaceAfterNFieldName:
      stateName = "sScanWhiteSpaceAfterNFieldName";
      break;
    case sScanWhiteSpaceOr1Value:
      stateName = "sScanWhiteSpaceOr1Value";
      break;
    case sScanWhiteSpaceOrNValue:
      stateName = "sScanWhiteSpaceOrNValue";
      break;
    case sHalfLineBreakInWhiteSpaceBefore1Value:
      stateName = "sHalfLineBreakInWhiteSpaceBefore1Value";
      break;
    case sHalfLineBreakInWhiteSpaceBeforeNValue:
      stateName = "sHalfLineBreakInWhiteSpaceBeforeNValue";
      break;
    case sAfterLineBreakInWhiteSpaceBefore1Value:
      stateName = "sAfterLineBreakInWhiteSpaceBefore1Value";
      break;
    case sAfterLineBreakInWhiteSpaceBeforeNValue:
      stateName = "sAfterLineBreakInWhiteSpaceBeforeNValue";
      break;
    case sScan1Value:
      stateName = "sScan1Value";
      break;
    case sScanNValue:
      stateName = "sScanNValue";
      break;
    case sHalfLineBreakIn1Value:
      stateName = "sHalfLineBreakIn1Value";
      break;
    case sHalfLineBreakInNValue:
      stateName = "sHalfLineBreakInNValue";
      break;
    case sAfterLineBreakIn1Value:
      stateName = "sAfterLineBreakIn1Value";
      break;
    case sAfterLineBreakInNValue:
      stateName = "sAfterLineBreakInNValue";
      break;
    case sScanNValueInQuotes:
      stateName = "sScanNValueInQuotes";
      break;
    case sAfterEscCharInQuotesInNValue:
      stateName = "sAfterEscCharInQuotesInNValue";
      break;
    case sHalfLineBreakInQuotesInNValue:
      stateName = "sHalfLineBreakInQuotesInNValue";
      break;
    case sAfterLineBreakInQuotesInNValue:
      stateName = "sAfterLineBreakInQuotesInNValue";
      break;
    case sScanNValueInAngles:
      stateName = "sScanNValueInAngles";
      break;
    case sHalfLineBreakInAnglesInNValue:
      stateName = "sHalfLineBreakInAnglesInNValue";
      break;
    case sAfterLineBreakInAnglesInNValue:
      stateName = "sAfterLineBreakInAnglesInNValue";
      break;
    case sHalfLineBreakAfterLineBreak:
      stateName = "sHalfLineBreakAfterLineBreak";
      break;
    default:
      stateName = "<unknown>";
  }//switch
  const char *transitionActionName;
  switch (transitionAction) {
    case taNone:
      transitionActionName = "taNone";
      break;
    case taTermStatusLine:
      transitionActionName = "taTermStatusLine";
      break;
    case taTermFieldName:
      transitionActionName = "taTermFieldName";
      break;
    case taBeyondEmptyValue:
      transitionActionName = "taBeyondEmptyValue";
      break;
    case taTermValueAfterLineBreak:
      transitionActionName = "taTermValueAfterLineBreak";
      break;
    case taTermValue:
      transitionActionName = "taTermValue";
      break;
    case taStartText:
      transitionActionName = "taStartText";
      break;
    case taEndHeader:
      transitionActionName = "taEndHeader";
      break;
    case taChunkTermSentinel:
      transitionActionName = "taChunkTermSentinel";
      break;
    case taError:
      transitionActionName = "taError";
      break;
    default:
      transitionActionName = "<unknown>";
  }//switch
  printf("                %s['", stateName);
  printText(&character, 1);
  printf("']: %s\n", transitionActionName);
}

#endif //defined(RESIP_MSG_HEADER_SCANNER_DEBUG) // }

///////////////////////////////////////////////////////////////////////////////

#ifdef RESIP_MSG_HEADER_SCANNER_DEBUG // {

static const char *const multiValuedFieldNameArray[] = {
                                                         "allow-events",
                                                         "accept-encoding",
                                                         "accept-language",
                                                         "allow",
                                                         "content-language",
                                                         "proxy-require",
                                                         "require",
                                                         "supported",
                                                         "subscription-state",
                                                         "unsupported",
                                                         "security-client",
                                                         "security-server",
                                                         "security-verify",
                                                         "accept",
                                                         "call-info",
                                                         "alert-info",
                                                         "error-info",
                                                         "record-route",
                                                         "route",
                                                         "contact",
                                                         "authorization",
                                                         "proxy-authenticate",
                                                         "proxy-authorization",
                                                         "www-authenticate",
                                                         "via",
                                                         0
                                                       };

extern
void
lookupMsgHeaderFieldInfo(
            char *                             fieldName,               //inout
            unsigned int                       *fieldNameLength,        //inout
            MsgHeaderScanner::TextPropBitMask  fieldNameTextPropBitmask,
            int                                *fieldKind,              //out
            bool                               *isMultiValueAllowed)    //out
{
  *isMultiValueAllowed = false;
  const char *const *multiValuedFieldNamePtr = multiValuedFieldNameArray;
  for (;;) {
    const char *multiValuedFieldName = *multiValuedFieldNamePtr;
    if (!multiValuedFieldName) {
      break;
    }
    if (strncmp(fieldName, multiValuedFieldName, *fieldNameLength) == 0) {
      *isMultiValueAllowed = true;
      break;
    }
    ++multiValuedFieldNamePtr;
  }//for
}

static
bool
processMsgHeaderStatusLine(
                       SipMessage *                       msg,
                       char *                             lineText,
                       unsigned int                       lineTextLength,
                       MsgHeaderScanner::TextPropBitMask  lineTextPropBitMask)
{
  printf("status line: ");
  printText(lineText, lineTextLength);
  printf("\n");
  return true;
}

static
void
processMsgHeaderFieldNameAndValue(
                      SipMessage *                       msg,
                      int                                fieldKind,
                      const char *                       fieldName,
                      unsigned int                       fieldNameLength,
                      char *                             valueText,
                      unsigned int                       valueTextLength,
                      MsgHeaderScanner::TextPropBitMask  valueTextPropBitmask)
{
  printText(fieldName, fieldNameLength);
  printf(": [[[[");
  printText(valueText, valueTextLength);
  printf("]]]]\n");
}

#else //!defined(RESIP_MSG_HEADER_SCANNER_DEBUG) } {

/*
Determine a field's kind and whether it allows (comma separated) multi-values.
"fieldName" is not empty and contains only legal characters.
The text in "fieldName" may be canonicalized (eg, translating % escapes),
including shrinking it if necessary.
*/
inline
void
lookupMsgHeaderFieldInfo(
            char *                             fieldName,               //inout
            unsigned int                       *fieldNameLength,        //inout
            MsgHeaderScanner::TextPropBitMask  fieldNameTextPropBitmask,
            int                                *fieldKind,              //out
            bool                               *isMultiValueAllowed)    //out
{
  //.jacob. Don't ignore fieldNameTextPropBitmask.
  *fieldKind = Headers::getType(fieldName, *fieldNameLength);
  *isMultiValueAllowed =
      Headers::isCommaTokenizing(static_cast<Headers::Type>(*fieldKind));
}

/*
"lineText" contains no carriage returns and no line feeds.
Return true on success, false on failure.
*/
inline
bool
processMsgHeaderStatusLine(
                       SipMessage *                       msg,
                       char *                             lineText,
                       unsigned int                       lineTextLength,
                       MsgHeaderScanner::TextPropBitMask  lineTextPropBitMask)
{
  //.jacob. Don't ignore valueTextPropBitmask.
  msg->setStartLine(lineText, lineTextLength);
  return true;
}

/*
This function is called once for a field with one value.  (The value could be
    several values, but separated by something other than commas.)
This function is called once for a field with 0 comma-separated values, with
    an empty value.
This function is called N times for a field with N comma-separated values,
    but with the same value of "fieldName" each time.
"fieldName" is not empty and contains only legal characters.
"valueText" may be empty, has no leading whitespace, may contain trailing
    whitespace, contains carriage returns and line feeds only in correct pairs
    and followed by whitespace, and, if the field is multi-valued, contains
    balanced '<'/'>' and '"' pairs, contains ',' only within '<'/'>' or '"'
    pairs, and respects '\\'s within '"' pairs.
The text in "valueText" may be canonicalized (eg, translating % escapes),
    including shrinking it if necessary.
*/
inline
void
processMsgHeaderFieldNameAndValue(
                      SipMessage *                       msg,
                      int                                fieldKind,
                      const char *                       fieldName,
                      unsigned int                       fieldNameLength,
                      char *                             valueText,
                      unsigned int                       valueTextLength,
                      MsgHeaderScanner::TextPropBitMask  valueTextPropBitmask)
{
  //.jacob. Don't ignore valueTextPropBitmask.
  msg->addHeader(static_cast<Headers::Type>(fieldKind),
                 fieldName,
                 fieldNameLength,
                 valueText,
                 valueTextLength);
}

#endif //!defined(RESIP_MSG_HEADER_SCANNER_DEBUG) }

///////////////////////////////////////////////////////////////////////////////

void
MsgHeaderScanner::prepareForMessage(SipMessage *  msg)
{
  _msg = msg;
  _state = sMsgStart;
  _prevScanChunkNumSavedTextChars = 0;
}

MsgHeaderScanner::ScanChunkResult
MsgHeaderScanner::scanChunk(char *        chunk,
                            unsigned int  chunkLength,
                            char *        *unprocessedCharPtr)
{
  MsgHeaderScanner::ScanChunkResult result;
  CharInfo* localCharInfoArray = charInfoArray;
  TransitionInfo (*localStateMachine)[numCharCategories] = stateMachine;
  State localState = _state;
  char *charPtr = chunk + _prevScanChunkNumSavedTextChars;
  char *termCharPtr = chunk + chunkLength;
  char saveChunkTermChar = *termCharPtr;
  *termCharPtr = chunkTermSentinelChar;
  char *textStartCharPtr;
  MsgHeaderScanner::TextPropBitMask localTextPropBitMask = _textPropBitMask;
  if (_prevScanChunkNumSavedTextChars == 0) {
    textStartCharPtr = 0;
  } else {
    textStartCharPtr = chunk;
  }
  --charPtr;  // The loop starts by advancing "charPtr", so pre-adjust it.
  for (;;) {
    // BEGIN message header character scan block BEGIN
    // The code in this block is executed once per message header character.
    // This entire file is designed specifically to minimize this block's size.
    ++charPtr;
    CharInfo *charInfo = &localCharInfoArray[((unsigned char) (*charPtr))];
    CharCategory charCategory = charInfo->category;
    localTextPropBitMask |= charInfo->textPropBitMask;
determineTransitionFromCharCategory:
    TransitionInfo *transitionInfo =
        &(localStateMachine[localState][charCategory]);
    TransitionAction transitionAction = transitionInfo->action;
#ifdef RESIP_MSG_HEADER_SCANNER_DEBUG
    printStateTransition(localState, *charPtr, transitionAction);
#endif
    localState = transitionInfo->nextState;
    if (transitionAction == taNone) continue;
    // END message header character scan block END
    // The loop remainder is executed about 4-5 times per message header line.
    switch (transitionAction) {
      case taTermStatusLine:
        if (!processMsgHeaderStatusLine(_msg,
                                        textStartCharPtr,
                                        charPtr - textStartCharPtr,
                                        localTextPropBitMask)) {
          result = MsgHeaderScanner::scrError;
          *unprocessedCharPtr = charPtr;
          goto endOfFunction;
        }
        textStartCharPtr = 0;
        break;
      case taTermFieldName:
        {
          _fieldNameLength = charPtr - textStartCharPtr;
          bool isMultiValueAllowed;
          lookupMsgHeaderFieldInfo(textStartCharPtr,
                                   &_fieldNameLength,
                                   localTextPropBitMask,
                                   &_fieldKind,
                                   &isMultiValueAllowed);
          _fieldName = textStartCharPtr;
          textStartCharPtr = 0;
          if (isMultiValueAllowed) {
            localState += deltaOfNStateFrom1State;
          }
        }
        break;
      case taBeyondEmptyValue:
        processMsgHeaderFieldNameAndValue(_msg,
                                          _fieldKind,
                                          _fieldName,
                                          _fieldNameLength,
                                          0,
                                          0,
                                          0);
        goto performStartTextAction;
      case taTermValueAfterLineBreak:
        processMsgHeaderFieldNameAndValue(_msg,
                                          _fieldKind,
                                          _fieldName,
                                          _fieldNameLength,
                                          textStartCharPtr,
                                          (charPtr - textStartCharPtr) - 2,
                                          localTextPropBitMask);       //^:CRLF
        goto performStartTextAction;
      case taTermValue:
        processMsgHeaderFieldNameAndValue(_msg,
                                          _fieldKind,
                                          _fieldName,
                                          _fieldNameLength,
                                          textStartCharPtr,
                                          charPtr - textStartCharPtr,
                                          localTextPropBitMask);
        textStartCharPtr = 0;
        break;
      case taStartText:
performStartTextAction:
        textStartCharPtr = charPtr;
        localTextPropBitMask = 0;
        break;
      case taEndHeader:
        // textStartCharPtr is not 0.  Not currently relevant.
        result = MsgHeaderScanner::scrEnd;
        *unprocessedCharPtr = charPtr + 1;  // The current char is processed.
        goto endOfFunction;
        break;
      case taChunkTermSentinel:
        if (charPtr == termCharPtr) {
          // The chunk has been consumed.  Save some state and request another.
          _state = localState;
          if (textStartCharPtr == 0) {
            _prevScanChunkNumSavedTextChars = 0;
          } else {
            _prevScanChunkNumSavedTextChars = termCharPtr - textStartCharPtr;
          }
          _textPropBitMask = localTextPropBitMask;
          result = MsgHeaderScanner::scrNextChunk;
          *unprocessedCharPtr = termCharPtr - _prevScanChunkNumSavedTextChars;
          goto endOfFunction;
        } else {
          // The character is not the sentinel.  Treat it like any other.
          charCategory = ccOther;
          goto determineTransitionFromCharCategory;
        }
        break;
      default:
        result = MsgHeaderScanner::scrError;
        *unprocessedCharPtr = charPtr;
        goto endOfFunction;
    }//switch
  }//for
endOfFunction:
  *termCharPtr = saveChunkTermChar;
  return result;
}

///////////////////////////////////////////////////////////////////////////////

//static
bool
MsgHeaderScanner::init()
{
  static bool alreadyInvoked = false;
  if (!alreadyInvoked) {
    alreadyInvoked = true;
    initCharInfoArray();
    initStateMachine();
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace resip

///////////////////////////////////////////////////////////////////////////////

#ifdef RESIP_MSG_HEADER_SCANNER_DEBUG // {

extern
int
main(unsigned int   numArgs,
     const char * * argVector)
{
  ::resip::MsgHeaderScanner scanner;
  scanner.prepareForMessage(0);
  char *text =
      "status\r\n"
      "bobby: dummy\r\n"
      "allow: foo, bar, \"don,\\\"xyz\r\n zy\", buzz\r\n\r\n";
  unsigned int textLength = strlen(text);
  char chunk[10000];
  strcpy(chunk, text);
  ::resip::MsgHeaderScanner::ScanChunkResult scanChunkResult;
  char *unprocessedCharPtr;
  scanChunkResult = scanner.scanChunk(chunk, 21, &unprocessedCharPtr);
  if (scanChunkResult == ::resip::MsgHeaderScanner::scrNextChunk) {
    printf("Scanning another chunk '.");
    ::resip::printText(unprocessedCharPtr, 1);
    printf("'\n");
    scanChunkResult =
        scanner.scanChunk(unprocessedCharPtr,
                          (chunk + textLength) - unprocessedCharPtr,
                          &unprocessedCharPtr);
  }
  if (scanChunkResult != ::resip::MsgHeaderScanner::scrEnd) {
    printf("Error %d at character %d.\n",
           scanChunkResult,
           unprocessedCharPtr - chunk);
  }
  return 0;
}

#endif //!defined(RESIP_MSG_HEADER_SCANNER_DEBUG) }
