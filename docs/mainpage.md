# ARo Mixed Bag of C++

This is a collection of code that I hope might be useful. It uses some C++ 20 features, so your compiler
needs to have at least basic C++ 20 support.

The project is hosted on GitHub: [https://github.com/andersrosen/mixedbag](https://github.com/andersrosen/mixedbag)

## How to Use in Your Project

The files are standalone, so you can simply download individual files and add them to your source tree.
You can also add the GitHub repository as a git submodule in your own repo, or use `CMake_FetchContent`
to automatically download it for you.

### Add a Git Sub Module

If you want to have this code in the directory `thirdparty/mixedbag` in your source tree, cd to the root
of the source tree and add it as a submodule like this:

    cd my-source-tree
    git submodule add https://github.com/andersrosen/mixedbag.git thirdparty/mixedbag 

If you use CMake, you can then include it in your project by adding it as a subdirectory in `CMakeLists.txt`:

    add_subdirectory(thirdparty/mixedbag)

This will provide a target you can link your own targets to:

    target_link_libraries(my-program PUBLIC mixedbag::mixedbag)

### Add It to Your CMake Project Using FetchContent

Add the following to your CMakeLists.txt:

    include(FetchContent)
    FetchContent_Declare(mixedbag
        GIT_REPOSITORY https://github.com/andersrosen/mixedbag.git
        GIT_TAG latest
    )
    FetchContent_MakeAvailable(mixedbag)

This will tell CMake to download it for you, and it will be made available as a target which you can link
your own targets to:

    target_link_libraries(my-program PUBLIC mixedbag::mixedbag)

## Classes

[sparse_vector](#ARo.sparse_vector) - A vector-backed key-value container for fast unordered iteration of the values

[bookkeeping_memory_resource.hxx](#ARo.bookkeeping_memory_resource) - A memory resource that's intended for use in test code
