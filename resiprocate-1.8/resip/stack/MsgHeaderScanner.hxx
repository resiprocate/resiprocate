#if !defined(RESIP_MSG_HEADER_SCANNER_HXX)
#define RESIP_MSG_HEADER_SCANNER_HXX
namespace resip 
{

class SipMessage;

///////////////////////////////////////////////////////////////////////////////
// This class scans a message header for its status line (the first non-empty
// line) and then any number of field name/value pairs, terminated by an empty
// line.
// The message header text may be divided into arbitrary chunks.
// A single instance may be used to scan any number of message headers.
//
// Its intended usage pattern is as follows:
//
//     MsgHeaderScanner scanner;
//     for (;;) {
//       SipMessage *msg = ...
//       scanner.prepareForMessage(msg);
//       MsgHeaderScanner::ScanChunkResult scanChunkResult;
//       do {
//         (Input the next chunk of the message.)
//         scanChunkResult = scanner.scanChunk(...);
//       } while (scanChunkResult == MsgHeaderScanner::scrNextChunk);
//       ...
//     }//for
//
// Note that during the input of each message header chunk this class
// encapsulates the full state of the message header scan, so that input may
// be performed in whatever manner desired (eg without blocking).
//
// In this class, "multi-value" refers to associating multiple values with a
// single field name by separating the individual values with commas.
//
// Some assertions about message headers:
//     Only space and tab are whitespace characters.
//     A carriage return must always be followed by a line feed.
//     A line feed must always be preceded by a carriage return.
//     A field name always starts at the beginning of a line.
//     A value may contain a line break (a carriage return / line feed pair)
//         before any whitespace, but not otherwise.  (The scanner allows
//         whitespace in a few extra places for simplicity.)
//     A ',' within '"' pair or '<'/'>' pair does not separate multi-values.
//     A '\\' is used only within a '"' pair, and not to escape a carriage
//         return or line feed.
//     '<'/'>' pairs do not nest, nor contain '"' pairs.
//    '('/')' pair comments cannot be combined with any multi-values.
//     A multi-value cannot be empty, except as needed to specify 0 multi-values.

class MsgHeaderScanner
{
      
   public:
      enum { MaxNumCharsChunkOverflow = 5 };
      static char* allocateBuffer(int size);
      
      enum TextPropBitMaskEnum 
      {
         tpbmContainsLineBreak  = 1 << 0,     // '\r' or '\n', always paired
         tpbmContainsWhitespace = 1 << 1,     // ' ' or '\t'
         tpbmContainsBackslash  = 1 << 2,     // '\\'
         tpbmContainsPercent    = 1 << 3,     // '%'
         tpbmContainsSemicolon  = 1 << 4,     // ';'
         tpbmContainsParen      = 1 << 5      // '(' or ')', possibly mismatched
      };
      typedef unsigned char TextPropBitMask;
    
      inline unsigned int getHeaderCount() const { return mNumHeaders;} 

   private:
    
      // Fields:
      SipMessage *                       mMsg;
      unsigned int                       mNumHeaders;
      /*State*/int                       mState;      // Type defined in .cxx file.
      int                                mPrevScanChunkNumSavedTextChars;
      MsgHeaderScanner::TextPropBitMask  mTextPropBitMask;
      const char *                       mFieldName;
      unsigned int                       mFieldNameLength;
      int                                mFieldKind;
      /*
        "mState" and "mPrevScanChunkNumSavedTextChars" are meaningful only between
        input chunks.
        "mTextPropBitMask" is meaningful only between input chunks and only when
        scanning a field name or value.
        "mFieldName", "mFieldNameLength", and "mFieldKind" are meaningful only
        between terminating a field name and finding the termination of its value.
      */
    
   public:
    
      MsgHeaderScanner();
    
      // Destructor: defined implicitly
      void prepareForMessage(SipMessage *  msg);
      // allow proper parsing of message/sipfrag & msg/external
      // presence of start line is determined in SipFrag
      void prepareForFrag(SipMessage *  msg, bool hasStartLine);
 
      
      enum ScanChunkResult {
         scrEnd,       // Message header scan ended.
         scrNextChunk, // Another chunk is needed.
         scrError      // The message header is in error.
      };
    

      //       The meaning of "*unprocessedCharPtr" depends on the result:
      //       scrEnd:       The character that terminates the message header.
      //       scrError:     The erroneous character.
      //       scrNextChunk: The first character of some incomplete text unit.  The
      //       remaining portion of the old chunk must be placed
      //       in the beginning of the new chunk.
      //       This method writes a sentinel in the chunk's terminal character.
      MsgHeaderScanner::ScanChunkResult scanChunk(char * chunk,
                                                  unsigned int chunkLength,
                                                  char **unprocessedCharPtr); 
    
      // !ah! DEBUG only, write to fd.
      // !ah! for documentation generation
      static int dumpStateMachine(int fd); 

   private:


      // Copy constructor: declared but not defined
      MsgHeaderScanner(const MsgHeaderScanner & from);

      // Assignment: declared but not defined
      MsgHeaderScanner & operator=(const MsgHeaderScanner & from);

      // Automatically called when 1st MsgHeaderScanner constructed.
      bool initialize();
      static bool mInitialized;


};


///////////////////////////////////////////////////////////////////////////////

} // namespace resip

#endif
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
