cmake_minimum_required(VERSION 3.15)
project(CameraServer VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find System Dependencies
find_package(PkgConfig REQUIRED)

# Use IMPORTED_TARGET to automatically handle both includes and libraries
pkg_check_modules(GPHOTO2 REQUIRED IMPORTED_TARGET libgphoto2)
pkg_check_modules(CUPS REQUIRED IMPORTED_TARGET cups)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

include(FetchContent)

# 1. Fetch Crow (Consider pinning a specific tag instead of master)
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        master 
)
FetchContent_MakeAvailable(Crow)

# 2. Fetch Nlohmann JSON
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.2
)
FetchContent_MakeAvailable(json)

# 3. Fetch CPR (HTTP Client)
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG        1.10.4 
)
set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "" FORCE) 
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)     
FetchContent_MakeAvailable(cpr)

# 4. Build Executable
add_executable(camera_server 
    src/main.cpp
    src/CameraClient.cpp
)

# 5. Include Paths
target_include_directories(camera_server PRIVATE 
    include
    ${OpenCV_INCLUDE_DIRS}
)

# 6. Link Libraries
target_link_libraries(camera_server PRIVATE 
    PkgConfig::GPHOTO2
    PkgConfig::CUPS
    ${OpenCV_LIBS}
    Threads::Threads
    Crow::Crow 
    cpr::cpr 
    nlohmann_json::nlohmann_json
)