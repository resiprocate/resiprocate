#ifndef DtlsSocket_hxx
#define DtlsSocket_hxx


namespace dtls
{

class DtlsFactory;

class DtlsSocketContext
{
   public:
      //memory is only valid for duration of callback; must be copied if queueing
      //is requried 
      virtual void write(const char* data, unsigned int len)=0;
      void handshakeCompleted()=0;
};

class DtlsSocket
{
   public:
      DtlsSocket(std::auto_ptr<DtlsSocketContext>, DtlsFactory* factory);
      bool consumedPacket(const char* bytes, unsigned int len);
      bool checkFingerprint(const char* fingerprint, unsigned int len);      
      
      //guts of one connection go here

   private:
      DtlsFactory* mFactory;
};

   



#endif








}

#endif
