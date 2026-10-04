find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(VisrManualPresenter src/main.cpp)
target_link_libraries(VisrManualPresenter PRIVATE
    VISR::Language
    VISR::Presentation
    glfw
    imgui::imgui
    OpenGL::GL)
visr_enable_runtime_instrumentation(VisrManualPresenter)
