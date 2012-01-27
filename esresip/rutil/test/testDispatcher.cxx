#include "rutil/Data.hxx"
#include "rutil/Dispatcher.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/Task.hxx"
#include "rutil/Worker.hxx"

#include <iostream>
#include <cassert>

//main is way down at the bottom.

namespace resip
{

class DummyWorkMessage : public Task
{
   public:
      DummyWorkMessage(Fifo<Task>& finishedTasks, Data string=Data::Empty) :
         wait(false),
         mFinishedTasks(finishedTasks),
         mQuery(string)
      {}

      virtual ~DummyWorkMessage(){}

      Data& result()
      {
         return mResult;
      }

      Data& query()
      {
         return mQuery;
      }

      virtual void onDone()
      {
         mFinishedTasks.add(this);
      }

      bool wait;

   private:
      Fifo<Task>& mFinishedTasks;
      Data mResult;
      Data mQuery;
}; // Class DummyWorkMessage

class DummyWorker : public Worker
{
   public:
      DummyWorker(){}
      virtual ~DummyWorker(){}
      
      virtual void process(Task& task)
      {
         DummyWorkMessage* dwm = dynamic_cast<DummyWorkMessage*>(&task);
         if(dwm)
         {
            dwm->result()=" I got that thing you sent me.";
            if(dwm->wait)
            {
               sleep(1);
            }
         }
         task.onDone();
      }
      
      virtual DummyWorker* clone() const
      {
         return new DummyWorker;
      }
}; // Class DummyWorker

class TestDispatcher
{
   public:
      TestDispatcher()
      {
         DummyWorker* dw = new DummyWorker;
         std::auto_ptr<Worker> worker(dw);
         mDispatcher = new Dispatcher(worker,3,false);
      }

      virtual ~TestDispatcher(){};

      virtual void go()
      {
         assert(mDispatcher);
         DummyWorkMessage lost(mFifo);
         
         //Dispatcher should not be accepting work yet
         {
            assert(!mDispatcher->post(lost));
         }

         mDispatcher->startAll();

         DummyWorkMessage a(mFifo,"A");
         DummyWorkMessage b(mFifo,"B");
         DummyWorkMessage c(mFifo,"C");
         DummyWorkMessage d(mFifo,"D");
         
         assert(mDispatcher->post(a));
         assert(mDispatcher->post(b));
         assert(mDispatcher->post(c));
         assert(mDispatcher->post(d));
         
         bool gotA=false;
         bool gotB=false;
         bool gotC=false;
         bool gotD=false;
         int i=0;
         Task* task=0;
         
         for( i=0; i<4 ;i++)
         {
            assert(task = mFifo.getNext(1000));
            
            DummyWorkMessage* dwm = dynamic_cast<DummyWorkMessage*>(task);
            
            assert(dwm);

            if(dwm->result()==" I got that thing you sent me.")
            {
               if(dwm->query()=="A")  gotA=true;
               if(dwm->query()=="B")  gotB=true;
               if(dwm->query()=="C")  gotC=true;
               if(dwm->query()=="D")  gotD=true;
            }
         }
         
         assert(i==4);
         assert(gotA && gotB && gotC && gotD);

         mDispatcher->stop();

         //We stopped the thread bank, it should not be accepting work.
         {
            assert(!mDispatcher->post(lost));
         }

         mDispatcher->resume();

         DummyWorkMessage wasteTime1(mFifo,"Take your time.");
         DummyWorkMessage wasteTime2(mFifo,"Take your time.");
         DummyWorkMessage wasteTime3(mFifo,"Take your time.");
         DummyWorkMessage wasteTime4(mFifo,"Take your time.");
         wasteTime1.wait=true;
         wasteTime2.wait=true;
         wasteTime3.wait=true;
         wasteTime4.wait=true;

         assert(mDispatcher->fifoCountDepth()==0);

         {
            mDispatcher->post(wasteTime1);
            mDispatcher->post(wasteTime2);
            mDispatcher->post(wasteTime3);
            mDispatcher->post(wasteTime4);
         }
         mDispatcher->stop();

         usleep(1000);
         //Three in the thread bank, one in the queue.
         assert(mDispatcher->fifoCountDepth()==1);

         for( i=0; i<4 ;i++)
         {
            assert(task = mFifo.getNext(2000));
            DummyWorkMessage* dwm = dynamic_cast<DummyWorkMessage*>(task);
            assert(dwm);
            assert(dwm->result()==" I got that thing you sent me.");
         }

         assert(!(task=mFifo.getNext(1000)));

         mDispatcher->resume();

         DummyWorkMessage* clog1 = new DummyWorkMessage(mFifo);
         DummyWorkMessage* clog2 = new DummyWorkMessage(mFifo);
         DummyWorkMessage* clog3 = new DummyWorkMessage(mFifo);
         clog1->wait=true;
         clog2->wait=true;
         clog3->wait=true;
         
         //Three threads in the bank, each waiting for 1 sec
         {
            mDispatcher->post(*clog1);
            mDispatcher->post(*clog2);
            mDispatcher->post(*clog3);
         }

         usleep(100);

         for(i=0;i<1000;i++)
         {
            DummyWorkMessage* pummel = new DummyWorkMessage(mFifo);
            assert(mDispatcher->post(*pummel));
         }

         usleep(100000);

         assert(task=mFifo.getNext(1000));
         delete task;

         for(i=0;i<1002;i++)
         {
            task=mFifo.getNext(10);
            assert(task);
            delete task;
         }

         mDispatcher->shutdownAll();

         {
            assert(!(mDispatcher->post(lost)));
         }

         delete mDispatcher;
      }
   
   private:
      Dispatcher* mDispatcher;
      Fifo<Task> mFifo;
}; // class TestDispatcherTU



}


int
main()
{
   resip::TestDispatcher mTest;
   mTest.go();
   std::cout << "PASSED" << std::endl;
}

