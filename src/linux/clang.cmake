set(CMAKE_C_FLAGS                   "-std=c99 -Wall" CACHE STRING "")
set(CMAKE_C_FLAGS_DEBUG             "-g -DDEBUG" CACHE STRING "")
set(CMAKE_C_FLAGS_RELEASE           "-O3 -DFINAL" CACHE STRING "")
set(CMAKE_C_FLAGS_RELWITHDEBINFO    "-O2 -g -DDEBUG" CACHE STRING "")

set(CMAKE_CXX_FLAGS                 "-std=c++11 -stdlib=libc++ -Wall -Wno-char-subscripts -Wno-missing-braces" CACHE STRING "")
set(CMAKE_CXX_FLAGS_DEBUG           "-g -DDEBUG" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE         "-O3 -DFINAL" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "-O2 -g -DDEBUG" CACHE STRING "")

set(CMAKE_EXE_LINKER_FLAGS          "-lc++abi" CACHE STRING "")

#echo | clang++ -### -c -x c++ -std=c++11 -stdlib=libc++ -
set(CMAKE_PCH_FLAGS
"-triple" "x86_64-pc-linux-gnu" "-mrelax-all" "-disable-free" "-disable-llvm-verifier" "-mrelocation-model" "static" "-mdisable-fp-elim" "-fmath-errno" "-masm-verbose" "-mconstructor-aliases" "-munwind-tables" "-fuse-init-array" "-target-cpu" "x86-64" "-target-linker-version" "2.23.2" "-resource-dir" "/usr/bin/../lib/clang/3.4" "-internal-isystem" "/usr/include/c++/v1" "-internal-isystem" "/usr/local/include" "-internal-isystem" "/usr/bin/../lib/clang/3.4/include" "-internal-isystem" "/usr/bin/../include/clang/3.4/include/" "-internal-externc-isystem" "/usr/include/x86_64-linux-gnu" "-internal-externc-isystem" "/usr/include/x86_64-linux-gnu" "-internal-externc-isystem" "/usr/include" "-std=c++11" "-fdeprecated-macro" "-ferror-limit" "19" "-fmessage-length" "168" "-mstackrealign" "-fobjc-runtime=gcc" "-fcxx-exceptions" "-fexceptions" "-fdiagnostics-show-option" "-fcolor-diagnostics" "-vectorize-slp"
CACHE STRING "")
