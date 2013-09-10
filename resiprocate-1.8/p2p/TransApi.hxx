// Much of the reload protocol should be hidden from app level users of the
// transaction layer. Methods are opqaue to the tranport layer, but a certain
// amount of interaction will be required. 
// 1. A mechanism to correleate requests and responses
// 2. Enough information to populate a Via list
// It is unclear how much more information should be required by the transaction
// layer API.


//this approach exposes a limited message concept which will contrcut transation
//ids...similar to how resip populates a tid when a via header is
//created. Subclasses(store_q, fetch_q, etc) will infer a destination list whenever possible.
//Dignature operations are hidden; signature check methods are expsoed where
//apropriate.

class MessageFactory() // some sort sort of factory sigleton for pasing data from the
                       // wire and forming a new message of the correc type
                       // (like FetchReq for example)
{
  public:
   static ReloadMeassage* parseData( Data msg );
}

// Show me what the elements of a DestinationList look like given they are a
// variant record

class Message
{
   public:
      DestinationList destinationList();
      ViaList viaList(); //is this useful or just diagnostic? (CJ - have to use it )
      TransactionId transactionId(); //populated at const. time(for requests)
      MessageContents contents();
      virtual Data encode();
};


   
//note that signature is not exposed
class MessageContents
{
  public:
   virtual u_int16 messageCode() const=0; //should we req. subclassing?
   virtual Data payload();
  private:
};

// needs classes simular to this for all the message types 
class FetchReq : public Message // Should we derive off Message or
                                      // MEssageContents ????
{
   public:
}
class FetchAns : public Message
{
   public:
}

//base class for MessageContents which can hint at routing?
//shared ptr. for poly. list? Ownership should be clear.
class StoreReq : public MessageContents
{
   public:
      typedef std::list<StoreKindData*> StoreList();
      ResourceID resource();      
      StoreList storeList();
};

//variants here?
class StoreKindData
{
      KindId kindId();
      Generation generation();
};

   
//data model classes

/* Basic operation--2 phase

Contrsuct StoreQ.
call reloadTrans->makeRequest(StoreQ);
custom modification if necessary, store tid
call reloadTrans->send(req)
   
ownership?

Sepcifying a per-operation/per-kind sink will avoid unecessary demux cod ein the
app. 

*/

//from rutil...
class FifoHandler
{
   public:
      void post(Message m);
};

void dhtSipRegister()
{
   //setup stuff...dht
   ReloadTransaction reloadTrans;
   StoreQ s = reloadTrans->makeStoreQ(unhashedId, kind);
   //install payload into s
   //per-request handler model
   reloadTrans->send(s, handler);
}

//1 per data model?
class StoreQHandler
{
      void onFailure(..);
      void onSuccess(StoreQResponse);
};

   
// Each peer in th dht is providing a service to store data; there are light
// validation rules, but it seems that the data can be treated as opaque. The
// main issue is that the kind-id must be supported. I'm not sure if generation
// calcuations should involve the app at all.

//one of these is installed for each kind/dm supported. There will be a subclass
//for each DM. We can use static_cast for the calls or do templates w/ no
//subclassing. Signature checking is done by the sig. service and hidden from
//the app.
class ValidateCRUD
{
};

class ValidateDictionary : public ValidateCRUD
{
      typedef vector<pair<Data, DictEntry>> Dictionary;
      
      bool isValid(const Resourse& r, const Dictionary& d, Generation gen);
};

// storage services are also installed onse per kind/DM
class CRUDService
{
};


class DictionaryService
{
      Generation update(const Resourse& r, const Dictionary& d);
      //not sure about generation here....
      Generation delete(const Resourse& r);
};

class ServiceMap
{
   public:
      void registerKind();
      map<KindId, Validate>;
      
        
};

class ConnectRequest
{
      //peer id
      //protocol
};

   
//under the hood mechanisms that need to be fleshed out
// signature validation/signing service

/* How do we wire this into resip?

- Fetch-Q for location mapped into 3263 style dns lookup
  - transactionState will convey that this is dht to the transportSelector
  - TS will try various alternatives
*/

   
