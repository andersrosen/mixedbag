#include <array>
#include <catch.hpp>

#include <mixedbag/bookkeeping_memory_resource.hxx>
#include <mixedbag/sparse_vector.hxx>

namespace ARo::Test {

struct NonDefaultConstructibleMoveOnlyType {
    int value;

    explicit NonDefaultConstructibleMoveOnlyType(int val)
        : value(val)
    {}
    NonDefaultConstructibleMoveOnlyType(const NonDefaultConstructibleMoveOnlyType&) = delete;
    NonDefaultConstructibleMoveOnlyType(NonDefaultConstructibleMoveOnlyType&& other) noexcept
        : value(other.value)
    {}
    NonDefaultConstructibleMoveOnlyType& operator=(const NonDefaultConstructibleMoveOnlyType&) = delete;
    NonDefaultConstructibleMoveOnlyType& operator=(NonDefaultConstructibleMoveOnlyType&& other) noexcept = default;

    auto operator<=>(const NonDefaultConstructibleMoveOnlyType& other) const noexcept = default;
    bool operator==(const NonDefaultConstructibleMoveOnlyType& other) const noexcept = default;
    bool operator==(int rhs) const noexcept
    {
        return value == rhs;
    }
};


template <typename ValT>
struct IndexValuePair {
    std::size_t idx;
    ValT val;
};

template <typename ElementT, typename IndexT, typename MapElementT = ElementT>
bool equals(const sparse_vector<ElementT, IndexT>& v, const std::map<IndexT, MapElementT>& m)
{
    if (m.size() != v.size())
        return false;

    for (auto [idx, val] : m) {
        if (m.at(idx) != v[idx])
            return false;
    }

    return true;
}

template <typename ElementT, typename PairElementT = ElementT>
sparse_vector<ElementT> makeVec(std::pmr::polymorphic_allocator<> allocator, std::initializer_list<IndexValuePair<PairElementT>> pairs)
{
    sparse_vector<ElementT> v(allocator);
    for (const auto& [idx, val] : pairs) {
        v.emplace(idx, val);
    }
    return v;
}

} // namespace ARo::Test

