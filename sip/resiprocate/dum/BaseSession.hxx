class BaseSession
{
   public:
      BaseSession(SAManager& sam);
      
      SAManager& sam();
      Dialog& dialog();
      
   private:
      SAManager& mSAM;
      DialogImpl& mDialog;
};

   
