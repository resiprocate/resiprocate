#ifndef RESOURCE
#define RESOURCE

#include <set>
#include "resip/stack/Contents.hxx"
#include "SubDialog.h"

typedef std::set<SubDialog *> PDocObserverSet_t;

using namespace resip;

class Resource {
  public:
    Resource();
    ~Resource();
    const Contents * presenceDocument() {return mPresenceDocument;}
    void setPresenceDocument(Contents* document);
    void attachToPresenceDoc(SubDialog* observer);
    void detachFromPresenceDoc(SubDialog* observer);

  private:
    void notifyPDocObservers();
    Contents * mPresenceDocument;
    PDocObserverSet_t mPdocObservers;
};

#endif
