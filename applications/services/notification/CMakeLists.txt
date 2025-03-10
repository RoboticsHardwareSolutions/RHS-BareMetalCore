cmake_minimum_required(VERSION 3.24)
project(notification C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC notification_app.c notification_app_api.c notification_messages_notes.c notification_messages.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
        ${PROJECT_NAME}

        PRIVATE
        rhs
        rhs_hal
)

service(notification_srv "notification_srv" 1024)
