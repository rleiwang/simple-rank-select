cmake_minimum_required(VERSION 2.8)

project (simple-rank-select)
#cmake -DLIBXML2_INCLUDE_DIR=/usr/local/libxml2/include/libxml2 -DLIBXML2_LIBRARIES=/usr/local/libxml2/lib/libxml2.so  -DCMAKE_BUILD_TYPE=Debug ../libxml2-play

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
add_executable (simple simple.cpp BitVector.cc ExcessBitVector.cc LRU.cc WaveletSequence.cc)
target_link_libraries (simple ${LIBXML2_LIBRARIES})
