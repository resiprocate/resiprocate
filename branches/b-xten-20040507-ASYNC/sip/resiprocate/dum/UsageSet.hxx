
class UsageSet
{
   public:
      typedef std::list<BaseUsage>::iterator BaseUsageIterator;
      BaseUsageIterator begin();
      BaseUsageIterator end();
   private:
      std::list<BaseUsage> mUsages;
};

