#include "mixedbag/bookkeeping_memory_resource.hxx"

#include <format>
#include <stdexcept>

namespace ARo {

void bookkeeping_memory_resource::print_live_allocations(std::ostream& outputStream) const
{
    outputStream << std::format("There are {} live allocations, with a total of {} bytes allocated:\n", liveAllocations_.size(), numAllocatedBytes_);
    for (const auto& allocation : liveAllocations_) {
        outputStream << std::format("  {}: {} bytes, alignment {}\n", allocation.address, allocation.byteCount, allocation.alignment);
    }
}

void* bookkeeping_memory_resource::do_allocate(std::size_t byteCount, std::size_t alignment)
{
    const auto& allocation = liveAllocations_.emplace_back(byteCount, alignment, upstream_->allocate(byteCount, alignment));
    numAllocatedBytes_ += allocation.byteCount;
    return allocation.address;
};

void bookkeeping_memory_resource::do_deallocate(void* address, std::size_t byteCount, std::size_t alignment)
{
    const Allocation allocation{byteCount, alignment, address};

    if (const auto it = std::ranges::find(liveAllocations_, allocation); it != liveAllocations_.end()) {
        liveAllocations_.erase(it);
        deadAllocations_.push_back(allocation);
        numAllocatedBytes_ -= byteCount;
        upstream_->deallocate(address, byteCount, alignment);
        return;
    }

    if (const auto it = std::ranges::find(liveAllocations_, address, &Allocation::address); it != liveAllocations_.end()) {
        throw std::runtime_error(std::format(
            "Mismatched deallocation of {} bytes with alignment {} at address {} - existing allocation was of {} bytes with alignment {}",
            byteCount, alignment, address, it->byteCount, it->alignment));
    }

    if (const auto it = std::ranges::find(deadAllocations_, address, &Allocation::address); it != deadAllocations_.end()) {
        throw std::runtime_error(std::format(
            "Double free of address {}",
            address));
    }

    throw std::runtime_error(std::format("Deallocation of memory that was not allocated by this resource!"));
}

bool bookkeeping_memory_resource::do_is_equal(const memory_resource& other) const noexcept
{
    return &other == this;
}


} // namespace ARo
