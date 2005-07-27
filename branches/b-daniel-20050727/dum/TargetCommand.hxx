
class TargetCommand
{
   public:
      class Target
      {
         public:
            virtual void post()(std::auto_ptr<Message>)=0;
      };
      
      void execute()
      {
         mTarget(mMessage);
      }
      
   private:
      Target* mTarget;
      std::auto_ptr<Message> mMessage;
};

      
     
