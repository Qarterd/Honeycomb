dir=$(dirname $0)
export CC=clang
export CXX=clang++
cmake -DCMAKE_TOOLCHAIN_FILE="$dir/clang.cmake" $@
