add_library(VISRRenderVulkan STATIC VulkanRenderDevice.cpp)
add_library(VISR::RenderVulkan ALIAS VISRRenderVulkan)
target_link_libraries(VISRRenderVulkan PUBLIC VISR::Render VISR::XR spdlog::spdlog)
