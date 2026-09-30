#pragma once

#include "ARUI/Render/RenderCommons.hpp"
#include "ARUI/XR/Display/RenderView.hpp"
#include <string>
#include <vector>
namespace ARUI {
class IPresenter {
public:
  virtual ~IPresenter() = default;

  virtual void BeginFrame() = 0;

  virtual void Present(const RenderView &view,
                       Render::ImageViewHandle image) = 0;

  virtual void EndFrame() = 0;

  virtual std::vector<std::string> GetRequiredVulkanExtensions() const = 0;
};
} // namespace ARUI