TEST_CASE("sparse_vector Construction", "[normal]")
{
    std::array<std::byte, 10*1024> buf{};
    std::pmr::monotonic_buffer_resource bufferResource(buf.data(), buf.size());

    SECTION("Initial state")
    {
        ARo::bookkeeping_memory_resource memResource(&bufferResource);
        std::pmr::polymorphic_allocator testAllocator(&memResource);

        ARo::sparse_vector<int> v1;
        REQUIRE(v1.size() == 0U);
        REQUIRE(v1.empty());

        ARo::sparse_vector<int> v2(testAllocator);
        REQUIRE(v2.size() == 0U);
        REQUIRE(v2.empty());
        REQUIRE(memResource.is_unused());
    }

    SECTION("Copy construction")
    {
        ARo::bookkeeping_memory_resource memResource(&bufferResource);
        std::pmr::polymorphic_allocator testAllocator(&memResource);

        SECTION("Copy of empty vector")
        {
            ARo::sparse_vector<int> v(testAllocator);
            auto memResource1InitialAllocationCount = memResource.get_num_live_allocations();

            auto v2 = v;
            REQUIRE(v2.size() == 0U);
            REQUIRE(v2.empty());
            REQUIRE(v2.get_allocator() != v.get_allocator());
            REQUIRE(memResource.get_num_live_allocations() == memResource1InitialAllocationCount);

            ARo::sparse_vector v3{v, v.get_allocator()};
            REQUIRE(v3.size() == 0U);
            REQUIRE(v3.empty());
            REQUIRE(v3.get_allocator() == v.get_allocator());
            REQUIRE(memResource.get_num_live_allocations() == memResource1InitialAllocationCount);

            ARo::bookkeeping_memory_resource memResource2(&bufferResource);
            std::pmr::polymorphic_allocator testAllocator2(&memResource2);

            ARo::sparse_vector v4{v, testAllocator2};
            REQUIRE(v4.size() == 0U);
            REQUIRE(v4.empty());
            REQUIRE(v4.get_allocator() == testAllocator2);
            REQUIRE(memResource2.is_unused());
        }

        SECTION("Copy of populated vector")
        {
            const auto v = ARo::Test::makeVec<int>(testAllocator, {
                {0, 1},
                {5, 14},
                {8, 3}});

            auto memResource1InitialAllocationCount = memResource.get_num_live_allocations();

            std::map<std::size_t, int> expected{
                        {0, 1},
                        {5, 14},
                        {8, 3},
                    };

            const ARo::sparse_vector v2 = v;
            REQUIRE(v2.size() == v.size());
            REQUIRE(v2.get_allocator() != v.get_allocator());
            REQUIRE(memResource.get_num_live_allocations() == memResource1InitialAllocationCount);
            REQUIRE(ARo::Test::equals(v2, expected));

            ARo::bookkeeping_memory_resource memResource2(&bufferResource);
            std::pmr::polymorphic_allocator testAllocator2(&memResource2);

            const ARo::sparse_vector v3(v, testAllocator2);
            REQUIRE(v3.size() == v.size());
            REQUIRE(v3.get_allocator() == testAllocator2);
            REQUIRE(!memResource2.is_unused());
            REQUIRE(memResource.get_num_live_allocations() == memResource1InitialAllocationCount);
            REQUIRE(ARo::Test::equals(v3, expected));
        }
    }

    SECTION("Move construction")
    {
        SECTION("Move of empty vector")
        {
            ARo::bookkeeping_memory_resource memResource(&bufferResource);
            std::pmr::polymorphic_allocator testAllocator(&memResource);

            {
                ARo::sparse_vector<int> v(testAllocator);
                const auto v2 = std::move(v);
                REQUIRE(v2.size() == 0U);
                REQUIRE(v2.empty());
                REQUIRE(v2.get_allocator() == testAllocator);
            }
            REQUIRE(memResource.is_unused());

            {
                ARo::sparse_vector<int> v(testAllocator);
                const ARo::sparse_vector v2{std::move(v), testAllocator};
                REQUIRE(v2.size() == 0U);
                REQUIRE(v2.empty());
                REQUIRE(v2.get_allocator() == testAllocator);
            }
            REQUIRE(memResource.is_unused());

            {
                ARo::sparse_vector<int> v(testAllocator);

                ARo::bookkeeping_memory_resource memResource2(&bufferResource);
                std::pmr::polymorphic_allocator testAllocator2(&memResource2);
                ARo::sparse_vector v2{std::move(v), testAllocator2};
                REQUIRE(v2.size() == 0U);
                REQUIRE(v2.empty());
                REQUIRE(v2.get_allocator() == testAllocator2);
                REQUIRE(memResource2.is_unused());
            }
            REQUIRE(memResource.is_unused());
        }

        SECTION("Move of populated vector")
        {
            std::map<std::size_t, int> expected{
                    {0, 1},
                    {5, 14},
                    {8, 3},
                };

            {
                ARo::bookkeeping_memory_resource memResource(&bufferResource);
                std::pmr::polymorphic_allocator testAllocator(&memResource);
                auto v = ARo::Test::makeVec<int>(testAllocator, {
                    {0, 1},
                    {5, 14},
                    {8, 3}
                });

                const auto memResource1InitialAllocationCount = memResource.get_num_live_allocations();

                ARo::sparse_vector v2{std::move(v)};
                REQUIRE(ARo::Test::equals(v2, expected));
                REQUIRE(v.empty());
                REQUIRE(memResource.get_num_live_allocations() == memResource1InitialAllocationCount);
                REQUIRE(v2.get_allocator() == testAllocator);
            }

            {
                ARo::bookkeeping_memory_resource memResource(&bufferResource);
                std::pmr::polymorphic_allocator testAllocator(&memResource);
                auto v = ARo::Test::makeVec<int>(testAllocator, {
                    {0, 1},
                    {5, 14},
                    {8, 3}
                });

                auto memResource1InitialAllocationCount = memResource.get_num_live_allocations();

                ARo::bookkeeping_memory_resource memResource2(&bufferResource);
                std::pmr::polymorphic_allocator testAllocator2(&memResource2);
                ARo::sparse_vector v2{std::move(v), testAllocator2};
                REQUIRE(ARo::Test::equals(v2, expected));
                REQUIRE(memResource.get_num_live_allocations() == memResource1InitialAllocationCount);
                REQUIRE(v2.get_allocator() == testAllocator2);
                REQUIRE(!memResource2.is_unused());
                REQUIRE(memResource2.get_num_live_allocations() == 2);
            }
        }
    }
}

