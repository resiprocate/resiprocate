////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design 
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any 
//     purpose is hereby granted without fee, provided that the above copyright 
//     notice appear in all copies and that both that copyright notice and this 
//     permission notice appear in supporting documentation.
// The author or Addison-Welsey Longman make no representations about the 
//     suitability of this software for any purpose. It is provided "as is" 
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

// Last update: June 20, 2001

#ifndef SMARTPTR_INC_
#define SMARTPTR_INC_

////////////////////////////////////////////////////////////////////////////////
// IMPORTANT NOTE
// Due to threading issues, the OwnershipPolicy has been changed as follows:
//     Release() returns a boolean saying if that was the last release
//        so the pointer can be deleted by the StoragePolicy
//     IsUnique() was removed
////////////////////////////////////////////////////////////////////////////////


#include "SmallObj.h"
#include "TypeManip.h"
#include "static_check.h"
#include <functional>
#include <stdexcept>

namespace Loki
{

////////////////////////////////////////////////////////////////////////////////
// class template DefaultSPStorage
// Implementation of the StoragePolicy used by SmartPtr
////////////////////////////////////////////////////////////////////////////////

    template <class T>
    class DefaultSPStorage
    {
    protected:
        typedef T* StoredType;    // the type of the pointee_ object
        typedef T* PointerType;   // type returned by operator->
        typedef T& ReferenceType; // type returned by operator*
        
    public:
        DefaultSPStorage() : pointee_(Default()) 
        {}

        // The storage policy doesn't initialize the stored pointer 
        //     which will be initialized by the OwnershipPolicy's Clone fn
        DefaultSPStorage(const DefaultSPStorage&)
        {}

        template <class U>
        DefaultSPStorage(const DefaultSPStorage<U>&) 
        {}
        
        DefaultSPStorage(const StoredType& p) : pointee_(p) {}
        
        PointerType operator->() const { return pointee_; }
        
        ReferenceType operator*() const { return *pointee_; }
        
        void Swap(DefaultSPStorage& rhs)
        { std::swap(pointee_, rhs.pointee_); }
    
        // Accessors
        friend inline PointerType GetImpl(const DefaultSPStorage& sp)
        { return sp.pointee_; }
        
    	friend inline const StoredType& GetImplRef(const DefaultSPStorage& sp)
    	{ return sp.pointee_; }

    	friend inline StoredType& GetImplRef(DefaultSPStorage& sp)
    	{ return sp.pointee_; }

    protected:
        // Destroys the data stored
        // (Destruction might be taken over by the OwnershipPolicy)
        void Destroy()
        { delete pointee_; }
        
        // Default value to initialize the pointer
        static StoredType Default()
        { return 0; }
    
    private:
        // Data
        StoredType pointee_;
    };

////////////////////////////////////////////////////////////////////////////////
// class template RefCounted
// Implementation of the OwnershipPolicy used by SmartPtr
// Provides a classic external reference counting implementation
////////////////////////////////////////////////////////////////////////////////

    template <class P>
    class RefCounted
    {
    public:
        RefCounted() 
        {
            pCount_ = static_cast<unsigned int*>(
                SmallObject<>::operator new(sizeof(unsigned int)));
            assert(pCount_);
            *pCount_ = 1;
        }
        
        RefCounted(const RefCounted& rhs) 
        : pCount_(rhs.pCount_)
        {}
        
        // MWCW lacks template friends, hence the following kludge
        template <typename P1>
        RefCounted(const RefCounted<P1>& rhs) 
        : pCount_(reinterpret_cast<const RefCounted&>(rhs).pCount_)
        {}
        
        P Clone(const P& val)
        {
            ++*pCount_;
            return val;
        }
        
        bool Release(const P&)
        {
            if (!--*pCount_)
            {
                SmallObject<>::operator delete(pCount_, sizeof(unsigned int));
                return true;
            }
            return false;
        }
        
        void Swap(RefCounted& rhs)
        { std::swap(pCount_, rhs.pCount_); }
    
        enum { destructiveCopy = false };

