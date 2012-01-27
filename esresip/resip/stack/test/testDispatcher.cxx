#include "resip/stack/test/DummyTU.hxx"

#include "resip/stack/Worker.hxx"
#include "resip/stack/Dispatcher.hxx"

#include "resip/stack/SipStack.hxx"
#include "resip/stack/StackThread.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/ApplicationMessage.hxx"

#include "rutil/Data.hxx"

#include <iostream>
#include <cassert>

//main is way down at the bottom.

namespace resip
{

class DummyWorkMessage : public ApplicationMessage
{
   public:
      DummyWorkMessage(TransactionUser* passedTU, Data string=Data::Empty)
      {
         tu=passedTU;
         mTid="";
         mQuery=string;
         wait=false;
      }
      
      
      DummyWorkMessage(const DummyWorkMessage& orig)
      {
         tu=orig.tu;
         mTid=orig.mTid;
         mQuery=orig.mQuery;
         mResult=orig.mResult;
         wait=orig.wait;
      }
      
      virtual ~DummyWorkMessage(){}
      
      Data& result()
      {
         return mResult;
      }
      
      Data& query()
      {
         return mQuery;
      }
      
      virtual DummyWorkMessage* clone() const {return new DummyWorkMessage(*this);};
      virtual const Data& getTransactionId() const {return mTid;};

      virtual std::ostream& encode(std::ostream& ostr) const { ostr << "DummyWorkMessage("<<mTid<<") "; return ostr; };
      virtual std::ostream& encodeBrief(std::ostream& ostr) const{ ostr << "DummyWorkMessage("<<mTid<<") "; return ostr; };
      
      bool wait;

   private:
      Data mResult;
      Data mQuery;
      Data mTid;
}; // Class DummyWorkMessage

class DummyWorker : public Worker
{
   public:
      DummyWorker(){}
      virtual ~DummyWorker(){}
      
      virtual void process(ApplicationMessage* app)
      {
         DummyWorkMessage* dwm = dynamic_cast<DummyWorkMessage*>(app);
         if(dwm)
         {
            dwm->result()=" I got that thing you sent me.";
            if(dwm->wait)
            {
               sleep(1);
            }
         }
         
      }
      
      virtual DummyWorker* clone() const
      {
         return new DummyWorker;
      }
}; // Class DummyWorker

class TestDispatcherTU : public DummyTU
{
   public:
      TestDispatcherTU()
      {
         DummyWorker* dw = new DummyWorker;
         std::auto_ptr<Worker> worker(dw);
         mDispatcher = new Dispatcher(worker,mStack,3,false);
      }

