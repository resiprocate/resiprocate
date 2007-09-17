#ifndef ASYNCSTAGE_H_
#define ASYNCSTAGE_H_
#include "FdSet.h"
#include <map>
#include <vector>
#include <typeinfo>
#include <queue>
#include "GenericStage.h"
#include "EventInfo.h"
namespace p2p
{

//class TimerRequestEvent;
//class NetworkRequestEvent;

class AsyncStage: public p2p::GenericStage
{
public:
	AsyncStage();
	virtual ~AsyncStage();
	int enqueueEvent(std::auto_ptr<p2p::EventInfo> );
	int handleEvent(std::auto_ptr<p2p::EventInfo> );
	void run();
private:
	class TimerHandlerInfo
	{
	public:
		TimerHandlerInfo();
		TimerHandlerInfo(timeval, TimerRequestEvent *);
		~TimerHandlerInfo();
		TimerRequestEvent * mEventInfo;
		timeval mRequestedTime;
		TimerHandlerInfo &operator= (const TimerHandlerInfo &);
		bool operator== (const TimerHandlerInfo &) const;
		bool operator< (const TimerHandlerInfo &) const;
	};

	class NetworkHandlerInfo
	{
	public:
		NetworkHandlerInfo();
		~NetworkHandlerInfo();
		NetworkRequestEvent * mEventInfo;
	};
	
	class FdHandlerInfo
	{
	public:
		FdHandlerInfo();
		~FdHandlerInfo();
		FdRequestEvent * mEventInfo;
	};
	struct NetworkHandlers{
		std::vector<NetworkHandlerInfo *> readHandlers;
		std::vector<NetworkHandlerInfo *> writeHandlers;
	};
	
	void handleFdRequest(FdRequestEvent *);
	void handleTimerRequest(TimerRequestEvent *);
	void handleNetworkRequest(NetworkRequestEvent *);
	int fireNetworkHandlers(std::vector<NetworkHandlerInfo *> &);
	void clearNetworkHandlers(std::vector<GenericStage *> &, std::vector<NetworkHandlerInfo *> &);
	int processTimerHandlers();
	int processFdHandlers(int);
	int processNetworkHandlers(Socket);
	FdSet * mFdSet;	
	std::priority_queue<TimerHandlerInfo> timerHandlers;
    std::map<Socket, NetworkHandlers > networkHandlerInfoMap;
    std::map <int, FdHandlerInfo *> fdHandlerInfoMap;
  

};
}

#endif /*ASYNCSTAGE_H_*/
