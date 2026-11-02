#include <iostream>
#include <memory>

#include "resip/stack/ApplicationMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/TransactionUserMessage.hxx"

using namespace std;
using namespace resip;

static int failures = 0;

static void
check(bool condition, const char* what)
{
   cerr << (condition ? "ok   " : "FAIL ") << what << endl;
   if (!condition)
   {
      ++failures;
   }
}

class TestAppMessage : public ApplicationMessage
{
   public:
      explicit TestAppMessage(TransactionUser* tu) { setTransactionUser(tu); }
      Message* clone() const override { return new TestAppMessage(*this); }
      EncodeStream& encode(EncodeStream& s) const override { return s << "TestAppMessage"; }
      EncodeStream& encodeBrief(EncodeStream& s) const override { return encode(s); }
};

class TestTu : public TransactionUser
{
   public:
      const Data& name() const override { static const Data n("TestTu"); return n; }

      // Returns the next message, or nullptr when the fifo is empty.
      unique_ptr<Message> next()
      {
         return unique_ptr<Message>(mFifo.messageAvailable() ? mFifo.getNext() : nullptr);
      }
};

static void
postTo(SipStack& stack, TestTu& tu)
{
   stack.post(unique_ptr<ApplicationMessage>(new TestAppMessage(&tu)));
}

int
main()
{
   TestTu tu;
   TestTu tu2;
   SipStack stack;

   // The first processTimers() starts processing, so the registration below
   // is queued.
   stack.processTimers();
   stack.registerTransactionUser(tu);
   postTo(stack, tu);
   stack.processTimers();

   unique_ptr<Message> msg = tu.next();
   check(dynamic_cast<TestAppMessage*>(msg.get()) != nullptr, "message posted before registration is applied arrives");

   postTo(stack, tu);
   msg = tu.next();
   check(dynamic_cast<TestAppMessage*>(msg.get()) != nullptr, "registered TU receives a posted message");

   postTo(stack, tu);
   stack.unregisterTransactionUser(tu);
   stack.processTimers();

   msg = tu.next();
   check(dynamic_cast<TestAppMessage*>(msg.get()) != nullptr, "message posted before unregister is delivered");
   msg = tu.next();
   TransactionUserMessage* removed = dynamic_cast<TransactionUserMessage*>(msg.get());
   check(removed && removed->type() == TransactionUserMessage::TransactionUserRemoved,
         "TransactionUserRemoved follows it");

   postTo(stack, tu);
   check(!tu.next(), "message posted after removal is dropped");

   // Registration and unregister both queued before the TransactionController
   // applies either of them.
   stack.registerTransactionUser(tu2);
   postTo(stack, tu2);
   stack.unregisterTransactionUser(tu2);
   stack.processTimers();

   msg = tu2.next();
   check(dynamic_cast<TestAppMessage*>(msg.get()) != nullptr, "message posted to a pending TU is delivered");
   msg = tu2.next();
   removed = dynamic_cast<TransactionUserMessage*>(msg.get());
   check(removed && removed->type() == TransactionUserMessage::TransactionUserRemoved,
         "pending TU unregistered in the same pass gets TransactionUserRemoved");

   cerr << (failures == 0 ? "PASS" : "FAILED") << endl;

   return failures == 0 ? 0 : 1;
}
