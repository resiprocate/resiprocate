#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif


#include "resip/stack/SipMessage.hxx"
#include <fstream>
#include <string>

using namespace resip;
using namespace std;

class Args
{
public:

	Args(void):runs(100000),runFs(false),runDs(true)
	{}

	int runs;
	bool runFs;
	bool runDs;
};

void processArgs(int argc, char* argv[],Args &args);

int
main(int argc, char* argv[])
{
	Args args;	

	cout << "\r\n------------------------------------------------------\r\n";
	cout << "Resiprocate resip::SipMessage encoder speed test rev 1.0\r\n";
	cout << "Args: [-r <number of runs>] [-runfs=(yes|no)] [-runds=(yes|no)]\r\n";
	cout << "Example: -r 100000 -runfs=yes -runds=no\r\n";
	cout << "------------------------------------------------------------\r\n";

	processArgs(argc,argv,args);
	
	Data txt("INVITE sip:192.168.2.92:5100;q=1 SIP/2.0\r\n"
               "To: <sip:yiwen_AT_meet2talk.com@whistler.gloo.net>\r\n"
               "From: Jason Fischl<sip:jason_AT_meet2talk.com@whistler.gloo.net>;tag=ba1aee2d\r\n"
               "Via: SIP/2.0/UDP 192.168.2.220:5060;branch=z9hG4bK-c87542-da4d3e6a.0-1--c87542-;rport=5060;received=192.168.2.220;stid=579667358\r\n"
               "Via: SIP/2.0/UDP 192.168.2.15:5100;branch=z9hG4bK-c87542-579667358-1--c87542-;rport=5100;received=192.168.2.15\r\n"
               "Call-ID: 6c64b42fce01b007\r\n"
               "CSeq: 2 INVITE\r\n"
               "Record-Route: <sip:proxy@192.168.2.220:5060;lr>\r\n"
               "Contact: <sip:192.168.2.15:5100>\r\n"
               "Max-Forwards: 69\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 307\r\n"
               "\r\n"
               "v=0\r\n"
               "o=M2TUA 1589993278 1032390928 IN IP4 192.168.2.15\r\n"
               "s=-\r\n"
               "c=IN IP4 192.168.2.15\r\n"
               "t=0 0\r\n"
               "m=audio 9000 RTP/AVP 103 97 100 101 0 8 102\r\n"
               "a=rtpmap:103 ISAC/16000\r\n"
               "a=rtpmap:97 IPCMWB/16000\r\n"
               "a=rtpmap:100 EG711U/8000\r\n"
               "a=rtpmap:101 EG711A/8000\r\n"
               "a=rtpmap:0 PCMU/8000\r\n"
               "a=rtpmap:8 PCMA/8000\r\n"
               "a=rtpmap:102 iLBC/8000\r\n");

	SipMessage *msg;
	msg = SipMessage::make(txt);

	if( NULL == msg )
	{
		cout << "\r\nError: Unable to build test message\r\n";
		return -1;
	}

	cout << "\r\nRunning SipMsg Encoder Speed test\r\n";
#ifdef RESIP_USE_STL_STREAMS
	cout << "USING STL STREAMS\r\n";
#else
	cout << "USING RESIP FAST STREAMS\r\n";
#endif

	UInt64 startTime=0;
	UInt64 elapsed=0;
	double secs=0;

	if( args.runFs )
	{
		fstream fs;
		fs.open("_testSipMsgEncode_.txt",ios_base::out | ios_base::trunc);

		if( !fs.is_open() )
		{
			cout << "Error opening file";
			return -1;
		}			

		cout << "\r\nOutput to file, runs = " << args.runs << ", ...\r\n";
		startTime = Timer::getTimeMs();
		for(int i=0; i<args.runs; i++)
		{
			fs << *msg;			
		}
		elapsed = Timer::getTimeMs() - startTime;
		secs = ((double) elapsed / 1000.0);

		cout << "\r\nOutput to file completed, elapsed time= " << secs << " seconds.\r\n";

	}

	if( args.runDs )
	{
		Data data;
		DataStream resipStr(data);

		cout << "\r\nOutput to resip::DataStream, runs = " << args.runs << ", ...\r\n";

		startTime = Timer::getTimeMs();
		for(int i=0; i<args.runs; i++)
		{
			msg->encode(resipStr);
			data.clear();
		}
		elapsed = Timer::getTimeMs() - startTime;
		secs = ((double) elapsed / 1000.0);

		cout << "\r\nOutput to resip::DataStream completed, elapsed time= " << secs << " seconds.\r\n";
	}

	cout << "Test complete.\r\n";

	return 0;
}

void processArgs(int argc, char* argv[],Args &args)
{
	if( argc <= 1 )
		return;
	
	for( int i=1; i<argc; i++ )
	{
		string arg(argv[i]);			

		if( arg == "-r" )
		{
			if( ++i >= argc )
			{
				cout << "\r\n Bad argument for -r, needs -r <run number>\r\n";
				exit(-1);
			}

			int iruns = atoi(argv[i]);

			if( iruns <= 0 )
			{
				cout << "\r\n Bad argument for -r, needs -r <run number>\r\n";
				exit(-1);
			}

			args.runs = iruns;
		}
		else if( arg.substr(0,7) == "-runfs=" )
		{
			if( arg.substr(7) == "yes" )
			{
				args.runFs = true;
			}
			else
			{
				args.runFs = false;
			}
		}
		else if( arg.substr(0,7) == "-runds=" )
		{
			if( arg.substr(7) == "yes" )
			{
				args.runDs = true;
			}
			else
			{
				args.runDs = false;
			}
		}
	}
}
