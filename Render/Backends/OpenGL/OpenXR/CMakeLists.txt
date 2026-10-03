add_library(ARUIRenderOpenGLOpenXR STATIC
    ../OpenGLOpenXRBinding.cpp)
add_library(ARUI::RenderOpenGLOpenXR ALIAS ARUIRenderOpenGLOpenXR)
target_link_libraries(ARUIRenderOpenGLOpenXR PUBLIC
    ARUI::OpenXR
    ARUI::RenderOpenGL
    OpenXR::headers
    OpenXR::openxr_loader
    OpenGL::EGL
    GLEW::GLEW)