TEST_CASE("sparse_vector Modifiers", "[normal]")
{
    std::array<std::byte, 10*1024> buf{};
    std::pmr::monotonic_buffer_resource bufferResource(buf.data(), buf.size());

    SECTION("Insert")
    {
        ARo::bookkeeping_memory_resource memResource(&bufferResource);
        std::pmr::polymorphic_allocator testAllocator(&memResource);

        ARo::sparse_vector<int, std::uint8_t> v(testAllocator);
        v.insert(2, 8);
        v.insert(4, 9);

        REQUIRE_FALSE(v.empty());
        REQUIRE(v.size() == 2U);

        {
            const std::map<std::uint8_t, int> expected{
                {2, 8},
                {4, 9},
            };

            REQUIRE(ARo::Test::equals(v, expected));
        }

        REQUIRE_THROWS(v.insert(2, 3));

        v.insert(3, 14);
        v.insert(9, 42);
        REQUIRE(v.size() == 4U);

        auto& ref = v.insert(5, 23);
        REQUIRE(ref == 23);
        ref += 2;

        {
            const std::map<std::uint8_t, int> expected{
            {2, 8},
            {3, 14},
            {4, 9},
            {5, 25},
            {9, 42},
            };

            REQUIRE(ARo::Test::equals(v, expected));
        }



        REQUIRE_THROWS(v.insert(255, 33));
    }

    SECTION("Emplace")
    {
        ARo::bookkeeping_memory_resource memResource(&bufferResource);
        std::pmr::polymorphic_allocator testAllocator(&memResource);

        ARo::sparse_vector<ARo::Test::NonDefaultConstructibleMoveOnlyType, std::uint8_t> v(testAllocator);
        v.emplace(2, 8);
        v.emplace(4, 9);

        REQUIRE_FALSE(v.empty());
        REQUIRE(v.size() == 2U);

        {
            const std::map<std::uint8_t, int> expected{
                    {2, 8},
                    {4, 9},
                };

            REQUIRE(ARo::Test::equals(v, expected));
        }

        REQUIRE_THROWS(v.emplace(2, 3));

        v.emplace(3, 14);
        v.emplace(9, 42);
        REQUIRE(v.size() == 4U);

        auto& ref = v.emplace(5, 23);
        REQUIRE(ref == 23);
        ref.value += 2;

        {
            const std::map<std::uint8_t, int> expected{
                {2, 8},
                {3, 14},
                {4, 9},
                {5, 25},
                {9, 42},
                };

            REQUIRE(ARo::Test::equals(v, expected));
        }

        REQUIRE_THROWS(v.emplace(255, 33));
}

    SECTION("Erase")
    {
        ARo::bookkeeping_memory_resource memResource(&bufferResource);
        std::pmr::polymorphic_allocator testAllocator(&memResource);
        auto v = ARo::Test::makeVec<ARo::Test::NonDefaultConstructibleMoveOnlyType, int>(testAllocator, {
            {0, 4},
            {8, 43},
            {4, 32},
            {25, 2},
            {32, 1},
        });

        std::map<std::size_t, int> expected{
                {0, 4},
                {8, 43},
                {4, 32},
                {25, 2},
                {32, 1},
            };

        REQUIRE_THROWS(v.erase(2));

        v.erase(32);
        expected.erase(32);
        REQUIRE(ARo::Test::equals(v, expected));

        v.erase(4);
        expected.erase(4);
        REQUIRE(ARo::Test::equals(v, expected));

        v.erase(0);
        expected.erase(0);
        REQUIRE(ARo::Test::equals(v, expected));

        v.erase(8);
        expected.erase(8);
        REQUIRE(ARo::Test::equals(v, expected));

        v.erase(25);
        expected.erase(25);
        REQUIRE(ARo::Test::equals(v, expected));
    }
}

