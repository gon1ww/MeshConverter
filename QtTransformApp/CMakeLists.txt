# ==============================================================================
# Qt Transform Application CMake Configuration
# ==============================================================================
cmake_minimum_required(VERSION 3.20)
project(QtTransformApp VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 跨平台配置
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_compile_options(/utf-8)
endif()

# 输出目录设置
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Qt 配置
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Concurrent)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 为VTK添加Qt6::OpenGL别名（因为VTK可能还在使用旧的Qt5命名方式）
if(TARGET Qt6::OpenGLWidgets AND NOT TARGET Qt6::OpenGL)
    add_library(Qt6::OpenGL ALIAS Qt6::OpenGLWidgets)
endif()

# 设置UI文件搜索路径
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/view)

# VTK 配置
find_package(VTK REQUIRED COMPONENTS
    CommonCore
    CommonDataModel
    FiltersCore
    FiltersGeneral
    FiltersGeometry
    FiltersSources
    IOGeometry
    IOLegacy
    IOXML
    OPTIONAL_COMPONENTS IOCGNS
)

# 检查VTK是否包含Gmsh支持
set(HAS_VTK_GMSH OFF)
foreach(dir IN LISTS VTK_INCLUDE_DIRS)
    if (EXISTS "${dir}/vtkGmshReader.h")
        set(HAS_VTK_GMSH ON)
        break()
    endif()
endforeach()

# 源文件
set(SOURCES
    src/main.cpp
    src/transform.cpp
)

# 头文件
set(HEADERS
    include/transform.h
)

# UI文件
set(UIS
    view/transform.ui
)

# 创建可执行程序
add_executable(QtTransformApp ${SOURCES} ${HEADERS} ${UIS})

# 链接依赖
target_link_libraries(QtTransformApp PRIVATE 
    Qt6::Widgets 
    Qt6::Concurrent
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::FiltersCore
    VTK::FiltersGeneral
    VTK::FiltersGeometry
    VTK::FiltersSources
    VTK::IOGeometry
    VTK::IOLegacy
    VTK::IOXML
)

# 检查并链接CGNS支持
if (TARGET VTK::IOCGNS)
    target_link_libraries(QtTransformApp PRIVATE VTK::IOCGNS)
    target_compile_definitions(QtTransformApp PRIVATE HAS_VTK_IOCGNS)
else()
    message(STATUS "VTK::IOCGNS not found, CGNS import will be disabled.")
endif()

# 检查并定义Gmsh支持
if (HAS_VTK_GMSH)
    target_compile_definitions(QtTransformApp PRIVATE HAS_VTK_GMSH)
else()
    message(STATUS "vtkGmshReader.h not found, Gmsh import will be disabled.")
endif()

# 移除VTK模块自动初始化，因为我们不再使用VTK的Qt相关模块

# 包含目录
target_include_directories(QtTransformApp PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${VTK_INCLUDE_DIRS}
)
