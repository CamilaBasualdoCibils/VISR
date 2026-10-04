add_library(VISRRenderOpenGL STATIC OpenGLCommandList.cpp OpenGLRenderDevice.cpp)
add_library(VISR::RenderOpenGL ALIAS VISRRenderOpenGL)
target_link_libraries(VISRRenderOpenGL PUBLIC VISR::Render VISR::Instrumentation GLEW::GLEW OpenGL::EGL spdlog::spdlog)
