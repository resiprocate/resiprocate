class BaseCreator
{
   public:
      BaseCreator(SAManager& sam);
      
   private:
      SipMessage mInitialRequest;
      SAManager& mSAM;
};

   
