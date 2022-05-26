#pragma once

#include <wtypes.h>

#include <cassert>
#include <memory>	// std::addressof

template<typename OutType>
class TypedBuffer
{
public:
    TypedBuffer(size_t size)
        : buffer_(std::make_unique<byte[]>(size))
        , size_(size)
    {}

    operator OutType *() const
    {
        return (OutType*)buffer_.get();
    }

    const OutType * ptr() const
    {
        return (OutType*)buffer_.get();
    }

    size_t size() const
    {
        return size_;
    }

private:
    std::unique_ptr<byte[]> buffer_;
    const size_t size_;
};


template<typename T>
class scoped_ptr : public std::unique_ptr<T, void(*)(T *)>
{
public:
    using deleter = void(*)(T *);

    scoped_ptr(T * v, const deleter & d) : std::unique_ptr<T, deleter>(v, d) {};

    operator T * () const { return this->get(); }
};


template<typename ResourceType, typename ResourceTag = ResourceType>
class ScopedResource
{
    ScopedResource(const ScopedResource &) = delete;
    ScopedResource & operator=(const ScopedResource &) = delete;
public:
    ScopedResource() noexcept = default;

    explicit ScopedResource(ResourceType resource) noexcept
        : resource_{ resource }
    {}

    ScopedResource(ScopedResource && other) noexcept
        : resource_{ other.resource_ }
    {
        other.resource_ = {};
    }

    ScopedResource & operator=(ScopedResource && other) noexcept
    {
        assert(this != std::addressof(other));
        Cleanup();
        resource_ = other.resource_;
        other.resource_ = {};
        return *this;
    }

    ~ScopedResource()
    {
        Cleanup();
    }

    operator const ResourceType & () const noexcept
    {
        return resource_;
    }

    const ResourceType * operator&() const noexcept
    {
        return &resource_;
    }

    ResourceType * operator&() noexcept
    {
        Cleanup();
        resource_ = {};
        return &resource_;
    }

private:
    // must be explicitly specialized.
    void Cleanup() noexcept;

    ResourceType resource_{};
};

//

using Handle = ScopedResource<HANDLE, struct HandleTag>;
using HMenu = ScopedResource<HMENU>;
using HKey = ScopedResource<HKEY>;
using PSid = ScopedResource<PSID, struct PSIDTag>;

