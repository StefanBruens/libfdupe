
add_library(fdupe
            filefinder.cpp
            files.cpp
            fletcher.cpp
            prefixtree.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set_property(TARGET fdupe PROPERTY CXX_STANDARD 11)
set_property(TARGET fdupe PROPERTY CXX_STANDARD_REQUIRED ON)