TEST_CASE("sparse_vector Assignment", "[normal]")
{
    std::array<std::byte, 10*1024> buf{};
    std::pmr::monotonic_buffer_resource bufferResource(buf.data(), buf.size());
    std::pmr::polymorphic_allocator testAllocator(&bufferResource);

    SECTION("Copy assignment")
    {
        ARo::sparse_vector<int> v1(testAllocator);
        ARo::sparse_vector<int> v2(testAllocator);

        std::pmr::polymorphic_allocator testAllocator2(&bufferResource);
        auto v3 = ARo::Test::makeVec<int>(testAllocator2,{
            {1, 4},
            {8, 10},
        });

        v2 = v1;
        REQUIRE(v2.empty());
        REQUIRE(v2.get_allocator() == testAllocator);

        v2 = v3;
        REQUIRE(v2 == v3);
        REQUIRE(v2.get_allocator() == testAllocator);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
        v2 = v2; // NOLINT: Intentional self-assignment
#pragma clang diagnostic pop

        REQUIRE(v2 == v3);
        REQUIRE(v2.get_allocator() == testAllocator);

        v3 = v1;
        REQUIRE(v3.empty());
        REQUIRE(v3.get_allocator() == testAllocator2);
    }

    SECTION("Move assignment")
    {
        ARo::sparse_vector<int> v1(testAllocator);
        ARo::sparse_vector<int> v2(testAllocator);

        v2 = std::move(v1);
        REQUIRE(v2.empty());
        REQUIRE(v2.get_allocator() == testAllocator);

        std::pmr::polymorphic_allocator testAllocator2(&bufferResource);
        auto v3 = ARo::Test::makeVec<int>(testAllocator2,{
            {1, 4},
            {8, 10},
        });

        v2 = std::move(v3);
        REQUIRE(v2.size() == 2U);
        REQUIRE(v2[1] == 4);
        REQUIRE(v2[8] == 10);
        REQUIRE(v2.get_allocator() == testAllocator);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
        v2 = std::move(v2); // NOLINT: Intentional self-assignment
        REQUIRE(v2.size() == 2U);
#pragma GCC diagnostic pop

        REQUIRE(v2[1] == 4);
        REQUIRE(v2[8] == 10);
        REQUIRE(v2.get_allocator() == testAllocator);

        ARo::sparse_vector v4{v3, testAllocator2};
        ARo::sparse_vector<int> v5(testAllocator);
        v4 = std::move(v5);
        REQUIRE(v4.empty());
        REQUIRE(v4.get_allocator() == testAllocator2);
    }
}

