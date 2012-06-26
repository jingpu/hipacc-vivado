# Find the native LLVM includes and library
#
#  LLVM_INCLUDE_DIR - where to find llvm include files
#  LLVM_LIBRARY_DIR - where to find llvm libs
#  LLVM_CFLAGS      - llvm C compiler flags
#  LLVM_CXXFLAGS    - llvm C++ compiler flags
#  LLVM_LFLAGS      - llvm linker flags
#  LLVM_MODULE_LIBS - list of llvm libs for working with modules.
#  LLVM_FOUND       - True if llvm found.

FIND_PROGRAM(LLVM_CONFIG_EXECUTABLE NAMES llvm-config DOC "llvm-config executable")
FIND_PROGRAM(CLANG_EXECUTABLE NAMES clang DOC "clang executable")

IF(LLVM_CONFIG_EXECUTABLE)
    MESSAGE(STATUS "LLVM llvm-config found at: ${LLVM_CONFIG_EXECUTABLE}")
    SET(LLVM_FOUND true)
ELSE(LLVM_CONFIG_EXECUTABLE)
    MESSAGE(FATAL_ERROR "Could NOT find LLVM executable")
    SET(LLVM_FOUND false)
ENDIF(LLVM_CONFIG_EXECUTABLE)

IF(CLANG_EXECUTABLE)
    MESSAGE(STATUS "Clang found at: ${CLANG_EXECUTABLE}")
ELSE(CLANG_EXECUTABLE)
    MESSAGE(FATAL_ERROR "Could NOT find Clang executable")
ENDIF(CLANG_EXECUTABLE)

EXECUTE_PROCESS(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
    OUTPUT_VARIABLE LLVM_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

EXECUTE_PROCESS(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
    OUTPUT_VARIABLE LLVM_LIBRARY_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

EXECUTE_PROCESS(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --cppflags
    OUTPUT_VARIABLE LLVM_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

EXECUTE_PROCESS(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
    OUTPUT_VARIABLE LLVM_CXXFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

EXECUTE_PROCESS(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
    OUTPUT_VARIABLE LLVM_LFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

EXECUTE_PROCESS(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs instrumentation ipo
    OUTPUT_VARIABLE LLVM_MODULE_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

