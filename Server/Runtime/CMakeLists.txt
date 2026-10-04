add_executable(VisrServer main.cpp VisrServer.cpp)
target_link_libraries(VisrServer PRIVATE
    VISR::PresentationServer
    VISR::RenderOpenGL
    VISR::OpenXR
    VISR::RenderOpenGLOpenXR
    Boost::program_options)
visr_enable_runtime_instrumentation(VisrServer)
