# $Id: CMakeLists.txt 100813 2016-11-02 15:09:50Z gcosmo $

#----------------------------------------------------------------------------
# Setup the project
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(G4ants)

#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# You can set WITH_GEANT4_UIVIS to OFF via the command line or ccmake/cmake-gui
# to build a batch mode only executable
#
option(WITH_GEANT4_UIVIS "Build example with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
# Setup include directory for this project
#
include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/include)

execute_process (
    COMMAND geant4-config --version
    OUTPUT_VARIABLE Geant4_VERSION
)
message(${Geant4_VERSION})
string(REPLACE "." ";" Geant4_LIST ${Geant4_VERSION})
list(GET Geant4_LIST 0 Geant4_VERSION_MAJOR)
message(${Geant4_VERSION_MAJOR})
if (Geant4_VERSION_MAJOR GREATER 10)
    message("Geant4's version is 11 or larger")
    add_compile_definitions(GEANT_VERSION_FROM_11)
    endif()

#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(G4ants G4ants.cc ${sources} ${headers})
target_link_libraries(G4ants ${Geant4_LIBRARIES})

#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build B2a. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
set(EXTRA_FILES
  vis.mac
  )

foreach(_file ${EXTRA_FILES})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_file}
    ${PROJECT_BINARY_DIR}/${_file}
    COPYONLY
    )
endforeach()

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
install(TARGETS G4ants DESTINATION bin)