    private:
        // Data
        unsigned int* pCount_;
    };
    
////////////////////////////////////////////////////////////////////////////////
// class template RefCountedMT
// Implementation of the OwnershipPolicy used by SmartPtr
// Implements external reference counting for multithreaded programs
////////////////////////////////////////////////////////////////////////////////
    template <class P,
        template <class> class ThreadingModel>
    class RefCountedMT : public ThreadingModel< RefCountedMT<P, ThreadingModel> >
    {
    public:
        RefCountedMT() 
        {
            pCount_ = static_cast<unsigned int*>(
                SmallObject<ThreadingModel>::operator new(
                    sizeof(unsigned int)));
            assert(pCount_);
            *pCount_ = 1;
        }
        
        RefCountedMT(const RefCountedMT& rhs) 
        : pCount_(rhs.pCount_)
        {}
        
        // MWCW lacks template friends, hence the following kludge
        template <typename P1>
        RefCountedMT(const RefCountedMT<P1, ThreadingModel>& rhs) 
        : pCount_(reinterpret_cast<const RefCounted<P>&>(rhs).pCount_)
        {}
        
        P Clone(const P& val)
        {
            ThreadingModel<RefCountedMT>::AtomicIncrement(*pCount_);
            return val;
        }
        
        bool Release(const P&)
        {
            if (!ThreadingModel<RefCountedMT>::AtomicDecrement(*pCount_))
            {
                SmallObject<ThreadingModel>::operator delete(pCount_, 
                    sizeof(unsigned int));
                return true;
            }
            return false;
        }
        
        void Swap(RefCountedMT& rhs)
        { std::swap(pCount_, rhs.pCount_); }
    
        enum { destructiveCopy = false };

    private:
        // Data
        volatile unsigned int* pCount_;
    };

////////////////////////////////////////////////////////////////////////////////
// class template COMRefCounted
// Implementation of the OwnershipPolicy used by SmartPtr
// Adapts COM intrusive reference counting to OwnershipPolicy-specific syntax
////////////////////////////////////////////////////////////////////////////////

    template <class P>
    class COMRefCounted
    {
    public:
        COMRefCounted()
        {}
        
        template <class U>
        COMRefCounted(const COMRefCounted<U>&)
        {}
        
        static P Clone(const P& val)
        {
            val->AddRef();
            return val;
        }
        
        static bool Release(const P& val)
        { val->Release(); return false; }
        
        enum { destructiveCopy = false };
        
        static void Swap(COMRefCounted&)
        {}
    };

////////////////////////////////////////////////////////////////////////////////
// class template DeepCopy
// Implementation of the OwnershipPolicy used by SmartPtr
// Implements deep copy semantics, assumes existence of a Clone() member 
//     function of the pointee type
////////////////////////////////////////////////////////////////////////////////

    template <class P>
    struct DeepCopy
    {
        DeepCopy()
        {}
        
        template <class P1>
        DeepCopy(const DeepCopy<P1>&)
        {}
        
        static P Clone(const P& val)
        { return val->Clone(); }
        
        static bool Release(const P& val)
        { return true; }
        
        static void Swap(DeepCopy&)
        {}
        
        enum { destructiveCopy = false };
    };
    
////////////////////////////////////////////////////////////////////////////////
// class template RefLinked
// Implementation of the OwnershipPolicy used by SmartPtr
// Implements reference linking
////////////////////////////////////////////////////////////////////////////////

    namespace Private
    {
        class RefLinkedBase
        {
        public:
            RefLinkedBase() 
            { prev_ = next_ = this; }
            
            RefLinkedBase(const RefLinkedBase& rhs) 
            {
                prev_ = &rhs;
                next_ = rhs.next_;
                prev_->next_ = this;
                next_->prev_ = this;
            }
            
            bool Release()
            {
                if (next_ == this)
                {   
                    assert(prev_ == this);
                    return true;
                }
                prev_->next_ = next_;
                next_->prev_ = prev_;
                return false;
            }
            
            void Swap(RefLinkedBase& rhs)
            {
                if (next_ == this)
                {
                    assert(prev_ == this);
                    if (rhs.next_ == &rhs)
                    {
                        assert(rhs.prev_ == &rhs);
                        // both lists are empty, nothing 2 do
                        return;
                    }
                    prev_ = rhs.prev_;
                    next_ = rhs.next_;
                    prev_->next_ = next_->prev_ = this;
                    rhs.next_ = rhs.prev_ = &rhs;
                    return;
                }
                if (rhs.next_ == &rhs)
                {
                    rhs.Swap(*this);
                    return;
                }
                std::swap(prev_, rhs.prev_);
                std::swap(next_, rhs.next_);
                std::swap(prev_->next_, rhs.prev_->next_);
                std::swap(next_->prev_, rhs.next_->prev_);
            }
                
