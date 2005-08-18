#ifndef SYNCHRONIZEDTESTRESULT_H
#define SYNCHRONIZEDTESTRESULT_H

#include <cppunit/TestResultCollector.h>


class SynchronizedTestResult : public CppUnit::TestResultCollector
{
public:

  class SynchronizationObjectListener
  {
  public:
    virtual ~SynchronizationObjectListener() {}
    virtual void locked() {}
    virtual void unlocked() {}
  };

  class ObservedSynchronizationObject : public CppUnit::SynchronizedObject::SynchronizationObject
  {
  public:
    ObservedSynchronizationObject( SynchronizationObjectListener *listener ) :
        m_listener( listener )
    {
    }

    virtual ~ObservedSynchronizationObject() 
    {
    }

    virtual void lock() 
    {
      m_listener->locked();
    }

    virtual void unlock() 
    {
      m_listener->unlocked();
    }

  private:
    SynchronizationObjectListener *m_listener;
  };


  SynchronizedTestResult( SynchronizationObjectListener *listener )
  {
    setSynchronizationObject( new ObservedSynchronizationObject( listener ) );
  }

  virtual ~SynchronizedTestResult() {}

};

#endif  // SYNCHRONIZEDTESTRESULT_H
