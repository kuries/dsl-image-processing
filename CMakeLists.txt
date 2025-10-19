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

# Make executable depend on ANTLR generation
add_dependencies(image-dsl GenerateGrammar)

# Tell compiler where to find includes
target_include_directories(image-dsl PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${GENERATED_DIR}
    ${antlr4_runtime_SOURCE_DIR}/runtime/Cpp/runtime/src
)

# Link ANTLR4 runtime
target_link_libraries(image-dsl PRIVATE antlr4_static)
# -----------------------------
# Info
# -----------------------------
message(STATUS "ANTLR grammar: ${GRAMMAR_FILE}")
message(STATUS "Generated sources dir: ${GENERATED_DIR}")
message(STATUS "ANTLR runtime from: ${antlr4_runtime_SOURCE_DIR}")
