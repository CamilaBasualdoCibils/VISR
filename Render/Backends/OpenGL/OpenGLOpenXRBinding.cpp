#include "VISR/Render/Backends/OpenGL/OpenGLOpenXRBinding.hpp"
#include "VISR/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include <EGL/egl.h>
#include <GL/glew.h>
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_PLATFORM_EGL
#include <openxr/openxr_platform.h>
#include <algorithm>
#include <array>
#include <vector>

namespace VISR::Render {
namespace {
PFN_xrVoidFunction GetEGLProcAddress(const char *name) {
  return reinterpret_cast<PFN_xrVoidFunction>(eglGetProcAddress(name));
}
}
struct OpenGLOpenXRBinding::State {
  std::array<std::vector<XrSwapchainImageOpenGLKHR>, 2> colorImages;
  std::vector<XrSwapchainImageOpenGLKHR> depthImages;
};
OpenGLOpenXRBinding::OpenGLOpenXRBinding(
    std::shared_ptr<OpenGLRenderDevice> device)
    : device_(std::move(device)), state_(std::make_unique<State>()) {
  if (!device_)
    throw std::invalid_argument("OpenGLOpenXRBinding requires a render device");
}
OpenGLOpenXRBinding::~OpenGLOpenXRBinding() = default;
std::vector<const char *>
OpenGLOpenXRBinding::RequiredInstanceExtensions() const {
  return {XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
          XR_MNDX_EGL_ENABLE_EXTENSION_NAME};
}
XrResult OpenGLOpenXRBinding::CreateSession(XrInstance instance,
                                             XrSystemId system,
                                             XrSession *session) {
  device_->MakeCurrent();
  PFN_xrGetOpenGLGraphicsRequirementsKHR getRequirements{};
  XrResult result = xrGetInstanceProcAddr(
      instance, "xrGetOpenGLGraphicsRequirementsKHR",
      reinterpret_cast<PFN_xrVoidFunction *>(&getRequirements));
  if (result != XR_SUCCESS || !getRequirements)
    return result == XR_SUCCESS ? XR_ERROR_FUNCTION_UNSUPPORTED : result;
  XrGraphicsRequirementsOpenGLKHR requirements{
      XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
  result = getRequirements(instance, system, &requirements);
  if (result != XR_SUCCESS)
    return result;
  GLint major = 0, minor = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  const XrVersion version = XR_MAKE_VERSION(major, minor, 0);
  if (version < requirements.minApiVersionSupported ||
      version > requirements.maxApiVersionSupported)
    return XR_ERROR_GRAPHICS_DEVICE_INVALID;
  XrGraphicsBindingEGLMNDX binding{XR_TYPE_GRAPHICS_BINDING_EGL_MNDX};
  binding.getProcAddress = GetEGLProcAddress;
  binding.display = device_->EGLDisplayHandle();
  binding.config = device_->EGLConfigHandle();
  binding.context = device_->EGLContextHandle();
  XrSessionCreateInfo info{XR_TYPE_SESSION_CREATE_INFO};
  info.next = &binding;
  info.systemId = system;
  return xrCreateSession(instance, &info, session);
}
int64_t OpenGLOpenXRBinding::SelectColorFormat(
    std::span<const int64_t> formats) const {
  for (const int64_t preferred : {int64_t{GL_RGBA8}, int64_t{GL_SRGB8_ALPHA8}})
    if (std::find(formats.begin(), formats.end(), preferred) != formats.end())
      return preferred;
  return formats.empty() ? 0 : formats.front();
}
bool OpenGLOpenXRBinding::EnumerateSwapchainImages(XrSwapchain swapchain,
                                                     std::size_t view) {
  if (view >= state_->colorImages.size()) return false;
  uint32_t count = 0;
  if (xrEnumerateSwapchainImages(swapchain, 0, &count, nullptr) != XR_SUCCESS || !count)
    return false;
  auto &images = state_->colorImages[view];
  images.resize(count);
  for (auto &image : images) image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
  return xrEnumerateSwapchainImages(swapchain, count, &count,
      reinterpret_cast<XrSwapchainImageBaseHeader *>(images.data())) == XR_SUCCESS;
}
bool OpenGLOpenXRBinding::EnumerateDepthImages(
    PFN_xrEnumerateEnvironmentDepthSwapchainImagesMETA enumerate,
    XrEnvironmentDepthSwapchainMETA swapchain) {
  uint32_t count = 0;
  if (enumerate(swapchain, 0, &count, nullptr) != XR_SUCCESS || !count)
    return false;
  state_->depthImages.resize(count);
  for (auto &image : state_->depthImages)
    image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
  return enumerate(swapchain, count, &count,
      reinterpret_cast<XrSwapchainImageBaseHeader *>(
          state_->depthImages.data())) == XR_SUCCESS;
}
std::pair<XRDepthImageBackend, std::uint64_t>
OpenGLOpenXRBinding::DepthImage(std::uint32_t index) const {
  if (index >= state_->depthImages.size()) return {};
  return {XRDepthImageBackend::OpenGL, state_->depthImages[index].image};
}
bool OpenGLOpenXRBinding::CopyRenderTarget(
    std::size_t view, std::uint32_t imageIndex, ImageViewHandle source,
    std::uint32_t width, std::uint32_t height) {
  if (view >= state_->colorImages.size() ||
      imageIndex >= state_->colorImages[view].size()) return false;
  device_->MakeCurrent();
  GLuint read = 0, draw = 0;
  glCreateFramebuffers(1, &read);
  glCreateFramebuffers(1, &draw);
  glNamedFramebufferTexture(read, GL_COLOR_ATTACHMENT0,
                            device_->GetGLTextureView(source).id, 0);
  glNamedFramebufferTexture(draw, GL_COLOR_ATTACHMENT0,
                            state_->colorImages[view][imageIndex].image, 0);
  const bool complete =
      glCheckNamedFramebufferStatus(read, GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE &&
      glCheckNamedFramebufferStatus(draw, GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  if (complete)
    glBlitNamedFramebuffer(read, draw, 0, 0, width, height, 0, 0, width, height,
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glDeleteFramebuffers(1, &read);
  glDeleteFramebuffers(1, &draw);
  glFlush();
  return complete;
}
} // namespace VISR::Render