      virtual void go()
      {
         assert(mDispatcher);
         DummyWorkMessage* lost = new DummyWorkMessage(this);
         
         //Dispatcher should not be accepting work yet
         {
            std::auto_ptr<ApplicationMessage> autoLost(lost);
            assert(!mDispatcher->post(autoLost));
         }

         mDispatcher->startAll();
         
         DummyWorkMessage* a = new DummyWorkMessage(this,"A");
         DummyWorkMessage* b = new DummyWorkMessage(this,"B");
         DummyWorkMessage* c = new DummyWorkMessage(this,"C");
         DummyWorkMessage* d = new DummyWorkMessage(this,"D");
         
         std::auto_ptr<ApplicationMessage> aa(a);
         std::auto_ptr<ApplicationMessage> ab(b);
         std::auto_ptr<ApplicationMessage> ac(c);
         std::auto_ptr<ApplicationMessage> ad(d);
         
         assert(mDispatcher->post(aa));
         assert(mDispatcher->post(ab));
         assert(mDispatcher->post(ac));
         assert(mDispatcher->post(ad));
         
         bool gotA=false;
         bool gotB=false;
         bool gotC=false;
         bool gotD=false;
         int i=0;
         Message* msg;
         
         for( i=0; i<4 ;i++)
         {
            assert(msg = mFifo.getNext(1000));
            
            DummyWorkMessage* dwm = dynamic_cast<DummyWorkMessage*>(msg);
            
            assert(dwm);

            if(dwm->result()==" I got that thing you sent me.")
            {
               if(dwm->query()=="A")  gotA=true;
               if(dwm->query()=="B")  gotB=true;
               if(dwm->query()=="C")  gotC=true;
               if(dwm->query()=="D")  gotD=true;
            }
            
            delete dwm;

         }
         
         assert(i==4);
         assert(gotA && gotB && gotC && gotD);
         
         mDispatcher->stop();
         
         lost = new DummyWorkMessage(this,"");
         
         //We stopped the thread bank, it should not be accepting work.
         {
            std::auto_ptr<ApplicationMessage> autoLost(lost);
            assert(!mDispatcher->post(autoLost));
         }
         
         mDispatcher->resume();
         
         DummyWorkMessage* wasteTime1 = new DummyWorkMessage(this,"Take your time.");
         DummyWorkMessage* wasteTime2 = new DummyWorkMessage(this,"Take your time.");
         DummyWorkMessage* wasteTime3 = new DummyWorkMessage(this,"Take your time.");
         DummyWorkMessage* wasteTime4 = new DummyWorkMessage(this,"Take your time.");
         wasteTime1->wait=true;
         wasteTime2->wait=true;
         wasteTime3->wait=true;
         wasteTime4->wait=true;
         
         assert(mDispatcher->fifoCountDepth()==0);
         
         {
            std::auto_ptr<ApplicationMessage> autoWasteTime1(wasteTime1);
            mDispatcher->post(autoWasteTime1);
            std::auto_ptr<ApplicationMessage> autoWasteTime2(wasteTime2);
            mDispatcher->post(autoWasteTime2);
            std::auto_ptr<ApplicationMessage> autoWasteTime3(wasteTime3);
            mDispatcher->post(autoWasteTime3);
            std::auto_ptr<ApplicationMessage> autoWasteTime4(wasteTime4);
            mDispatcher->post(autoWasteTime4);
         }
         mDispatcher->stop();
         
         usleep(1000);
         //Three in the thread bank, one in the queue.
         assert(mDispatcher->fifoCountDepth()==1);
         
         for( i=0; i<4 ;i++)
         {
            assert(msg = mFifo.getNext(2000));
            
            DummyWorkMessage* dwm = dynamic_cast<DummyWorkMessage*>(msg);
            
            assert(dwm);

            assert(dwm->result()==" I got that thing you sent me.");
            
            delete dwm;
         }
         
         assert(!(msg=mFifo.getNext(1000)));
         
         mDispatcher->resume();
         
         DummyWorkMessage* clog1 = new DummyWorkMessage(this);
         DummyWorkMessage* clog2 = new DummyWorkMessage(this);
         DummyWorkMessage* clog3 = new DummyWorkMessage(this);
         clog1->wait=true;
         clog2->wait=true;
         clog3->wait=true;
         
         //Three threads in the bank, each waiting for 1 sec
         {
            std::auto_ptr<ApplicationMessage> autoClog1(clog1);
            mDispatcher->post(autoClog1);
            std::auto_ptr<ApplicationMessage> autoClog2(clog2);
            mDispatcher->post(autoClog2);
            std::auto_ptr<ApplicationMessage> autoClog3(clog3);
            mDispatcher->post(autoClog3);
         }

         usleep(100);
         
         for(i=0;i<1000;i++)
         {
            DummyWorkMessage* pummel = new DummyWorkMessage(this);
            std::auto_ptr<ApplicationMessage> batter(pummel);
            assert(mDispatcher->post(batter));
         }

         
         usleep(100000);
         
         assert(msg=mFifo.getNext(1000));
         
         delete msg;
         
         
         
         for(i=0;i<1002;i++)
         {
            msg=mFifo.getNext(10);
            assert(msg);
            delete msg;
         }

         mDispatcher->shutdownAll();
         
         
         lost = new DummyWorkMessage(this);
         {
            std::auto_ptr<ApplicationMessage> autoLost(lost);
            assert(!(mDispatcher->post(autoLost)));
         }
         
         
         delete mDispatcher;
         
         
         DummyWorker* dw = new DummyWorker;
         std::auto_ptr<Worker> worker(dw);
         //Stack ptr set to null.
         mDispatcher = new Dispatcher(worker,0,3,true);
         
         std::cout << "The following error message is intentional." << std::endl;
         
         lost = new DummyWorkMessage(this);
         {
            std::auto_ptr<ApplicationMessage> autoLost(lost);
            assert(mDispatcher->post(autoLost));
         }
         
         assert(!(msg=mFifo.getNext(100)));
      }
   
   private:
      Dispatcher* mDispatcher;
}; // class TestDispatcherTU



}


int
main()
{
   resip::TestDispatcherTU mTU;
   mTU.go();
   std::cout << "PASSED" << std::endl;
}

