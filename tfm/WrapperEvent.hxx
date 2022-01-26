#if !defined WrapperEvent_hxx
#define WrapperEvent_hxx

#include "tfm/Expect.hxx"

#include <memory>
#include <utility>

template<class T>
class WrapperEvent : public Event
{
   public:
      
      WrapperEvent(std::shared_ptr<T> t, TestEndPoint* endPoint) :
         Event(endPoint),
         mEvent(std::move(t))
      {
      }

      std::shared_ptr<T> get() const noexcept
      {
         return mEvent;
      }

      //TODO - extract strings from contained event
      virtual resip::Data toString() const 
      {
         return typeid(T).name();
    
         //return resip::Data::Empty;
      }
      
      virtual resip::Data briefString() const
      {
         return typeid(T).name();
         //return resip::Data::Empty;
      }      

   protected:
       std::shared_ptr<T> mEvent;
};

template<class T>
static WrapperEvent<T>* createWrapperEvent(std::shared_ptr<T> t, TestEndPoint* endPoint)
{
   return new WrapperEvent<T>(std::move(t), endPoint);
}

template<class T>
class WrapperEventMatcher : public EventMatcher
{
   public:
      virtual bool isMatch(std::shared_ptr<Event> event)
      {
         WrapperEvent<T>* wevent = dynamic_cast<WrapperEvent<T>*>(event.get());
         if (wevent)
         {
            T* ievent = wevent->get().get();
            if (ievent)
            {
               return true;
            }
         }
         {
            resip::DataStream ds(mMessage);
            ds << "Wrong event type";
         }
         return false;
      }

      virtual resip::Data getMsgTypeString() const
      {
         resip::Data d;
         {
            resip::DataStream ds(d);
            ds << typeid(T).name();
         }
         return d;
      }

      const resip::Data& explainMismatch() const { return mMessage; }

   protected:
      resip::Data mMessage;
};

template<class T>
class WrapperEventPredicate : public ExpectPredicate
{
   protected:
      //on failure, write explanation to mMessage
      virtual bool checkPred(T*)=0;
      resip::Data& getMessage() 
      {
         return mMessage;
      }      
   public:
      virtual bool passes(std::shared_ptr<Event> event)
      {
         WrapperEvent<T>* wevent = dynamic_cast<WrapperEvent<T>*>(event.get());
         if (wevent)
         {
            T* ievent = wevent->get().get();
            if (ievent)
            {
               return checkPred(ievent);
            }
         }
         {
            resip::DataStream ds(mMessage);
            ds << "Wrong event type";
         }
         return false;
      }

      virtual resip::Data getMsgTypeString() const
      {
         resip::Data d;
         {
            resip::DataStream ds(d);
            ds << typeid(T).name();
         }
         return d;
      }

      const resip::Data& explainMismatch() const { return mMessage; }

   protected:
      resip::Data mMessage;
};


#endif
