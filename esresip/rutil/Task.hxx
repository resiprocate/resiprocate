#ifndef Task_Include_Guard
#define Task_Include_Guard

namespace resip
{

/**
   An abstract class that represents a task to be performed. Essentially, this 
   is a strongly-typed callback-with-state. Is is expected that subclasses of 
   this will contain both input and output for the task in question. Anything 
   performing the task will be expected to call onDone() whenever the task is 
   done.
*/
class Task
{
   public:
      Task(){};
      virtual ~Task(){};

      /**
         Called when this task is done.
      */
      virtual void onDone()=0;
};
}

#endif