            enum { destructiveCopy = false };

        private:
            mutable const RefLinkedBase* prev_;
            mutable const RefLinkedBase* next_;
        };
    }
    
    template <class P>
    class RefLinked : public Private::RefLinkedBase
    {
    public:
        RefLinked()
        {}
        
        template <class P1>
        RefLinked(const RefLinked<P1>& rhs) 
        : Private::RefLinkedBase(rhs)
        {}

        static P Clone(const P& val)
        { return val; }

        bool Release(const P&)
        { return Private::RefLinkedBase::Release(); }
    };
    
////////////////////////////////////////////////////////////////////////////////
// class template DestructiveCopy
// Implementation of the OwnershipPolicy used by SmartPtr
// Implements destructive copy semantics (a la std::auto_ptr)
////////////////////////////////////////////////////////////////////////////////

    template <class P>
    class DestructiveCopy
    {
    public:
        DestructiveCopy()
        {}
        
        template <class P1>
        DestructiveCopy(const DestructiveCopy<P1>&)
        {}
        
        template <class P1>
        static P Clone(P1& val)
        {
            P result(val);
            val = P1();
            return result;
        }
        
        static bool Release(const P&)
        { return true; }
        
        static void Swap(DestructiveCopy&)
        {}
        
        enum { destructiveCopy = true };
    };
    
////////////////////////////////////////////////////////////////////////////////
// class template NoCopy
// Implementation of the OwnershipPolicy used by SmartPtr
// Implements a policy that doesn't allow copying objects
////////////////////////////////////////////////////////////////////////////////

    template <class P>
    class NoCopy
    {
    public:
        NoCopy()
        {}
        
        template <class P1>
        NoCopy(const NoCopy<P1>&)
        {}
        
        static P Clone(const P&)
        {
            CT_ASSERT(false, This_Policy_Disallows_Value_Copying);
        }
        
        static bool Release(const P&)
        { return true; }
        
        static void Swap(NoCopy&)
        {}
        
        enum { destructiveCopy = false };
    };
    
////////////////////////////////////////////////////////////////////////////////
// class template AllowConversion
// Implementation of the ConversionPolicy used by SmartPtr
// Allows implicit conversion from SmartPtr to the pointee type
////////////////////////////////////////////////////////////////////////////////

    struct AllowConversion
    {
        enum { allow = true };

        void Swap(AllowConversion&)
        {}
    };

////////////////////////////////////////////////////////////////////////////////
// class template DisallowConversion
// Implementation of the ConversionPolicy used by SmartPtr
// Does not allow implicit conversion from SmartPtr to the pointee type
// You can initialize a DisallowConversion with an AllowConversion
////////////////////////////////////////////////////////////////////////////////

    struct DisallowConversion
    {
        DisallowConversion()
        {}
        
        DisallowConversion(const AllowConversion&)
        {}
        
        enum { allow = false };

        void Swap(DisallowConversion&)
        {}
    };

////////////////////////////////////////////////////////////////////////////////
// class template NoCheck
// Implementation of the CheckingPolicy used by SmartPtr
// Well, it's clear what it does :o)
////////////////////////////////////////////////////////////////////////////////

    template <class P>
    struct NoCheck
    {
        NoCheck()
        {}
        
        template <class P1>
        NoCheck(const NoCheck<P1>&)
        {}
        
        static void OnDefault(const P&)
        {}

        static void OnInit(const P&)
        {}

        static void OnDereference(const P&)
        {}

        static void Swap(NoCheck&)
        {}
    };


////////////////////////////////////////////////////////////////////////////////
// class template AssertCheck
// Implementation of the CheckingPolicy used by SmartPtr
// Checks the pointer before dereference
////////////////////////////////////////////////////////////////////////////////

    template <class P>
    struct AssertCheck
    {
        AssertCheck()
        {}
        
