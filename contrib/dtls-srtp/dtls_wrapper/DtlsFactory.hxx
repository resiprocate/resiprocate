#ifndef DtlsFactory_hxx
#define DtlsFactory_hxx


namespace dtls
{

//Not threadsafe. Timers must fire in the same thread as dtls processing.
class DtlsFactory
{
   public:
      DtlsFactory(std::auto_ptr<DtlsTimerContext> tc);
      DtlsSocket* createClient(DtlsSocketContext* context);
      DtlsSocket* createServer(DtlsSocketContext* context);

      DtlsTimerContext& getTimerContext;
      //context accessor
private:
      SLL_Context* mContext;
      std::auto_ptr<DtlsTimerContext> mTimerContext;
};


#endif
