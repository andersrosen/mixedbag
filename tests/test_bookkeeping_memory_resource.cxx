#include <catch.hpp>
#include <mixedbag/bookkeeping_memory_resource.hxx>

TEST_CASE("bookkeping_memory_resource", "[normal]")
{
    ARo::bookkeeping_memory_resource memResource;

    REQUIRE(memResource.is_unused());
    REQUIRE(memResource.has_no_leak());
    REQUIRE(memResource.get_num_live_allocations() == 0);
    REQUIRE(memResource.get_num_live_allocated_bytes() == 0);
    REQUIRE(memResource.get_num_deallocations() == 0);

    void* foo = memResource.allocate(10, 2);
    REQUIRE_FALSE(memResource.is_unused());
    REQUIRE_FALSE(memResource.has_no_leak());
    REQUIRE(memResource.get_num_live_allocations() == 1);
    REQUIRE(memResource.get_num_live_allocated_bytes() == 10);
    REQUIRE(memResource.get_num_deallocations() == 0);

    REQUIRE_THROWS(memResource.deallocate(foo, 1, 2));
    REQUIRE_THROWS(memResource.deallocate(foo, 10, 3));
    REQUIRE_THROWS(memResource.deallocate(&foo, 10, 2));

    memResource.deallocate(foo, 10, 2);
    REQUIRE_THROWS(memResource.deallocate(foo, 10, 2));

    REQUIRE(memResource.get_num_live_allocations() == 0);
    REQUIRE(memResource.get_num_live_allocated_bytes() == 0);
    REQUIRE(memResource.get_num_deallocations() == 1);
    REQUIRE(memResource.has_no_leak());

    void* bar = memResource.allocate(100, 4);
    REQUIRE(memResource.get_num_live_allocations() == 1);
    REQUIRE(memResource.get_num_deallocations() == 1);
    REQUIRE_FALSE(memResource.has_no_leak());

    memResource.deallocate(bar, 100, 4);
    REQUIRE(memResource.has_no_leak());
    REQUIRE_FALSE(memResource.is_unused());
}