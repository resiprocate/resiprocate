#if !defined(TurnEndPoint_hxx)
#define TurnEndPoint_hxx

#include "turn/Turn.hxx"
#include "EndPoint.hxx"
#include "tfm/TransportDriver.hxx"

#include "tfm/TurnEvent.hxx"
#include "tfm/Expect.hxx"

class ActionBase;

class TurnEndPoint: public EndPoint,
                    public TransportDriver::Client

{
   public:
      TurnEndPoint(std::auto_ptr<turn::Stack> stack, 
                   int port,
                   const resip::GenericIPAddress& turnServer);

      ~TurnEndPoint();

      void init();
      ActionBase* bind();
      ActionBase* destroy();
      ActionBase* sendTo(const resip::GenericIPAddress& ip, const resip::Data& data);

      virtual void process(resip::FdSet& fdset);
      virtual void buildFdSet(resip::FdSet& fdset);

      virtual resip::Data getName() const 
      {
         return "TurnEndPoint";
      }

      // expects
      ExpectBase* expect(TurnEvent::Type type,
                         int timeoutMs,
                         ActionBase* expectAction);

      ExpectBase* expect(TurnEvent::Type type,
                         ExpectPredicate* pred,
                         int timeoutMs,
                         ActionBase* expectAction);

      class CodeMatch : public ExpectPredicate
      {
         public:
            CodeMatch(TurnEndPoint& endPoint, int code)
               : mEndPoint(endPoint),
                 mCode(code)
            {}
            ~CodeMatch() {}
            resip::Data toString() const { return "Turn error code matcher";}
            bool passes(boost::shared_ptr<Event>)
            {
               return mCode == mEndPoint.mCode;
            }

            virtual const resip::Data& explainMismatch() const { return mMessage; };

         private:
            TurnEndPoint& mEndPoint;
            int mCode;
            resip::Data mMessage;
      };

   private:

      class TurnBinding : public turn::Binding
      {
         public:
            TurnBinding(TurnEndPoint& endPoint, turn::Stack& stack, int port, const resip::GenericIPAddress& server);

         protected:

            void onCreated();
            void onCreationFailure(int code);
            void onActiveDestinationReady();
            void onActiveDestinationFailure(int code);
            void onReceivedData(const resip::Data& data, const resip::GenericIPAddress& source);
            void onBindingFailure(int code);
            void onAlternateServer(const resip::GenericIPAddress& server);

         private:
            TurnEndPoint& mEndPoint;
      };

      void bindDelegate();
      void destroyDelegate();
      void sendToDelegate(const resip::GenericIPAddress& ip, const resip::Data& data);

      bool mInitialized;
      //turn::Stack& mStack;
      std::auto_ptr<turn::Stack> mStack;
      int mPort;
      const resip::GenericIPAddress mTurnServer;
      boost::shared_ptr<TurnBinding> mBinding;

      int mCode;

      friend class TurnBinding;
      friend class CodeMatch;
};

TurnEndPoint::CodeMatch* codeMatch(TurnEndPoint&, int);

#endif
