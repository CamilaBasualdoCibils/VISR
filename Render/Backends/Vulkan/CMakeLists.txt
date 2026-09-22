add_library(ARUIRenderVulkan STATIC VulkanRenderDevice.cpp)
add_library(ARUI::RenderVulkan ALIAS ARUIRenderVulkan)
target_link_libraries(ARUIRenderVulkan PUBLIC ARUI::Render ARUI::XR spdlog::spdlog)
