#include "Resource.h"

#include "resiprocate/PlainContents.hxx"

using namespace resip;

Resource::Resource()
{
  mPresenceDocument = new PlainContents("No known Presence document");
}

Resource::~Resource()
{
  delete mPresenceDocument;
}

void
Resource::setPresenceDocument(Contents* document)
{
  delete mPresenceDocument;
  mPresenceDocument = document->clone();
  notifyPDocObservers();
}

void
Resource::attachToPresenceDoc(SubDialog* observer)
{
  mPdocObservers.insert(observer);
}

void
Resource::detachFromPresenceDoc(SubDialog* observer)
{
  PDocObserverSet_t::iterator iter = mPdocObservers.find(observer);
  if (iter!=mPdocObservers.end())
  {
    mPdocObservers.erase(iter);
  }
}

void
Resource::notifyPDocObservers()
{
  PDocObserverSet_t::iterator iter = mPdocObservers.begin();
  while (iter!=mPdocObservers.end())
  {
    (*iter)->presenceDocumentChanged(presenceDocument());
    ++iter;
  }
}