        template <class P1>
        AssertCheck(const AssertCheck<P1>&)
        {}
        
        template <class P1>
        AssertCheck(const NoCheck<P1>&)
        {}
        
        static void OnDefault(const P&)
        {}

        static void OnInit(const P&)
        {}

        static void OnDereference(P val)
        { assert(val); }

        static void Swap(AssertCheck&)
        {}
    };

////////////////////////////////////////////////////////////////////////////////
// class template AssertCheckStrict
// Implementation of the CheckingPolicy used by SmartPtr
// Checks the pointer against zero upon initialization and before dereference
// You can initialize an AssertCheckStrict with an AssertCheck 
////////////////////////////////////////////////////////////////////////////////

    template <class P>
    struct AssertCheckStrict
    {
        AssertCheckStrict()
        {}
        
        template <class U>
        AssertCheckStrict(const AssertCheckStrict<U>&)
        {}
        
        template <class U>
        AssertCheckStrict(const AssertCheck<U>&)
        {}
        
        template <class P1>
        AssertCheckStrict(const NoCheck<P1>&)
        {}
        
        static void OnDefault(P val)
        { assert(val); }
        
        static void OnInit(P val)
        { assert(val); }
        
        static void OnDereference(P val)
        { assert(val); }
        
        static void Swap(AssertCheckStrict&)
        {}
    };

////////////////////////////////////////////////////////////////////////////////
// class NullPointerException
// Used by some implementations of the CheckingPolicy used by SmartPtr
////////////////////////////////////////////////////////////////////////////////

    struct NullPointerException : public std::runtime_error
    {
        NullPointerException() : std::runtime_error("")
        { }
        const char* what() const
        { return "Null Pointer Exception"; }
    };
        
////////////////////////////////////////////////////////////////////////////////
// class template RejectNullStatic
// Implementation of the CheckingPolicy used by SmartPtr
// Checks the pointer upon initialization and before dereference
////////////////////////////////////////////////////////////////////////////////

    template <class P>
    struct RejectNullStatic
    {
        RejectNullStatic()
        {}
        
        template <class P1>
        RejectNullStatic(const RejectNullStatic<P1>&)
        {}
        
        template <class P1>
        RejectNullStatic(const NoCheck<P1>&)
        {}
        
        template <class P1>
        RejectNullStatic(const AssertCheck<P1>&)
        {}
        
        template <class P1>
        RejectNullStatic(const AssertCheckStrict<P1>&)
        {}
        
        static void OnDefault(const P&)
        {
            CompileTimeError<false>
                ERROR_This_Policy_Does_Not_Allow_Default_Initialization;
        }
        
        static void OnInit(const P& val)
        { if (!val) throw NullPointerException(); }
        
        static void OnDereference(const P& val)
        { if (!val) throw NullPointerException(); }
        
        static void Swap(RejectNullStatic&)
        {}
    };

////////////////////////////////////////////////////////////////////////////////
// class template RejectNull
// Implementation of the CheckingPolicy used by SmartPtr
// Checks the pointer before dereference
////////////////////////////////////////////////////////////////////////////////

    template <class P>
    struct RejectNull
    {
        RejectNull()
        {}
        
        template <class P1>
        RejectNull(const RejectNull<P1>&)
        {}
        
        static void OnInit(P val)
        { if (!val) throw NullPointerException(); }

        static void OnDefault(P val)
        { OnInit(val); }
        
        void OnDereference(P val)
        { OnInit(val); }
        
        void Swap(RejectNull&)
        {}        
    };

////////////////////////////////////////////////////////////////////////////////
// class template RejectNullStrict
// Implementation of the CheckingPolicy used by SmartPtr
// Checks the pointer upon initialization and before dereference
////////////////////////////////////////////////////////////////////////////////

    template <class P>
    struct RejectNullStrict
    {
        RejectNullStrict()
        {}
        
        template <class P1>
        RejectNullStrict(const RejectNullStrict<P1>&)
        {}
        
        template <class P1>
        RejectNullStrict(const RejectNull<P1>&)
        {}
        
        static void OnInit(P val)
        { if (!val) throw NullPointerException(); }

        void OnDereference(P val)
        { OnInit(val); }
        
