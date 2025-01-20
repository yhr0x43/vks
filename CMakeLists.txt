set( PROJECT_NAME vks )

project( ${PROJECT_NAME} )

file( GLOB VKS_SRCS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" )

add_library( ${PROJECT_NAME} STATIC "${VKS_SRCS}" )
set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20 )
target_include_directories( ${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include" )

if(WIN32)
    target_link_directories( ${PROJECT_NAME} PRIVATE "$ENV{VK_SDK_PATH}/Lib" )
    target_include_directories( ${PROJECT_NAME} PRIVATE "$ENV{VK_SDK_PATH}/Include" )
    target_link_libraries( ${PROJECT_NAME} vulkan-1 spdlog::spdlog )
else()
    target_link_libraries( ${PROJECT_NAME} ${Vulkan_LIBRARY} )
endif()