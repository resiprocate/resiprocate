#include "repro/Worker.hxx"
#include "repro/Dispatcher.hxx"

#include "resip/stack/SipStack.hxx"
#include "resip/stack/StackThread.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/ApplicationMessage.hxx"

#include "rutil/Data.hxx"

#include <iostream>
#include <cassert>

//main is way down at the bottom.

namespace test
{

class DummyWorkMessage : public resip::ApplicationMessage
{
   public:
      DummyWorkMessage(resip::TransactionUser* passedTU, resip::Data string=resip::Data::Empty)
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
      
      resip::Data& result()
      {
         return mResult;
      }
      
      resip::Data& query()
      {
         return mQuery;
      }
      
      virtual DummyWorkMessage* clone() const {return new DummyWorkMessage(*this);};
      virtual const resip::Data& getTransactionId() const {return mTid;};

      virtual std::ostream& encode(std::ostream& ostr) const { ostr << "DummyWorkMessage("<<mTid<<") "; return ostr; };
      virtual std::ostream& encodeBrief(std::ostream& ostr) const{ ostr << "DummyWorkMessage("<<mTid<<") "; return ostr; };
      
      bool wait;

   private:
      resip::Data mResult;
      resip::Data mQuery;
      resip::Data mTid;
};

class DummyWorker : public repro::Worker
{
   public:
      DummyWorker(){}
      virtual ~DummyWorker(){}
      
      virtual void process(resip::ApplicationMessage* app)
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
};



class DummyTU : public resip::TransactionUser
{
   public:
      DummyTU(resip::SipStack* stack)
      {
         mName="DummyTU";
         mStack=stack;
         mStack->registerTransactionUser(*this);
         DummyWorker* dw = new DummyWorker;
         std::auto_ptr<repro::Worker> worker(dw);
         mDispatcher = new repro::Dispatcher(worker,mStack,3,false);
      }

      virtual ~DummyTU()
      {
         delete mDispatcher;
      }

      virtual void go()
      {
         assert(mDispatcher);
         DummyWorkMessage* lost = new DummyWorkMessage(this);
         
         //Dispatcher should not be accepting work yet
         assert(!mDispatcher->post(std::auto_ptr<resip::ApplicationMessage>(lost)));
                  
         mDispatcher->startAll();
         
         DummyWorkMessage* a = new DummyWorkMessage(this,"A");
         DummyWorkMessage* b = new DummyWorkMessage(this,"B");
         DummyWorkMessage* c = new DummyWorkMessage(this,"C");
         DummyWorkMessage* d = new DummyWorkMessage(this,"D");
         
         std::auto_ptr<resip::ApplicationMessage> aa(a);
         std::auto_ptr<resip::ApplicationMessage> ab(b);
         std::auto_ptr<resip::ApplicationMessage> ac(c);
         std::auto_ptr<resip::ApplicationMessage> ad(d);
         
         assert(mDispatcher->post(aa));
         assert(mDispatcher->post(ab));
         assert(mDispatcher->post(ac));
         assert(mDispatcher->post(ad));
         
         bool gotA=false;
         bool gotB=false;
         bool gotC=false;
         bool gotD=false;
         int i=0;
         resip::Message* msg;
         
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
         assert(!mDispatcher->post(std::auto_ptr<resip::ApplicationMessage>(lost)));
         
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
         
         mDispatcher->post(std::auto_ptr<resip::ApplicationMessage>(wasteTime1));
         mDispatcher->post(std::auto_ptr<resip::ApplicationMessage>(wasteTime2));
         mDispatcher->post(std::auto_ptr<resip::ApplicationMessage>(wasteTime3));
         mDispatcher->post(std::auto_ptr<resip::ApplicationMessage>(wasteTime4));
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
         mDispatcher->post(std::auto_ptr<resip::ApplicationMessage>(clog1));
         mDispatcher->post(std::auto_ptr<resip::ApplicationMessage>(clog2));
         mDispatcher->post(std::auto_ptr<resip::ApplicationMessage>(clog3));

         usleep(100);
         
         for(i=0;i<1000;i++)
         {
            DummyWorkMessage* pummel = new DummyWorkMessage(this);
            std::auto_ptr<resip::ApplicationMessage>batter(pummel);
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
         assert(!(mDispatcher->post(std::auto_ptr<resip::ApplicationMessage>(lost))));
         
         
         
         delete mDispatcher;
         
         
         DummyWorker* dw = new DummyWorker;
         std::auto_ptr<repro::Worker> worker(dw);
         //Stack ptr set to null.
         mDispatcher = new repro::Dispatcher(worker,0,3,true);
         
         std::cout << "The following error message is intentional." << std::endl;
         
         lost = new DummyWorkMessage(this);
         assert(mDispatcher->post(std::auto_ptr<resip::ApplicationMessage>(lost)));
         
         assert(!(msg=mFifo.getNext(100)));
         
         
         
      }
      
      virtual const resip::Data& name() const
      {
         return mName;
      }

      
   private:
      repro::Dispatcher* mDispatcher;
      resip::SipStack* mStack;
      resip::Data mName;

};


}


int
main()
{
   resip::SipStack mStack;
   resip::StackThread stackThread(mStack);
   test::DummyTU mTU(&mStack);
   stackThread.run();
   
   mTU.go();
   std::cout << "PASSED" << std::endl;
   stackThread.shutdown();
   stackThread.join();
   mStack.shutdown();
}