        void Swap(RejectNullStrict&)
        {}        
    };

////////////////////////////////////////////////////////////////////////////////
// class template ByRef
// Transports a reference as a value
// Serves to implement the Colvin/Gibbons trick for SmartPtr
////////////////////////////////////////////////////////////////////////////////

    template <class T>
    class ByRef
    {
    public:
        ByRef(T& v) : value_(v) {}
        operator T&() { return value_; }
        // gcc doesn't like this:
        // operator const T&() const { return value_; }
    private:
        T& value_;
    };

////////////////////////////////////////////////////////////////////////////////
// class template SmartPtr (declaration)
// The reason for all the fuss above
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OwnershipPolicy = RefCounted,
        class ConversionPolicy = DisallowConversion,
        template <class> class CheckingPolicy = AssertCheck,
        template <class> class StoragePolicy = DefaultSPStorage
    >
    class SmartPtr;

////////////////////////////////////////////////////////////////////////////////
// class template SmartPtr (definition)
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OwnershipPolicy,
        class ConversionPolicy,
        template <class> class CheckingPolicy,
        template <class> class StoragePolicy
    >
    class SmartPtr
        : public StoragePolicy<T>
        , public OwnershipPolicy<typename StoragePolicy<T>::PointerType>
        , public CheckingPolicy<typename StoragePolicy<T>::StoredType>
        , public ConversionPolicy
    {
        typedef StoragePolicy<T> SP;
        typedef OwnershipPolicy<typename StoragePolicy<T>::PointerType> OP;
        typedef CheckingPolicy<typename StoragePolicy<T>::StoredType> KP;
        typedef ConversionPolicy CP;
        
    public:
        typedef typename SP::PointerType PointerType;
        typedef typename SP::StoredType StoredType;
        typedef typename SP::ReferenceType ReferenceType;
        
        typedef typename Select<OP::destructiveCopy, 
                SmartPtr, const SmartPtr>::Result
            CopyArg;
    
        SmartPtr()
        { KP::OnDefault(GetImpl(*this)); }
    	
        SmartPtr(const StoredType& p) : SP(p)
        { KP::OnInit(GetImpl(*this)); }
    	
    	SmartPtr(CopyArg& rhs)
        : SP(rhs), OP(rhs), KP(rhs), CP(rhs)
        { GetImplRef(*this) = OP::Clone(GetImplRef(rhs)); }

        template
        <
            typename T1,
            template <class> class OP1,
            class CP1,
            template <class> class KP1,
            template <class> class SP1
        >
    	SmartPtr(const SmartPtr<T1, OP1, CP1, KP1, SP1>& rhs)
    	: SP(rhs), OP(rhs), KP(rhs), CP(rhs)
    	{ GetImplRef(*this) = OP::Clone(GetImplRef(rhs)); }

        template
        <
            typename T1,
            template <class> class OP1,
            class CP1,
            template <class> class KP1,
            template <class> class SP1
        >
    	SmartPtr(SmartPtr<T1, OP1, CP1, KP1, SP1>& rhs)
    	: SP(rhs), OP(rhs), KP(rhs), CP(rhs)
    	{ GetImplRef(*this) = OP::Clone(GetImplRef(rhs)); }

        SmartPtr(ByRef<SmartPtr> rhs)
    	: SP(rhs), OP(rhs), KP(rhs), CP(rhs)
        {}
        
        operator ByRef<SmartPtr>()
        { return ByRef<SmartPtr>(*this); }

    	SmartPtr& operator=(CopyArg& rhs)
    	{
    	    SmartPtr temp(rhs);
    	    temp.Swap(*this);
    	    return *this;
    	}

        template
        <
            typename T1,
            template <class> class OP1,
            class CP1,
            template <class> class KP1,
            template <class> class SP1
        >
    	SmartPtr& operator=(const SmartPtr<T1, OP1, CP1, KP1, SP1>& rhs)
    	{
    	    SmartPtr temp(rhs);
    	    temp.Swap(*this);
    	    return *this;
    	}
    	
        template
        <
            typename T1,
            template <class> class OP1,
            class CP1,
            template <class> class KP1,
            template <class> class SP1
        >
    	SmartPtr& operator=(SmartPtr<T1, OP1, CP1, KP1, SP1>& rhs)
    	{
    	    SmartPtr temp(rhs);
    	    temp.Swap(*this);
    	    return *this;
    	}
    	
    	void Swap(SmartPtr& rhs)
    	{
    	    OP::Swap(rhs);
    	    CP::Swap(rhs);
    	    KP::Swap(rhs);
    	    SP::Swap(rhs);
    	}
    	
    	~SmartPtr()
    	{
    	    if (OP::Release(GetImpl(*static_cast<SP*>(this))))
    	    {
    	        SP::Destroy();
    	    }
    	}
    	
    	friend inline void Release(SmartPtr& sp, typename SP::StoredType& p)
    	{
    	    p = GetImplRef(sp);
    	    GetImplRef(sp) = SP::Default();
    	}
    	
    	friend inline void Reset(SmartPtr& sp, typename SP::StoredType p)
    	{ SmartPtr(p).Swap(sp); }

        PointerType operator->()
        {
            KP::OnDereference(GetImplRef(*this));
            return SP::operator->();
        }

        PointerType operator->() const
        {
            KP::OnDereference(GetImplRef(*this));
            return SP::operator->();
        }

        ReferenceType operator*()
        {
            KP::OnDereference(GetImplRef(*this));
            return SP::operator*();
        }
    	
        ReferenceType operator*() const
        {
            KP::OnDereference(GetImplRef(*this));
            return SP::operator*();
        }
    	
        bool operator!() const // Enables "if (!sp) ..."
        { return GetImpl(*this) == 0; }
        
        inline friend bool operator==(const SmartPtr& lhs,
            const T* rhs)
        { return GetImpl(lhs) == rhs; }
        
        inline friend bool operator==(const T* lhs,
            const SmartPtr& rhs)
        { return rhs == lhs; }
        
        inline friend bool operator!=(const SmartPtr& lhs,
            const T* rhs)
        { return !(lhs == rhs); }
        
        inline friend bool operator!=(const T* lhs,
            const SmartPtr& rhs)
        { return rhs != lhs; }

        // Ambiguity buster
        template
        <
            typename T1,
            template <class> class OP1,
            class CP1,
            template <class> class KP1,
            template <class> class SP1
        >
        bool operator==(const SmartPtr<T1, OP1, CP1, KP1, SP1>& rhs) const
        { return *this == GetImpl(rhs); }

        // Ambiguity buster
        template
        <
            typename T1,
            template <class> class OP1,
            class CP1,
            template <class> class KP1,
            template <class> class SP1
        >
        bool operator!=(const SmartPtr<T1, OP1, CP1, KP1, SP1>& rhs) const
        { return !(*this == rhs); }

        // Ambiguity buster
        template
        <
            typename T1,
            template <class> class OP1,
            class CP1,
            template <class> class KP1,
            template <class> class SP1
        >
        bool operator<(const SmartPtr<T1, OP1, CP1, KP1, SP1>& rhs) const
        { return *this < GetImpl(rhs); }

    private:
        // Helper for enabling 'if (sp)'
        struct Tester
        {
            Tester() {}
        private:
            void operator delete(void*);
        };
        
    public:
        // enable 'if (sp)'
        operator Tester*() const
        {
            if (!*this) return 0;
            static Tester t;
            return &t;
        }

    private:
        // Helper for disallowing automatic conversion
        struct Insipid
        {
            Insipid(PointerType) {}
        };
        
        typedef typename Select<CP::allow, PointerType, Insipid>::Result
            AutomaticConversionResult;
    
    public:        
        operator AutomaticConversionResult() const
        { return GetImpl(*this); }
    };

////////////////////////////////////////////////////////////////////////////////
// free comparison operators for class template SmartPtr
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// operator== for lhs = SmartPtr, rhs = raw pointer
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP,
        typename U
    >
    inline bool operator==(const SmartPtr<T, OP, CP, KP, SP>& lhs,
        const U* rhs)
    { return GetImpl(lhs) == rhs; }
    
////////////////////////////////////////////////////////////////////////////////
// operator== for lhs = raw pointer, rhs = SmartPtr
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP,
        typename U
    >
    inline bool operator==(const U* lhs,
        const SmartPtr<T, OP, CP, KP, SP>& rhs)
    { return rhs == lhs; }

////////////////////////////////////////////////////////////////////////////////
// operator!= for lhs = SmartPtr, rhs = raw pointer
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP,
        typename U
    >
    inline bool operator!=(const SmartPtr<T, OP, CP, KP, SP>& lhs,
        const U* rhs)
    { return !(lhs == rhs); }
    
////////////////////////////////////////////////////////////////////////////////
// operator!= for lhs = raw pointer, rhs = SmartPtr
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP,
        typename U
    >
    inline bool operator!=(const U* lhs,
        const SmartPtr<T, OP, CP, KP, SP>& rhs)
    { return rhs != lhs; }

////////////////////////////////////////////////////////////////////////////////
// operator< for lhs = SmartPtr, rhs = raw pointer -- NOT DEFINED
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP,
        typename U
    >
    inline bool operator<(const SmartPtr<T, OP, CP, KP, SP>& lhs,
        const U* rhs);
        
////////////////////////////////////////////////////////////////////////////////
// operator< for lhs = raw pointer, rhs = SmartPtr -- NOT DEFINED
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP,
        typename U
    >
    inline bool operator<(const U* lhs,
        const SmartPtr<T, OP, CP, KP, SP>& rhs);
        
////////////////////////////////////////////////////////////////////////////////
// operator> for lhs = SmartPtr, rhs = raw pointer -- NOT DEFINED
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP,
        typename U
    >
    inline bool operator>(const SmartPtr<T, OP, CP, KP, SP>& lhs,
        const U* rhs)
    { return rhs < lhs; }
        
////////////////////////////////////////////////////////////////////////////////
// operator> for lhs = raw pointer, rhs = SmartPtr
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP,
        typename U
    >
    inline bool operator>(const U* lhs,
        const SmartPtr<T, OP, CP, KP, SP>& rhs)
    { return rhs < lhs; }
  
////////////////////////////////////////////////////////////////////////////////
// operator<= for lhs = SmartPtr, rhs = raw pointer
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP,
        typename U
    >
    inline bool operator<=(const SmartPtr<T, OP, CP, KP, SP>& lhs,
        const U* rhs)
    { return !(rhs < lhs); }
        
////////////////////////////////////////////////////////////////////////////////
// operator<= for lhs = raw pointer, rhs = SmartPtr
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP,
        typename U
    >
    inline bool operator<=(const U* lhs,
        const SmartPtr<T, OP, CP, KP, SP>& rhs)
    { return !(rhs < lhs); }

////////////////////////////////////////////////////////////////////////////////
// operator>= for lhs = SmartPtr, rhs = raw pointer
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP,
        typename U
    >
    inline bool operator>=(const SmartPtr<T, OP, CP, KP, SP>& lhs,
        const U* rhs)
    { return !(lhs < rhs); }
        
////////////////////////////////////////////////////////////////////////////////
// operator>= for lhs = raw pointer, rhs = SmartPtr
////////////////////////////////////////////////////////////////////////////////

    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP,
        typename U
    >
    inline bool operator>=(const U* lhs,
        const SmartPtr<T, OP, CP, KP, SP>& rhs)
    { return !(lhs < rhs); }

} // namespace Loki

////////////////////////////////////////////////////////////////////////////////
// specialization of std::less for SmartPtr
////////////////////////////////////////////////////////////////////////////////

namespace std
{
    template
    <
        typename T,
        template <class> class OP,
        class CP,
        template <class> class KP,
        template <class> class SP
    >
    struct less< Loki::SmartPtr<T, OP, CP, KP, SP> >
        : public binary_function<Loki::SmartPtr<T, OP, CP, KP, SP>,
            Loki::SmartPtr<T, OP, CP, KP, SP>, bool>
    {
        bool operator()(const Loki::SmartPtr<T, OP, CP, KP, SP>& lhs,
            const Loki::SmartPtr<T, OP, CP, KP, SP>& rhs) const
        { return less<T*>()(GetImpl(lhs), GetImpl(rhs)); }
    };
}

////////////////////////////////////////////////////////////////////////////////
// Change log:
// June 20, 2001: ported by Nick Thurn to gcc 2.95.3. Kudos, Nick!!!
////////////////////////////////////////////////////////////////////////////////

#endif // SMARTPTR_INC_
