#ifndef PRESCONFIG
#define PRESCONFIG

class PresConfig {
  public:
    static PresConfig & instance();
    void initializeStack(resip::SipStack& stack, int argc, char ** argv);
    void initializeResources(int argc, char ** argv);
  private:
    PresConfig(){}
    ~PresConfig(){}
    static PresConfig *theInstance;
};
#endif