TEST_CASE("sparse_vector Comparison", "[normal]")
{
    std::array<std::byte, 10*1024> buf{};
    std::pmr::monotonic_buffer_resource bufferResource(buf.data(), buf.size());
    ARo::bookkeeping_memory_resource memResource(&bufferResource);

    SECTION("Empty vectors")
    {
        ARo::sparse_vector<int> v1(&memResource);

        REQUIRE(v1 == v1);
        REQUIRE_FALSE(v1 != v1);
        REQUIRE_FALSE((v1 < v1));
        REQUIRE(v1 <= v1);
        REQUIRE_FALSE(v1 > v1);
        REQUIRE(v1 >= v1);

        ARo::sparse_vector<int> v2(&memResource);

        REQUIRE(v1 == v2);
        REQUIRE_FALSE(v1 != v2);
        REQUIRE_FALSE(v1 < v2);
        REQUIRE(v1 <= v2);
        REQUIRE_FALSE(v1 > v2);
        REQUIRE(v1 >= v2);
    }

    SECTION("Empty vector compared with populated vector")
    {
        ARo::sparse_vector<int> v1(&memResource);

        const auto v2 = ARo::Test::makeVec<int>(&memResource, {
            {0, 1},
        });

        REQUIRE_FALSE(v1 == v2);
        REQUIRE(v1 != v2);
        REQUIRE(v1 < v2);
        REQUIRE(v1 <= v2);
        REQUIRE_FALSE(v1 > v2);
        REQUIRE_FALSE(v1 >= v2);

        REQUIRE_FALSE(v2 == v1);
        REQUIRE(v2 != v1);
        REQUIRE(v2 > v1);
        REQUIRE(v2 >= v1);
        REQUIRE_FALSE(v2 < v1);
        REQUIRE_FALSE(v2 <= v1);
    }

    SECTION("Populated vectors of the same length")
    {
        const auto v1 = ARo::Test::makeVec<int>(&memResource, {
            {0, 1},
            {5, 14},
        });

        const auto v2 = ARo::Test::makeVec<int>(&memResource, {
            {1, 1},
            {5, 14},
        });

        const auto v3 = ARo::Test::makeVec<int>(&memResource, {
            {1, 1},
            {5, 13},
        });

        REQUIRE(v1 == v1);
        REQUIRE_FALSE(v1 != v1);
        REQUIRE_FALSE(v1 < v1);
        REQUIRE(v1 <= v1);
        REQUIRE_FALSE(v1 > v1);
        REQUIRE(v1 >= v1);

        REQUIRE_FALSE(v1 == v2);
        REQUIRE(v1 != v2);
        REQUIRE(v1 < v2);
        REQUIRE(v1 <= v2);
        REQUIRE_FALSE(v1 > v2);
        REQUIRE_FALSE(v1 >= v2);

        REQUIRE_FALSE(v2 == v1);
        REQUIRE(v2 != v1);
        REQUIRE_FALSE(v2 < v1);
        REQUIRE_FALSE(v2 <= v1);
        REQUIRE(v2 > v1);
        REQUIRE(v2 >= v1);

        REQUIRE_FALSE(v2 == v3);
        REQUIRE(v2 != v3);
        REQUIRE(v2 > v3);
        REQUIRE(v2 >= v3);
        REQUIRE_FALSE(v2 < v3);
        REQUIRE_FALSE(v2 <= v3);

        REQUIRE_FALSE(v3 == v2);
        REQUIRE(v3 != v2);
        REQUIRE_FALSE(v3 > v2);
        REQUIRE_FALSE(v3 >= v2);
        REQUIRE(v3 < v2);
        REQUIRE(v3 <= v2);

    }

    SECTION("Populated vectors of different lengths")
    {
        const auto v1 = ARo::Test::makeVec<int>(&memResource, {
            {0, 1},
            {5, 14},
        });

        const auto v2 = ARo::Test::makeVec<int>(&memResource, {
            {0, 1},
            {5, 14},
            {8, 99},
        });

        REQUIRE_FALSE(v1 == v2);
        REQUIRE(v1 != v2);
        REQUIRE(v1 < v2);
        REQUIRE(v1 <= v2);
        REQUIRE_FALSE(v1 > v2);
        REQUIRE_FALSE(v1 >= v2);

        REQUIRE_FALSE(v2 == v1);
        REQUIRE(v2 != v1);
        REQUIRE_FALSE(v2 < v1);
        REQUIRE_FALSE(v2 <= v1);
        REQUIRE(v2 > v1);
        REQUIRE(v2 >= v1);

        const auto v3 = ARo::Test::makeVec<int>(&memResource, {
            {0, 1},
            {5, 13},
            {8, 99},
        });

        REQUIRE_FALSE(v1 == v3);
        REQUIRE(v1 != v3);
        REQUIRE_FALSE(v1 < v3);
        REQUIRE_FALSE(v1 <= v3);
        REQUIRE(v1 > v3);
        REQUIRE(v1 >= v3);

        REQUIRE_FALSE(v3 == v1);
        REQUIRE(v3 != v1);
        REQUIRE(v3 < v1);
        REQUIRE(v3 <= v1);
        REQUIRE_FALSE(v3 > v1);
        REQUIRE_FALSE(v3 >= v1);

        const auto v4 = ARo::Test::makeVec<int>(&memResource, {
            {0, 1},
            {2, 14},
            {8, 99},
        });

        REQUIRE_FALSE(v1 == v4);
        REQUIRE(v1 != v4);
        REQUIRE_FALSE(v1 < v4);
        REQUIRE_FALSE(v1 <= v4);
        REQUIRE(v1 > v4);
        REQUIRE(v1 >= v4);

        REQUIRE_FALSE(v4 == v1);
        REQUIRE(v4 != v1);
        REQUIRE(v4 < v1);
        REQUIRE(v4 <= v1);
        REQUIRE_FALSE(v4 > v1);
        REQUIRE_FALSE(v4 >= v1);
    }
}

