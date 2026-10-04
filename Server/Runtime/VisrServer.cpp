#include "VisrServer.hpp"
#include "VISR/Presentation/PresentationRpcServer.hpp"
#include "VISR/Presentation/RuntimePresentationController.hpp"
#include "VISR/Render/Backends/OpenGL/OpenGLOpenXRBinding.hpp"
#include "VISR/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include "VISR/Render/Renderer.hpp"
#include "VISR/Render/StandardPipeline.hpp"
#include "VISR/Runtime/IPainter.hpp"
#include "VISR/Runtime/Painters/VisrFlatPainter.hpp"
#include "VISR/Runtime/RuntimePainter.hpp"
#include "VISR/Runtime/RuntimeTree.hpp"
#include "VISR/XR/OpenXR/OpenXRPresenter.hpp"
#include "VISR/XR/OpenXR/OpenXRTrackingProvider.hpp"
#include "VISR/XR/OpenXR/OpenXRViewProvider.hpp"

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <mutex>
#include <optional>
#include <span>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace {

float Meters(const std::optional<VISR::Language::Length> &length) {
  if (!length || !length->IsAbsolute())
    return 0.0F;
  return static_cast<float>(
      length->As(VISR::Language::LengthUnit::Meter).Value());
}

float Radians(const std::optional<VISR::Language::Angle> &angle) {
  if (!angle)
    return 0.0F;
  const float value = static_cast<float>(angle->value);
  return angle->unit == VISR::Language::AngleUnit::Degree ? glm::radians(value)
                                                          : value;
}

glm::mat4 SurfaceLocalToWorld(const VISR::Runtime::RNode &surface,
                              VISR::IXRTracker &tracker) {
  VISR::Pose anchor;
  const auto found = surface.attributes.find("anchor");
  const auto *name = found == surface.attributes.end()
                         ? nullptr
                         : std::get_if<std::string>(&found->second);
  if (name && *name != "world")
    if (const auto tracked = tracker.GetPose(*name))
      anchor = tracked->pose;

  const glm::vec3 offset{Meters(surface.style.xOffset),
                         Meters(surface.style.yOffset),
                         Meters(surface.style.zOffset)};
  const glm::quat rotation =
      glm::normalize(anchor.orientation *
                     glm::quat(glm::vec3{Radians(surface.style.xRotation),
                                         Radians(surface.style.yRotation),
                                         Radians(surface.style.zRotation)}));
  const glm::vec3 position = anchor.position + anchor.orientation * offset;
  return glm::translate(glm::mat4{1.0F}, position) * glm::mat4_cast(rotation);
}

class StereoStream {
public:
  StereoStream() {
    listener_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listener_ < 0)
      return;
    const int reuse = 1;
    setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(4242);
    if (bind(listener_, reinterpret_cast<sockaddr *>(&address),
             sizeof(address)) != 0 ||
        listen(listener_, 1) != 0) {
      close(listener_);
      listener_ = -1;
      return;
    }
    fcntl(listener_, F_SETFL, fcntl(listener_, F_GETFL) | O_NONBLOCK);
  }
  ~StereoStream() {
    if (client_ >= 0)
      close(client_);
    if (listener_ >= 0)
      close(listener_);
  }
  void Publish(std::uint8_t eye, std::uint32_t width, std::uint32_t height,
               GLuint texture) {
    Accept();
    if (client_ < 0)
      return;
    const std::size_t size = static_cast<std::size_t>(width) * height * 4U;
    pixels_.resize(size);
    glGetTextureImage(texture, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                      static_cast<GLsizei>(size), pixels_.data());
    std::array<std::uint8_t, 20> header{'A', 'R', 'X', 'R', eye};
    WriteU32(header.data() + 8, width);
    WriteU32(header.data() + 12, height);
    WriteU32(header.data() + 16, static_cast<std::uint32_t>(size));
    if (!Send(header) || !Send(pixels_)) {
      close(client_);
      client_ = -1;
    }
  }

