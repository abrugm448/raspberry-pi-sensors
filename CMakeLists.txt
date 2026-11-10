# CMakeList.txt : CMake project for kiosk-sensors, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("kiosk-sensors")

# libs
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiodcxx>=2.0)
pkg_check_modules(MOSQUITTO REQUIRED IMPORTED_TARGET libmosquitto)

# Add source to this project's executable.
add_executable (kiosk-sensors "kiosk-sensors.cpp"
                              "kaku_transmitter.cpp" "kaku_transmitter.h"
                              "mqtt_client.cpp" "mqtt_client.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET kiosk-sensors PROPERTY CXX_STANDARD 20)
endif()

# link libs to program
target_link_libraries(kiosk-sensors PRIVATE PkgConfig::GPIOD PkgConfig::MOSQUITTO)

# compile warnings
target_compile_options(kiosk-sensors PRIVATE -Wall -Wextra)
