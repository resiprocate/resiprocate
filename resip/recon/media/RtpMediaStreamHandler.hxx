#ifndef RtpMediaStreamHandler_hxx
#define RtpMediaStreamHandler_hxx

namespace recon
{
/**
 */
class RtpMediaStreamHandler
{
public:
	virtual void onTransportError(unsigned int errorCode) = 0;

	virtual void onRtpTimeout() = 0;
	virtual void onRtcpTimeout() = 0;
};
}

#endif // RtpMediaStreamHandler_hxx