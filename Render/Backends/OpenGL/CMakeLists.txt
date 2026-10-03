add_library(ARUIRenderOpenGL STATIC OpenGLCommandList.cpp OpenGLRenderDevice.cpp)
add_library(ARUI::RenderOpenGL ALIAS ARUIRenderOpenGL)
target_link_libraries(ARUIRenderOpenGL PUBLIC ARUI::Render ARUI::Instrumentation GLEW::GLEW OpenGL::EGL spdlog::spdlog)
