cmake_minimum_required(VERSION 3.21)
project(CVTrackingSystem VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Dependencies ----
find_package(OpenCV REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# ---- Sources ----
set(SOURCES
    src/main.cpp
    src/capture/VideoCapture.cpp
    src/detection/Detector.cpp
    src/tracking/KalmanTracker.cpp
    src/tracking/TrackManager.cpp
    src/threat/ThreatScorer.cpp
    src/render/Renderer.cpp
    src/render/ShaderProgram.cpp
    src/render/VideoTexture.cpp
    src/render/OverlayRenderer.cpp
    src/render/HudPanel.cpp
)

add_executable(CVTrackingSystem ${SOURCES})

target_include_directories(CVTrackingSystem PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(CVTrackingSystem PRIVATE
    ${OpenCV_LIBS}
    glfw
    glad::glad
    glm::glm
    imgui::imgui
    opengl32
)

# ---- Post-build: copy shaders and DLLs ----
add_custom_command(TARGET CVTrackingSystem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:CVTrackingSystem>/shaders"
    COMMENT "Copying shaders to output directory"
)

# Copy runtime DLLs (OpenCV + other vcpkg DLLs) on Windows
if(WIN32)
    add_custom_command(TARGET CVTrackingSystem POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:CVTrackingSystem>
            $<TARGET_FILE_DIR:CVTrackingSystem>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLLs"
    )
endif()
