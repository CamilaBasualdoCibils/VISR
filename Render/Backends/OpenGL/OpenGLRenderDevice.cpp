#include "VISR/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include "VISR/Render/Backends/OpenGL/OpenGLCommandList.hpp"
#include "VISR/Render/Backends/OpenGL/OpenGLCommons.hpp"
#include "VISR/Render/RenderCommons.hpp"
#include <unordered_set>
#include <EGL/eglext.h>

#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>
#endif
static void APIENTRY OpenGLDebugCallback(GLenum, GLenum, GLuint id,
                                         GLenum severity, GLsizei,
                                         const GLchar *message,
                                         const void *userData) {
  auto *logger = static_cast<spdlog::logger *>(const_cast<void *>(userData));
  auto level = spdlog::level::info;
  if (severity == GL_DEBUG_SEVERITY_HIGH)
    level = spdlog::level::err;
  else if (severity == GL_DEBUG_SEVERITY_MEDIUM)
    level = spdlog::level::warn;
  else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    level = spdlog::level::debug;
  logger->log(level, "OpenGL Debug Message [{}]: {}", id, message);
}

static std::unordered_set<std::string> GetGLExtensions() {
  std::unordered_set<std::string> extensions;

  GLint count = 0;
  glGetIntegerv(GL_NUM_EXTENSIONS, &count);

  for (GLint i = 0; i < count; ++i) {
    const char *ext =
        reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, i));

    if (ext)
      extensions.emplace(ext);
  }

  return extensions;
}

