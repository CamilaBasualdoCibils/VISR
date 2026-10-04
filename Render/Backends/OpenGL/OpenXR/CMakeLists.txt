add_library(VISRRenderOpenGLOpenXR STATIC
    ../OpenGLOpenXRBinding.cpp)
add_library(VISR::RenderOpenGLOpenXR ALIAS VISRRenderOpenGLOpenXR)
target_link_libraries(VISRRenderOpenGLOpenXR PUBLIC
    VISR::OpenXR
    VISR::RenderOpenGL
    OpenXR::headers
    OpenXR::openxr_loader
    OpenGL::EGL
    GLEW::GLEW)
