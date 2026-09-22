add_library(ARUIRenderOpenGL STATIC OpenGLCommandList.cpp OpenGLRenderDevice.cpp)
add_library(ARUI::RenderOpenGL ALIAS ARUIRenderOpenGL)
target_link_libraries(ARUIRenderOpenGL PUBLIC ARUI::Render GLEW::GLEW glfw spdlog::spdlog)