static bool HasExtension(const std::unordered_set<std::string> &extensions,
                         std::string_view name) {
  return extensions.contains(std::string{name});
}
VISR::Render::OpenGLRenderDevice::OpenGLRenderDevice(bool useCurrentContext)
    : IRenderDevice() {
  EGLContext sharedContext = EGL_NO_CONTEXT;
  if (useCurrentContext) {
    eglDisplay_ = eglGetCurrentDisplay();
    sharedContext = eglGetCurrentContext();
    if (eglDisplay_ == EGL_NO_DISPLAY || sharedContext == EGL_NO_CONTEXT)
      throw std::runtime_error("OpenGLRenderDevice requires a current context");
    EGLint configId = 0;
    if (eglQueryContext(eglDisplay_, sharedContext, EGL_CONFIG_ID, &configId) !=
        EGL_TRUE)
      throw std::runtime_error("Failed to query the current EGL configuration");
    const EGLint attributes[] = {EGL_CONFIG_ID, configId, EGL_NONE};
    EGLint count = 0;
    if (eglChooseConfig(eglDisplay_, attributes, &eglConfig_, 1, &count) !=
            EGL_TRUE || count == 0)
      throw std::runtime_error("Failed to find the current EGL configuration");
  } else {
    eglDisplay_ = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
                                        EGL_DEFAULT_DISPLAY, nullptr);
    if (eglDisplay_ == EGL_NO_DISPLAY ||
        eglInitialize(eglDisplay_, nullptr, nullptr) != EGL_TRUE)
      throw std::runtime_error("Failed to initialize surfaceless EGL");
    ownsDisplay_ = true;
    if (eglBindAPI(EGL_OPENGL_API) != EGL_TRUE)
      throw std::runtime_error("Failed to bind the EGL OpenGL API");
    const EGLint attributes[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_NONE};
    EGLint count = 0;
    if (eglChooseConfig(eglDisplay_, attributes, &eglConfig_, 1, &count) !=
            EGL_TRUE || count == 0)
      throw std::runtime_error("Failed to choose a surfaceless EGL configuration");
  }

  const EGLint surfaceAttributes[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
  eglSurface_ = eglCreatePbufferSurface(eglDisplay_, eglConfig_, surfaceAttributes);
  if (eglSurface_ == EGL_NO_SURFACE)
    throw std::runtime_error("Failed to create the render-device EGL surface");
  if (eglBindAPI(EGL_OPENGL_API) != EGL_TRUE)
    throw std::runtime_error("Failed to bind the EGL OpenGL API");
  const EGLint contextAttributes[] = {
      EGL_CONTEXT_MAJOR_VERSION_KHR, 4, EGL_CONTEXT_MINOR_VERSION_KHR, 6,
      EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
      EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR, EGL_NONE};
  eglContext_ = eglCreateContext(eglDisplay_, eglConfig_, sharedContext,
                                 contextAttributes);
  if (eglContext_ == EGL_NO_CONTEXT)
    throw std::runtime_error("Failed to create the render-device EGL context");

  ActivateContext();
  glewExperimental = GL_TRUE;
  const GLenum glewError = glewInit();
  if (glewError != GLEW_OK && glewError != GLEW_ERROR_NO_GLX_DISPLAY)
    throw std::runtime_error(
        reinterpret_cast<const char *>(glewGetErrorString(glewError)));
  glGetError();
#if defined(TRACY_ENABLE)
  TracyGpuContext;
#endif
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(OpenGLDebugCallback, logger.get());
  logger->info("OpenGL initialized: version={}, renderer={}, vendor={}",
               reinterpret_cast<const char *>(glGetString(GL_VERSION)),
               reinterpret_cast<const char *>(glGetString(GL_RENDERER)),
               reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
}

VISR::Render::OpenGLRenderDevice::~OpenGLRenderDevice() {
  if (eglDisplay_ == EGL_NO_DISPLAY)
    return;
  if (eglGetCurrentContext() == eglContext_)
    eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  if (eglContext_ != EGL_NO_CONTEXT)
    eglDestroyContext(eglDisplay_, eglContext_);
  if (eglSurface_ != EGL_NO_SURFACE)
    eglDestroySurface(eglDisplay_, eglSurface_);
  if (ownsDisplay_)
    eglTerminate(eglDisplay_);
}

void VISR::Render::OpenGLRenderDevice::ActivateContext() const {
  if (eglGetCurrentContext() == eglContext_)
    return;
  if (eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_) !=
      EGL_TRUE)
    throw std::runtime_error(
        "Failed to activate the render-device EGL context");
}
VISR::Render::ImageHandle
VISR::Render::OpenGLRenderDevice::CreateImage(const ImageDesc &desc) {
  ActivateContext();
  GLTexture glTexture;
  switch (desc.type) {

  case ImageType::Image2D: {
    glCreateTextures(GL_TEXTURE_2D, 1, &glTexture.id);
    std::optional<GLenum> glFormat = OpenGL::GetGLImageFormat(desc.format);
    assert(glFormat.has_value());
    glTextureStorage2D(glTexture.id, 1, glFormat.value(), desc.extent.x,
                       desc.extent.y);
    glTextureParameteri(glTexture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(glTexture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(glTexture.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(glTexture.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (!desc.initialData.empty()) {
      const bool singleChannel = desc.format == ImageFormat::R8_UNORM;
      const std::size_t bytesPerPixel = singleChannel ? 1U : 4U;
      const std::size_t expected = static_cast<std::size_t>(desc.extent.x) *
                                   desc.extent.y * bytesPerPixel;
      if (desc.initialData.size_bytes() < expected)
        throw std::invalid_argument("initial image data is too small");
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTextureSubImage2D(glTexture.id, 0, 0, 0, desc.extent.x,
                          desc.extent.y, singleChannel ? GL_RED : GL_RGBA,
                          GL_UNSIGNED_BYTE, desc.initialData.data());
      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
    ImageHandle handle = GenerateTextureHandle();
    textures[handle] = glTexture;
    return handle;
  } break;
  case ImageType::Image1D:
  case ImageType::Image3D:
  case ImageType::Image1DArray:
  case ImageType::Image2DArray:
  case ImageType::Cube:
  case ImageType::CubeArray:
  default: {
    throw std::runtime_error("Unsupported image type.");
  } break;
  }
  return ImageHandle(-1);
}
VISR::Render::BufferHandle
VISR::Render::OpenGLRenderDevice::CreateBuffer(const BufferDesc &desc) {
  ActivateContext();
  GLBuffer glBuffer;
  glCreateBuffers(1, &glBuffer.id);
  if (desc.initialData.size_bytes() > desc.size)
    throw std::invalid_argument("initial buffer data exceeds buffer size");
  glNamedBufferStorage(
      glBuffer.id, desc.size,
      desc.initialData.empty() ? nullptr : desc.initialData.data(), 0);
  BufferHandle handle = GenerateBufferHandle();
  buffers[handle] = glBuffer;
  return handle;
}
VISR::Render::GraphicsPipelineHandle
VISR::Render::OpenGLRenderDevice::CreatePipeline(
    const GraphicsPipelineDesc &graphicsDesc) {
  ActivateContext();
  GLuint program = glCreateProgram();
  glAttachShader(program, graphicsDesc.vertexShader.value);
  glAttachShader(program, graphicsDesc.fragmentShader.value);
  glLinkProgram(program);
  glValidateProgram(program);
  bool linked = false;
  glGetProgramiv(program, GL_LINK_STATUS, reinterpret_cast<GLint *>(&linked));
  if (!linked) {
    GLint infoLogLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
      std::vector<GLchar> infoLog(infoLogLength);
      glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog.data());
      logger->error("Program link error: {}", infoLog.data());

      throw std::runtime_error("Failed to link OpenGL program. This should "
                               "be replaced with a default error shader");
    }
  }

  GLuint vao;
  glCreateVertexArrays(1, &vao);
  for (const auto &binding : graphicsDesc.vertexLayout.bindings) {
    glVertexArrayBindingDivisor(vao, binding.binding,
                                binding.perInstance ? 1 : 0);
  }

  for (const auto &attr : graphicsDesc.vertexLayout.attributes) {
    const auto glFormatOpt = OpenGL::GetGLVertexFormat(attr.format);
    assert(glFormatOpt.has_value());
    const auto glFormat = glFormatOpt.value();

    glEnableVertexArrayAttrib(vao, attr.location);

    glVertexArrayAttribFormat(vao, attr.location, glFormat.componentCount,
                              glFormat.type, glFormat.normalized, attr.offset);

    glVertexArrayAttribBinding(vao, attr.location, attr.binding);
  }

  glBindVertexArray(0);
  GraphicsPipelineHandle handle = GeneratePipelineHandle();
  GLPipeline glPipeline{
      .programId = program,
      .vaoId = vao,
      .blending = !graphicsDesc.blend.attachments.empty() &&
                  graphicsDesc.blend.attachments.front().enabled,
  };
  pipelines[handle] = glPipeline;
  return handle;
}
VISR::Render::ShaderModuleHandle
VISR::Render::OpenGLRenderDevice::CreateShaderModule(
    const ShaderModuleDesc &desc) {
  ActivateContext();
  const auto glShaderType = OpenGL::GetGLShaderStage(desc.stage);
  assert(glShaderType.has_value());

  GLuint shader = glCreateShader(*glShaderType);

  glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, desc.spirv.data(),
                 static_cast<GLsizei>(desc.spirv.size()));

  // SPIR-V must be specialized before it is considered compiled.
  glSpecializeShader(shader,
                     "main", // SPIR-V entry point
                     0, nullptr, nullptr);

  GLint compiled = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

  if (compiled != GL_TRUE) {
    GLint infoLogLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (infoLogLength > 0) {
      std::vector<GLchar> infoLog(infoLogLength);
      glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog.data());

      logger->error("Shader compile error: {}", infoLog.data());
    }

    glDeleteShader(shader);
    throw std::runtime_error("Failed to compile OpenGL SPIR-V shader.");
  }

  const ShaderModuleHandle handle = GenerateShaderModuleHandle();
  shaderModules[handle] = GLShaderModule{.id = shader};

  return handle;
}
void VISR::Render::OpenGLRenderDevice::Destroy(GraphicsPipelineHandle handle) {
  ActivateContext();

  glDeleteProgram(pipelines[handle].programId);
  glDeleteVertexArrays(1, &pipelines[handle].vaoId);
  pipelines.erase(handle);
}
void VISR::Render::OpenGLRenderDevice::Destroy(ShaderModuleHandle handle) {
  ActivateContext();

  glDeleteShader(shaderModules[handle].id);
  shaderModules.erase(handle);
}
void VISR::Render::OpenGLRenderDevice::Destroy(ImageHandle handle) {
  ActivateContext();
  glDeleteTextures(1, &textures[handle].id);
  textures.erase(handle);
}
void VISR::Render::OpenGLRenderDevice::Destroy(BufferHandle handle) {
  ActivateContext();
  glDeleteBuffers(1, &buffers.at(handle).id);
  buffers.erase(handle);
}
std::unique_ptr<VISR::Render::IRenderCommandList>
VISR::Render::OpenGLRenderDevice::CreateCommandList(QueueType type) {
  return std::make_unique<OpenGLCommandList>(this);
}
void VISR::Render::OpenGLRenderDevice::Submit(
    const IRenderCommandList &commandList) {
#if defined(TRACY_ENABLE)
  ZoneScopedN("OpenGL Command Submission");
#endif
  ActivateContext();
  const OpenGLCommandList &glCommandList =
      dynamic_cast<const OpenGLCommandList &>(commandList);
  auto commandQueue = glCommandList.GetCommandQueue();
  for (auto &command : commandQueue) {
    std::visit([this](auto &&cmd) { cmd.Execute(this); }, command);
  }
#if defined(TRACY_ENABLE)
  TracyGpuCollect;
#endif
}

VISR::Render::RenderCapabilities
VISR::Render::OpenGLRenderDevice::GetGLCapabilities() {
  const auto extensions = GetGLExtensions();

  RenderCapabilities caps{};

  caps.multiView = HasExtension(extensions, "GL_OVR_multiview") ||
                   HasExtension(extensions, "GL_OVR_multiview2");

  caps.externalMemory = HasExtension(extensions, "GL_EXT_memory_object");

  caps.externalSemaphore = HasExtension(extensions, "GL_EXT_semaphore");

  caps.fragmentDensityMap = false; // No direct standard OpenGL equivalent.

  caps.variableRateShading =
      HasExtension(extensions, "GL_NV_shading_rate_image");

  caps.dmaBuf =
      false; // This is normally an EGL capability, not a GL capability.

  return caps;
}