TEST_CASE("sparse_vector Element Access", "[normal]")
{
    std::array<std::byte, 10*1024> buf{};
    std::pmr::monotonic_buffer_resource bufferResource(buf.data(), buf.size());
    ARo::bookkeeping_memory_resource memResource(&bufferResource);

    SECTION("Empty vector")
    {
        auto v = ARo::Test::makeVec<int>(&memResource, {});

        REQUIRE_THROWS(v[0] = 4);
        REQUIRE_THROWS(v[34] = 0);

        const auto cv = ARo::Test::makeVec<int>(&memResource, {});
        [[maybe_unused]] int x;
        REQUIRE_THROWS(x = cv[0]);
        REQUIRE_THROWS(x = cv[34]);
    }

    SECTION("Vector with multiple elements")
    {
        auto v = ARo::Test::makeVec<int>(&memResource, {
            {1, 11},
            {5, 55},
            {7, 77},
        });
        const ARo::sparse_vector cv{v, &memResource};

        REQUIRE_THROWS(v[0] == 1);
        REQUIRE(v[1] == 11);
        v[1] += 1;
        REQUIRE(v[1] == 12);
        REQUIRE_THROWS(v[4] == 1);
        REQUIRE(v[5] == 55);
        REQUIRE(v[7] == 77);
        REQUIRE_THROWS(v[8] == 1);

        REQUIRE_THROWS(cv[0] == 1);
        REQUIRE(cv[1] == 11);
        REQUIRE_THROWS(cv[4] == 1);
        REQUIRE(cv[5] == 55);
        REQUIRE(cv[7] == 77);
        REQUIRE_THROWS(cv[8] == 1);
    }
}

TEST_CASE("sparse_vector Iteration", "[normal]")
{
    std::array<std::byte, 10*1024> buf{};
    std::pmr::monotonic_buffer_resource bufferResource(buf.data(), buf.size());
    ARo::bookkeeping_memory_resource memResource(&bufferResource);

    SECTION("Empty vector")
    {
        auto v = ARo::Test::makeVec<int>(&memResource, {});

        std::map<int, int> valueCount;
        for (auto it = v.begin(); it != v.end(); ++it) {
            valueCount[*it]++;
        }

        REQUIRE(valueCount.empty());

        const auto& cv = v;
        for (auto it = cv.cbegin(); it != cv.cend(); ++it) {
            valueCount[*it]++;
        }

        REQUIRE(valueCount.empty());
    }

    SECTION("Vector with multiple elements")
    {
        auto v = ARo::Test::makeVec<int>(&memResource, {
            {3, 5},
            {5, 6},
            {53, 4},
            {44, 43},
        });

        {
            std::map<int, int> valueCount;
            for (auto it = v.begin(); it != v.end(); ++it) {
                valueCount[*it]++;
            }

            REQUIRE(valueCount.size() == 4);
            REQUIRE(valueCount[4] == 1);
            REQUIRE(valueCount[5] == 1);
            REQUIRE(valueCount[6] == 1);
            REQUIRE(valueCount[43] == 1);
        }

        {
            auto& cv = v;
            std::map<int, int> valueCount;
            for (auto it = cv.begin(); it != cv.end(); ++it) {
                valueCount[*it]++;
            }

            REQUIRE(valueCount.size() == 4);
            REQUIRE(valueCount[4] == 1);
            REQUIRE(valueCount[5] == 1);
            REQUIRE(valueCount[6] == 1);
            REQUIRE(valueCount[43] == 1);
        }
    }
}
