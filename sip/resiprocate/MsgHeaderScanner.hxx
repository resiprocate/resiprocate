#ifndef RESIP_MSG_HEADER_SCANNER_HXX
#define RESIP_MSG_HEADER_SCANNER_HXX
#if defined(NEW_MSG_HEADER_SCANNER)
namespace resip {

class SipMessage;

///////////////////////////////////////////////////////////////////////////////

/*
This class scans a message header for its status line (the first non-empty
line) and then any number of field name/value pairs, terminated by an empty
line.
The message header text may be divided into arbitrary chunks.
A single instance may be used to scan any number of message headers.

Its intended usage pattern is as follows:

    MsgHeaderScanner scanner;
    for (;;) {
      SipMessage *msg = ...
      scanner.prepareForMessage(msg);
      MsgHeaderScanner::ScanChunkResult scanChunkResult;
      do {
        (Input the next chunk of the message.)
        scanChunkResult = scanner.scanChunk(...);
      } while (scanChunkResult == MsgHeaderScanner::scrNextChunk);
      ...
    }//for

Note that during the input of each message header chunk this class
encapsulates the full state of the message header scan, so that input may
be performed in whatever manner desired (eg without blocking).

In this class, "multi-value" refers to associating multiple values with a
single field name by separating the individual values with commas.

Some assertions about message headers:
    Only space and tab are whitespace characters.
    A carriage return must always be followed by a line feed.
    A line feed must always be preceded by a carriage return.
    A field name always starts at the beginning of a line.
    A value may contain a line break (a carriage return / line feed pair)
        before any whitespace, but not otherwise.  (The scanner allows
        whitespace in a few extra places for simplicity.)
    A ',' within '"' pair or '<'/'>' pair does not separate multi-values.
    A '\\' is used only within a '"' pair, and not to escape a carriage
        return or line feed.
    '<'/'>' pairs do not nest, nor contain '"' pairs.
   '('/')' pair comments cannot be combined with any multi-values.
    A multi-value cannot be empty, except as needed to specify 0 multi-values.
*/
class MsgHeaderScanner {

 public:

  enum TextPropBitMaskEnum {
    tpbmContainsLineBreak  = 1 << 0,     // '\r' or '\n', always paired
    tpbmContainsWhiteSpace = 1 << 1,     // ' ' or '\t'
    tpbmContainsBackSlash  = 1 << 2,     // '\\'
    tpbmContainsPercent    = 1 << 3,     // '%'
    tpbmContainsSemiColon  = 1 << 4,     // ';'
    tpbmContainsParen      = 1 << 5      // '(' or ')', possibly mismatched
  };
  typedef unsigned char TextPropBitMask;

 private:

  // Fields:
  SipMessage *                       _msg;
  /*State*/int                       _state;      // Type defined in .cxx file.
  int                                _prevScanChunkNumSavedTextChars;
  MsgHeaderScanner::TextPropBitMask  _textPropBitMask;
  const char *                       _fieldName;
  unsigned int                       _fieldNameLength;
  int                                _fieldKind;
  /*
  "_state" and "_prevScanChunkNumSavedTextChars" are meaningful only between
  input chunks.
  "_textPropBitMask" is meaningful only between input chunks and only when
  scanning a field name or value.
  "_fieldName", "_fieldNameLength", and "_fieldKind" are meaningful only
  between terminating a field name and finding the termination of its value.
  */

 public:

  inline
  //constructor
  MsgHeaderScanner()
  {
  }

  // Destructor: defined implicitly

  void
  prepareForMessage(SipMessage *  msg);

  enum ScanChunkResult {
    scrEnd,       // Message header scan ended.
    scrNextChunk, // Another chunk is needed.
    scrError      // The message header is in error.
  };

  /*
  The meaning of "*unprocessedCharPtr" depends on the result:
    scrEnd:       The character that terminates the message header.
    scrError:     The erroneous character.
    scrNextChunk: The first character of some incomplete text unit.  The
                        remaining portion of the old chunk must be placed
                        in the beginning of the new chunk.
  This method writes a sentinel in the chunk's terminal character.
  */
  MsgHeaderScanner::ScanChunkResult
  scanChunk(char *        chunk,
            unsigned int  chunkLength,
            char *        *unprocessedCharPtr); //out

  // Automatically called when this file is #included.
  static
  bool
  init();

 private:

  // Copy constructor: declared but not defined
  MsgHeaderScanner(const MsgHeaderScanner & from);

  // Assignment: declared but not defined
  MsgHeaderScanner &
  operator=(const MsgHeaderScanner & from);

};

static bool invoke_MsgHeaderScanner_init = MsgHeaderScanner::init();

///////////////////////////////////////////////////////////////////////////////

} // namespace resip

#endif // !defined(RESIP_MSG_HEADER_SCANNER_HXX)
#endif
