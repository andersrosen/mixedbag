find_package(Catch2 CONFIG REQUIRED)

add_executable(test_mixedbag)
target_compile_features(test_mixedbag PUBLIC cxx_std_20)

target_sources(test_mixedbag PUBLIC
    test_bookkeeping_memory_resource.cxx
    test_sparse_vector.cxx
)
target_link_libraries(test_mixedbag PRIVATE mixedbag Catch2::Catch2 Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(test_mixedbag)
