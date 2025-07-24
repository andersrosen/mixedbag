#pragma once

#include <mixedbag/exports.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory_resource>

namespace ARo {

/**
 * A memory resource meant for tests, that can be queried for stats, checks deallocations that they match a live allocation, and asserts on leaks.
 */
class MIXEDBAG_EXPORT bookkeeping_memory_resource final : public std::pmr::memory_resource {
    public:
    explicit bookkeeping_memory_resource(std::pmr::memory_resource* upstream)
        : upstream_(upstream)
        , liveAllocations_(upstream)
        , deadAllocations_(upstream)
    {};

    bookkeeping_memory_resource() : bookkeeping_memory_resource(std::pmr::get_default_resource()) {}

    virtual ~bookkeeping_memory_resource()
    {
        if (!has_no_leak()) {
            std::cerr << "Leaking memory resource!\n";
            std::terminate();
        }
    }

    [[nodiscard]] std::size_t get_num_live_allocations() const noexcept
    {
        return liveAllocations_.size();
    }

    [[nodiscard]] std::size_t get_num_deallocations() const noexcept
    {
        return deadAllocations_.size();
    }

    [[nodiscard]] std::size_t get_num_live_allocated_bytes() const noexcept
    {
        return numAllocatedBytes_;
    }

    [[nodiscard]] std::size_t is_unused() const noexcept
    {
        return liveAllocations_.empty() && deadAllocations_.empty();
    }

    [[nodiscard]] std::size_t has_no_leak() const noexcept
    {
        return liveAllocations_.empty();
    }

    void print_live_allocations(std::ostream& outputStream) const;

    private:
    struct Allocation {
        std::size_t byteCount;
        std::size_t alignment;
        void* address;

        auto operator<=>(const Allocation&) const noexcept = default;
    };

    std::pmr::memory_resource* upstream_;
    std::pmr::vector<Allocation> liveAllocations_;
    std::pmr::vector<Allocation> deadAllocations_;
    std::size_t numAllocatedBytes_ = 0;

    void* do_allocate(std::size_t byteCount, std::size_t alignment) override;
    void do_deallocate(void* address, std::size_t byteCount, std::size_t alignment) override;
    bool do_is_equal(const memory_resource& other) const noexcept override;
};

} // namespace ARo

