#if !defined(RESIP_Handles_hxx)
#define RESIP_Handles_hxx

#include "resiprocate/dum/Handle.hxx"

namespace resip
{

class AppDialogSet;
class AppDialog;

class ClientInviteSession;
class ClientOutOfDialogReq;
class ClientPublication;
class ClientRegistration;
class ClientSubscription;
class ClientPagerMessage;
class ServerPagerMessage;
class InviteSession;
class ServerInviteSession;
class ServerOutOfDialogReq;
class ServerPublication;
class ServerRegistration;
class ServerSubscription;
class BaseUsage;

typedef Handle<AppDialogSet> AppDialogSetHandle;
typedef Handle<AppDialog> AppDialogHandle;
typedef Handle<BaseUsage> BaseUsageHandle;
typedef Handle<ClientOutOfDialogReq> ClientOutOfDialogReqHandle;
typedef Handle<ClientPublication> ClientPublicationHandle;
typedef Handle<ClientRegistration> ClientRegistrationHandle;
typedef Handle<ClientSubscription> ClientSubscriptionHandle;
typedef Handle<ClientPagerMessage> ClientPagerMessageHandle;
typedef Handle<ServerPagerMessage> ServerPagerMessageHandle;
typedef Handle<ClientInviteSession> ClientInviteSessionHandle;
typedef Handle<ServerInviteSession> ServerInviteSessionHandle;
typedef Handle<InviteSession> InviteSessionHandle;
typedef Handle<ServerOutOfDialogReq> ServerOutOfDialogReqHandle;
typedef Handle<ServerPublication> ServerPublicationHandle;
typedef Handle<ServerRegistration> ServerRegistrationHandle;
typedef Handle<ServerSubscription> ServerSubscriptionHandle;


}

#endif
