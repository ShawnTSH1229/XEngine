#pragma once
#include <iostream>
template <typename T>
class XRefCountPtr
{
protected:
	T* ptr_;

    void InternalAddRef() const throw()
    {
        if (ptr_ != nullptr)
        {
            ptr_->AddRef();
        }
    }

    unsigned long InternalRelease() throw()
    {
        unsigned long ref = 0;
        T* temp = ptr_;

        if (temp != nullptr)
        {
            ptr_ = nullptr;
            ref = temp->Release();
        }

        return ref;
    }
public:
    XRefCountPtr() throw() : ptr_(nullptr) {
    }
    
    XRefCountPtr(decltype(__nullptr)) throw() : ptr_(nullptr) {
    }

    template<class U>
    XRefCountPtr(U* other) throw() : ptr_(other)
    {
        InternalAddRef();
    }

    XRefCountPtr(const XRefCountPtr& other) throw() : ptr_(other.ptr_)
    {
        InternalAddRef();
    }

    XRefCountPtr(XRefCountPtr&& other) throw() : ptr_(nullptr)
    {
        if (this != reinterpret_cast<XRefCountPtr*>(&reinterpret_cast<unsigned char&>(other)))
        {
            Swap(other);
        }
    }

    template<class U>
    XRefCountPtr& operator=(const XRefCountPtr<U>& other) throw()
    {
        XRefCountPtr(other).Swap(*this);
        return *this;
    }

    XRefCountPtr& operator=(XRefCountPtr&& other) throw()
    {
        XRefCountPtr(static_cast<XRefCountPtr&&>(other)).Swap(*this);
        return *this;
    }

    void Swap(XRefCountPtr&& r) throw()
    {
        T* tmp = ptr_;
        ptr_ = r.ptr_;
        r.ptr_ = tmp;
    }

    void Swap(XRefCountPtr& r) throw()
    {
        T* tmp = ptr_;
        ptr_ = r.ptr_;
        r.ptr_ = tmp;
    }

    T* Get() const throw()
    {
        return ptr_;
    }

    T** GetAddressOf() throw()
    {
        return &ptr_;
    }

    ~XRefCountPtr()
    {
        InternalRelease();
    }
};