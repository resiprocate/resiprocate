class InvSessionCreator : public BaseCreator
{
   public:
      InvSessionCreator(const Uri& aor, SdpContents* initial);
      void end();
      
   private:
      typedef enum
      {
         Trying, 
         Proceeding
      } State;
      
      State mState;
      
};

   
