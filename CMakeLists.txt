set(ENV{GIT_TERMINAL_PROMPT} 1)
set(ENV{GIT_CURL_VERBOSE} 1)
set(ENV{GIT_HTTPS_PROXY} "")
set(ENV{GIT_SSH_COMMAND} "ssh -o ConnectTimeout=60")

# Extend Git timeout for FetchContent
set(ENV{GIT_HTTP_LOW_SPEED_LIMIT} 0)
set(ENV{GIT_HTTP_LOW_SPEED_TIME} 999999)
set(ENV{GIT_HTTP_CONNECT_TIMEOUT} 120)


cmake_minimum_required(VERSION 3.10)
project(ImageDSL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DANTLR4CPP_STATIC)

include(FetchContent)

# -----------------------------
# Fetch ANTLR4 runtime
# -----------------------------
FetchContent_Declare(
    antlr4_runtime
    GIT_REPOSITORY https://github.com/antlr/antlr4.git
    GIT_TAG 4.13.0
)
FetchContent_MakeAvailable(antlr4_runtime)


# Build ANTLR runtime library
add_subdirectory(${antlr4_runtime_SOURCE_DIR}/runtime/Cpp)

# -----------------------------
# Download ANTLR tool jar
# -----------------------------
set(ANTLR_JAR "${CMAKE_BINARY_DIR}/antlr-4.13.0-complete.jar")
if (NOT EXISTS ${ANTLR_JAR})
    message(STATUS "Downloading ANTLR4 tool jar...")
    file(DOWNLOAD
        https://www.antlr.org/download/antlr-4.13.0-complete.jar
        ${ANTLR_JAR}
        SHOW_PROGRESS
    )
endif()


# -----------------------------
# Fetch STBI
# -----------------------------


FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# =========================================
# Find LLVM
# =========================================
find_package(LLVM REQUIRED CONFIG)

# Print LLVM info (optional but useful)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM libraries: ${LLVM_LIBRARIES}")

# =========================================
# Include LLVM headers and definitions
# =========================================
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# =========================================
# Find OpenCV
# =========================================

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# -----------------------------
# Grammar and generated directory
# -----------------------------
set(GRAMMAR_FILE ${PROJECT_SOURCE_DIR}/grammar/ImageLang.g4)
set(GENERATED_DIR ${PROJECT_SOURCE_DIR}/src/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

# -----------------------------
# Custom target to generate grammar
# -----------------------------
add_custom_target(GenerateGrammar
    COMMAND java -jar ${ANTLR_JAR} -Dlanguage=Cpp -visitor -o ${GENERATED_DIR} ${GRAMMAR_FILE}
    COMMENT "🧩 Generating ANTLR C++ sources..."
    VERBATIM
)

# -----------------------------
# Source files (include generated sources dynamically at build time)
# -----------------------------
file(GLOB_RECURSE SRC_FILES src/*.cpp)

add_executable(image-dsl ${SRC_FILES})

# Ensure grammar is generated before build
add_dependencies(image-dsl GenerateGrammar)

# Include generated sources dynamically after they're built
add_custom_command(
    TARGET image-dsl PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "🔧 Including generated ANTLR sources..."
    COMMAND ${CMAKE_COMMAND} -E sleep 0.1
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${GENERATED_DIR}/dummy.cpp
)

# # Add the generated sources directory to the build system
# target_sources(image-dsl PRIVATE
#     ${GENERATED_DIR}/ImageLangLexer.cpp
#     ${GENERATED_DIR}/ImageLangParser.cpp
#     ${GENERATED_DIR}/ImageLangBaseVisitor.cpp
#     ${GENERATED_DIR}/ImageLangVisitor.cpp
# )

# Tell compiler where to find includes
target_include_directories(image-dsl PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/Passes
    ${GENERATED_DIR}
    ${antlr4_runtime_SOURCE_DIR}/runtime/Cpp/runtime/src
    ${LLVM_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${stb_SOURCE_DIR}
)

llvm_map_components_to_libnames(llvm_libs
    Core            # Core LLVM IR classes
    IRReader        # For parsing .ll / .bc IR files
    Support         # Utilities, command-line, etc.
    TransformUtils  # Transformation helpers
    Analysis        # For passes or IR analysis
    ExecutionEngine # For JIT or running IR in-memory
    Interpreter     # If you plan to interpret IR directly
    MC              # Machine code layer (needed by JIT)
    MCJIT           # JIT engine
    native          # Native target support
    OrcJIT
    Target
    
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")


add_library(runtime SHARED src/ImageRuntime.cpp)
target_include_directories(runtime PUBLIC ${OpenCV_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include ${stb_SOURCE_DIR})
target_link_libraries(runtime PUBLIC ${OpenCV_LIBS})

# Link ANTLR4 runtime
target_link_libraries(image-dsl PRIVATE 
    antlr4_static
    ${llvm_libs}
    ${OpenCV_LIBS}
)
# -----------------------------
# Info
# -----------------------------
message(STATUS "ANTLR grammar: ${GRAMMAR_FILE}")
message(STATUS "Generated sources dir: ${GENERATED_DIR}")
message(STATUS "ANTLR runtime from: ${antlr4_runtime_SOURCE_DIR}")


# -----------------------------
# Suppress warnings, show only errors
# -----------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(image-dsl PRIVATE -w)   # suppress all warnings
elseif(MSVC)
    target_compile_options(image-dsl PRIVATE /W0)  # suppress all warnings
endif()

target_compile_options(image-dsl PRIVATE
    -w             # disable warnings
    -fno-diagnostics-show-caret
    -fno-diagnostics-show-option
)