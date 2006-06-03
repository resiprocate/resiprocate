#ifndef RESOURCEMGR
#define RESOURCEMGR
#include <map>

#include "Resource.h"

using namespace resip;
using namespace std;

typedef map<Data,Resource*> ResourceMap;
typedef ResourceMap::iterator ResourceMap_iter;

class ResourceMgr {
  public:
    static ResourceMgr& instance();
    bool exists(const Data& aor);
    bool addResource(const Data& aor);
    bool setPresenceDocument(const Data& aor, Contents* contents);
    const Contents* presenceDocument(const Data& aor);
    void attachToPresenceDoc(const Data& aor, SubDialog* observer);
    void detachFromPresenceDoc(const Data& aor, SubDialog* observer);
  private:
    ResourceMgr(){}
    ~ResourceMgr(){}
    ResourceMap mResources;
    static ResourceMgr* mInstance;
};
#endif
