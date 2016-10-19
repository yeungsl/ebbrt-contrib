set(CMAKE_CXX_COMPILER "/home/yeungsl/EbbRT/toolchain/sysroot/usr/bin/x86_64-pc-ebbrt-g++")
set(CMAKE_CXX_COMPILER_ARG1 "")
set(CMAKE_CXX_COMPILER_ID "GNU")
set(CMAKE_CXX_COMPILER_VERSION "5.3.0")
set(CMAKE_CXX_PLATFORM_ID "")

set(CMAKE_AR "/home/yeungsl/EbbRT/toolchain/sysroot/usr/bin/x86_64-pc-ebbrt-gcc-ar")
set(CMAKE_RANLIB "/home/yeungsl/EbbRT/toolchain/sysroot/usr/bin/x86_64-pc-ebbrt-gcc-ranlib")
set(CMAKE_LINKER "/home/yeungsl/EbbRT/toolchain/sysroot/usr/bin/x86_64-pc-ebbrt-ld")
set(CMAKE_COMPILER_IS_GNUCXX 1)
set(CMAKE_CXX_COMPILER_LOADED 1)
set(CMAKE_CXX_COMPILER_WORKS TRUE)
set(CMAKE_CXX_ABI_COMPILED TRUE)
set(CMAKE_COMPILER_IS_MINGW )
set(CMAKE_COMPILER_IS_CYGWIN )
if(CMAKE_COMPILER_IS_CYGWIN)
  set(CYGWIN 1)
  set(UNIX 1)
endif()

set(CMAKE_CXX_COMPILER_ENV_VAR "CXX")

if(CMAKE_COMPILER_IS_MINGW)
  set(MINGW 1)
endif()
set(CMAKE_CXX_COMPILER_ID_RUN 1)
set(CMAKE_CXX_IGNORE_EXTENSIONS inl;h;hpp;HPP;H;o;O;obj;OBJ;def;DEF;rc;RC)
set(CMAKE_CXX_SOURCE_FILE_EXTENSIONS C;M;c++;cc;cpp;cxx;m;mm;CPP)
set(CMAKE_CXX_LINKER_PREFERENCE 30)
set(CMAKE_CXX_LINKER_PREFERENCE_PROPAGATES 1)

# Save compiler ABI information.
set(CMAKE_CXX_SIZEOF_DATA_PTR "8")
set(CMAKE_CXX_COMPILER_ABI "ELF")
set(CMAKE_CXX_LIBRARY_ARCHITECTURE "")

if(CMAKE_CXX_SIZEOF_DATA_PTR)
  set(CMAKE_SIZEOF_VOID_P "${CMAKE_CXX_SIZEOF_DATA_PTR}")
endif()

if(CMAKE_CXX_COMPILER_ABI)
  set(CMAKE_INTERNAL_PLATFORM_ABI "${CMAKE_CXX_COMPILER_ABI}")
endif()

if(CMAKE_CXX_LIBRARY_ARCHITECTURE)
  set(CMAKE_LIBRARY_ARCHITECTURE "")
endif()




set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "stdc++;m;ebbrt;capnp;kj;acpica;tbb;stdc++;supc++;m;c;g;nosys;ebbrt")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "/home/yeungsl/EbbRT/toolchain/sysroot/usr/lib/gcc/x86_64-pc-ebbrt/5.3.0;/home/yeungsl/EbbRT/toolchain/sysroot/usr/x86_64-pc-ebbrt/lib;/home/yeungsl/EbbRT/toolchain/sysroot/usr/lib")
set(CMAKE_CXX_IMPLICIT_LINK_FRAMEWORK_DIRECTORIES "")



