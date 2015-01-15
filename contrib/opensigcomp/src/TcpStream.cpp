/* ***********************************************************************
   Open SigComp -- Implementation of RFC 3320 Signaling Compression

   Copyright 2005 Estacado Systems, LLC

   Your use of this code is governed by the license under which it
   has been provided to you. Unless you have a written and signed
   document from Estacado Systems, LLC stating otherwise, your license
   is as provided by the GNU General Public License version 2, a copy
   of which is available in this project in the file named "LICENSE."
   Alternately, a copy of the licence is available by writing to
   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02110-1307 USA

   Unless your use of this code is goverened by a written and signed
   contract containing provisions to the contrary, this program is
   distributed WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */

/**
  @file TcpStream.cpp
  @brief Implementation of osc::TcpStream class.
*/



#include "ProfileStack.h"
#include <assert.h>
#include "TcpStream.h"
#include "MultiBuffer.h"

/**
  Constructor for osc::TcpStream.
 */
osc::TcpStream::TcpStream()
  : m_headFrame(0),
    m_tailFrame(0)
{
  DEBUG_STACK_FRAME;
}

/**
  Copy constructor for osc::TcpStream.
 */
osc::TcpStream::TcpStream(TcpStream const &r)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::TcpStream.
 */
osc::TcpStream::~TcpStream()
{
  DEBUG_STACK_FRAME;
  osc::TcpStream::Frame *frame = m_headFrame;
  osc::TcpStream::Frame *tmp;

  while (frame)
  {
    tmp = frame;
    frame = frame->next;
    delete tmp->message;
    delete tmp;
  }
}

/**
  Assignment operator for osc::TcpStream.
 */
osc::TcpStream &
osc::TcpStream::operator=(TcpStream const &r)
{
  DEBUG_STACK_FRAME;
  if (&r == this)
  {
    return *this;
  }
  /* Assign attributes */
  assert(0);
  return *this;
}

/**
  Adds data that has been read from the network.

  @param data  Pointer to new data. The TcpStream DOES NOT
               take ownership of the data.

  @param size  Number of bytes in the data buffer.
 */

osc::TcpStream::status_t
osc::TcpStream::addData(const void *data, size_t size)
{
  DEBUG_STACK_FRAME;
  int skip;
  osc::MultiBuffer mb;
  status_t status;
  bool success;

  if (partialFrame.size() > 0)
  {
    success = mb.addBlock(partialFrame);
    if (!success)
    {
      return OUT_OF_MEMORY;
    }
  }

  success = mb.addBlock(reinterpret_cast<const osc::byte_t*>(data), size);
  if (!success)
  {
    return OUT_OF_MEMORY;
  }

  size_t messageStart = 0;
  size_t messageSize = 0;

  for (size_t i = 0; i < mb.getSize(); i++)
  {
    if ((mb[i] == 0xFF) && ((i+1) < mb.getSize()))
    {
      skip = mb[i+1];
      if (skip == 0xFF)
      {
        // End of frame found: create new message
        status = addMessageToFrameList(mb, messageStart, i, messageSize);
        if (status != OK)
        {
          return status;
        }
        messageStart = i+2;
        messageSize = 0;
        i++;
      }
      else if (skip >= 0x80)
      {
        return FRAMING_ERROR;
      }
      else
      {
        i += skip + 1;
        messageSize += skip + 1;
      }
    }
    else
    {
      messageSize++;
    }
  }  


  // If there's a partial buffer left, copy it off for later
  if (messageStart < mb.getSize())
  {
    Buffer temp;
    osc::byte_t *t = temp.getMutableBuffer(mb.getSize() - messageStart);
    if (!t)
    {
      return OUT_OF_MEMORY;
    }

    for (size_t i = messageStart; i < mb.getSize(); i++)
    {
      *t = mb[i];
      t++;
    }
    partialFrame.subsume(temp);
  }
  else
  {
    partialFrame.free();
  }

  return OK;
}

/**
  Returns the next whole SigComp message available from the
  TCP stream.

  @note The caller takes ownership of the Sigcomp Message
        instance returned by this call.

  @retval 0  The stream does not contain a full SigComp
             message.
 */

osc::SigcompMessage *
osc::TcpStream::getNextMessage()
{
  DEBUG_STACK_FRAME;
  osc::TcpStream::Frame *frame = m_headFrame;

  if (!frame)
  {
    return 0;
  }

  osc::SigcompMessage *message = m_headFrame->message;

  m_headFrame = frame->next;

  if (m_headFrame == 0)
  {
    m_tailFrame = 0;
  }

  delete frame;
  return message;
}

osc::TcpStream::status_t
osc::TcpStream::addMessageToFrameList(const osc::MultiBuffer &mb,
                                      size_t start, size_t end,
                                      size_t resultSize)
{
  DEBUG_STACK_FRAME;
  if (resultSize == 0)
  {
    return OK;
  }

  osc::byte_t *buffer = new osc::byte_t[resultSize];
  if (!buffer)
  {
    return OUT_OF_MEMORY;
  }
  osc::byte_t *curr = buffer;
  
  for (size_t i = start; i < end; i++)
  {
    *curr = mb[i];
    curr++;

    // Handle escape sequences
    if (mb[i] == 0xFF)
    {
      int skip = mb[i+1];
      size_t j;
      for (j = i+2; j < i+2+skip; j++)
      {
        *curr = mb[j];
        curr++;
      }
      i = j - 1;
    }

  }

  assert(static_cast<size_t>(curr - buffer) == resultSize);

  osc::SigcompMessage *message = new SigcompMessage(buffer, resultSize);
  if (!message)
  {
    delete buffer;
    return OUT_OF_MEMORY;
  }
  message->takeOwnership(buffer);

  osc::TcpStream::Frame *newFrame = new osc::TcpStream::Frame();
  if (!newFrame)
  {
    delete message;
    return OUT_OF_MEMORY;
  }

  if (!m_headFrame)
  {
    m_headFrame = newFrame;
    m_tailFrame = m_headFrame;
  }
  else
  {
    m_tailFrame->next = newFrame;
    m_tailFrame = m_tailFrame->next;
  }

  m_tailFrame->message = message;
  m_tailFrame->next = 0;

  return OK;
}