private:
  void Accept() {
    if (client_ >= 0 || listener_ < 0)
      return;
    client_ = accept(listener_, nullptr, nullptr);
  }
  static void WriteU32(std::uint8_t *bytes, std::uint32_t value) {
    value = htonl(value);
    std::memcpy(bytes, &value, sizeof(value));
  }
  bool Send(std::span<const std::uint8_t> bytes) {
    std::size_t sent = 0;
    while (sent < bytes.size()) {
      const auto count =
          send(client_, bytes.data() + sent, bytes.size() - sent, MSG_NOSIGNAL);
      if (count <= 0)
        return false;
      sent += static_cast<std::size_t>(count);
    }
    return true;
  }
  int listener_{-1};
  int client_{-1};
  std::vector<std::uint8_t> pixels_;
};
} // namespace

int VISR::Server::VisrServer::Run() {
  auto openGL = std::make_shared<Render::OpenGLRenderDevice>();
  auto graphics = std::make_shared<Render::OpenGLOpenXRBinding>(openGL);
  auto openXRPresenter = std::make_shared<OpenXR::OpenXRPresenter>(graphics);
  auto openXRRuntime = openXRPresenter->Runtime();
  auto openXRViews =
      std::make_shared<OpenXR::OpenXRViewProvider>(openXRRuntime);
  presenter = openXRPresenter;
  viewProvider = openXRViews;
  tracker = openXRRuntime;
  environment = openXRRuntime;

  renderDevice = openGL;

  std::array<Render::ImageHandle, 2> images;
  std::array<Render::ImageViewHandle, 2> imageViews;
  std::array<glm::uvec2, 2> extents;
  for (std::size_t eye = 0; eye < 2; ++eye) {
    extents[eye] = glm::uvec2{openXRRuntime->GetRecommendedExtent(eye)};
    images[eye] = renderDevice->CreateImage(
        {.extent = {extents[eye], 1},
         .format = Render::ImageFormat::R8G8B8A8_UNORM,
         .usage = static_cast<Render::ImageUsage>(
                      Render::ImageUsageFlags::ColorAttachment) |
                  static_cast<Render::ImageUsage>(
                      Render::ImageUsageFlags::Sampled)});
    imageViews[eye] = renderDevice->CreateImageView(
        {.image = images[eye], .format = Render::ImageFormat::R8G8B8A8_UNORM});
  }

  Render::StandardPipeline standardPipeline{*renderDevice};
  Runtime::PainterRegistry painters;
  Runtime::RegisterVisrFlatPainter(painters);
  Runtime::RuntimeTree runtime;
  std::mutex runtimeMutex;
  Presentation::RuntimePresentationController presentation(runtime,
                                                           runtimeMutex);
  Presentation::PresentationRpcServer rpcServer(presentation);
  rpcServer.Start();
  StereoStream stereoStream;

  while (!stopRequested && !openXRPresenter->ShouldExit()) {
    presenter->BeginFrame();
    viewProvider->BeginFrame();
    const auto views = viewProvider->GetViews();
    for (std::size_t eye = 0; eye < views.size(); ++eye) {
      Render::Renderer renderer(
          *renderDevice,
          standardPipeline.Configuration(
              {.colorAttachment = imageViews[eye],
               .clearColor = true,
               .clearColorValue = environment->IsPassthroughEnabled()
                                      ? glm::vec4{0.0F}
                                      : glm::vec4{0.08F, 0.09F, 0.12F, 1.0F},
               .extent = extents[eye],
               .offset = {0, 0}},
              views[eye].projection * views[eye].view));
      Render::RenderGraph graph;
      renderer.BeginFrame();
      Runtime::PaintContext paintContext{renderer};
      {
        const std::scoped_lock lock(runtimeMutex);
        for (const Runtime::NodeID surface : runtime.RootChildren()) {
          const auto *node = runtime.Get(surface);
          if (!node)
            continue;
          Runtime::PaintRuntimeSurface(
              paintContext, painters, runtime, surface,
              {.localToWorld = SurfaceLocalToWorld(*node, *tracker)});
        }
      }
      renderer.BuildRenderGraph(graph);
      graph.Compile();
      graph.Execute(*renderDevice);
      renderer.EndFrame();
      stereoStream.Publish(static_cast<std::uint8_t>(eye), extents[eye].x,
                           extents[eye].y,
                           openGL->GetGLTextureView(imageViews[eye]).id);
      presenter->Present(views[eye], imageViews[eye]);
    }
    viewProvider->EndFrame();
    presenter->EndFrame();
  }

  for (std::size_t eye = 0; eye < 2; ++eye) {
    renderDevice->Destroy(imageViews[eye]);
    renderDevice->Destroy(images[eye]);
  }
  return 0;
}
