#include "VISR/Render/Backends/Vulkan/VulkanRenderDevice.hpp"
#include "VISR/XR/Display/IPresenter.hpp"
#include "vulkan/vulkan.hpp"
#include <spdlog/common.h>
#include <unordered_set>

static bool FindExtension(const char *extensionName,
                          std::span<const vk::ExtensionProperties> extensions) {
  for (const auto &extension : extensions) {
    if (strcmp(extension.extensionName.data(), extensionName) == 0) {
      return true;
    }
  }
  return false;
}
VISR::Render::VulkanRenderDevice::VulkanRenderDevice(
    std::shared_ptr<IPresenter> presenter) 
{

  std::vector<std::string> essentialExtensions =
      presenter->GetRequiredVulkanExtensions();
  const auto extensions = vk::enumerateInstanceExtensionProperties();
  std::unordered_set<std::string> requestedExtensions;
  auto PrintFound = [&](const char *extensionName, bool found) {
    if (found)
      logger->info("Extension {} {}", extensionName,
                   found ? "FOUND" : "NOT FOUND");
  };
  bool allEssentialExtensionsFound = true;
  logger->info("Checking for essential Vulkan extensions...");
  for (const auto &extensionName : essentialExtensions) {
    bool found = FindExtension(extensionName.c_str(), std::span(extensions));
    if (!found)
      allEssentialExtensionsFound = false;
    PrintFound(extensionName.c_str(), found);
  }
  if (!allEssentialExtensionsFound) {
    throw std::runtime_error("Not all essential Vulkan extensions were found.");
  }

  vk::ApplicationInfo appInfo;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_4;
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "VISR Engine";

  vk::InstanceCreateInfo createInfo{};
  createInfo.pApplicationInfo = &appInfo;

  // Create the Vulkan instance
  instance = vk::createInstance(createInfo);
}
