cmake_minimum_required(VERSION 3.16)

project(RadarVisualizationApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 first, fallback to Qt5
find_package(Qt6 QUIET COMPONENTS Core Widgets Network Charts)
if(NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network Charts)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

# Enable automatic MOC, UIC, and RCC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Source files
set(SOURCES
    main.cpp
    MainWindow.cpp
    PPIPlot.cpp
    FFTPlot.cpp
    UDPReceiver.cpp
)

set(HEADERS
    MainWindow.h
    PPIPlot.h
    FFTPlot.h
    UDPReceiver.h
    DataStructures.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Qt libraries
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(${PROJECT_NAME} 
        Qt6::Core 
        Qt6::Widgets 
        Qt6::Network 
        Qt6::Charts
    )
else()
    target_link_libraries(${PROJECT_NAME} 
        Qt5::Core 
        Qt5::Widgets 
        Qt5::Network 
        Qt5::Charts
    )
endif()

# Enable debug information in debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
endif()

# Platform-specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()