#if !defined(RESIP_SERVEROUTOFDIALOGREQ_HXX)
#define RESIP_SERVEROUTOFDIALOGREQ_HXX

namespace resip
{

/** @file ServerOutOfDialogReq.hxx
 *   @todo This file is empty
 */

class ServerOutOfDialogReq : public BaseUsage
{
  public:
    class Handle
    {
    };

    // !rm! do we need this?:    void accept(void);
    void accept(const SipMessage& ok);
    void reject(int statusCode);
    void reject(const SipMessage& response);
};
 
}

#endif
