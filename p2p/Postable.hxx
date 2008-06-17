#ifndef P2P_Postable_hxx
#define P2P_Postable_hxx

namespace p2p
{

template<class T>
class Postable
{
   public:
      virtual ~Postable(){}
      virtual void post(T*)=0;
};

}

#endif
