#include "ARUI/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLCommandList.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLCommons.hpp"
#include "ARUI/Render/RenderCommons.hpp"
#include <unordered_set>
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
ARUI::Render::OpenGLRenderDevice::OpenGLRenderDevice() : IRenderDevice() {}
ARUI::Render::ImageHandle
ARUI::Render::OpenGLRenderDevice::CreateImage(const ImageDesc &desc) {
  GLTexture glTexture;
  switch (desc.type) {

  case ImageType::Image2D: {
    glCreateTextures(GL_TEXTURE_2D, 1, &glTexture.id);
    std::optional<GLenum> glFormat = OpenGL::GetGLImageFormat(desc.format);
    assert(glFormat.has_value());
    glTextureStorage2D(GL_TEXTURE_2D, 1, glFormat.value(), desc.extent.x,
                       desc.extent.y);
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
ARUI::Render::BufferHandle
ARUI::Render::OpenGLRenderDevice::CreateBuffer(const BufferDesc &desc) {
  // TODO: Implement this pure virtual method.
  GLBuffer glBuffer;
  glCreateBuffers(1, &glBuffer.id);
  // for every bit in BufferUsage, map it to the corresponding GLenum using
  // bufferUsageToGLenum
  GLenum usage = 0;

  bool success = OpenGL::BufferUsageBitsToGl(desc.usage, usage);
  assert(success);
  glBufferStorage(GL_ARRAY_BUFFER, desc.size, nullptr, usage);
  BufferHandle handle = GenerateBufferHandle();
  buffers[handle] = glBuffer;
  return handle;
}
ARUI::Render::GraphicsPipelineHandle
ARUI::Render::OpenGLRenderDevice::CreatePipeline(
    const GraphicsPipelineDesc &graphicsDesc) {
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
    glVertexBindingDivisor(binding.binding, binding.perInstance ? 1 : 0);
  }

  for (const auto &attr : graphicsDesc.vertexLayout.attributes) {
    const auto glFormatOpt = OpenGL::GetGLVertexFormat(attr.format);
    assert(glFormatOpt.has_value());
    const auto glFormat = glFormatOpt.value();

    glEnableVertexAttribArray(attr.location);

    glVertexAttribFormat(attr.location, glFormat.componentCount, glFormat.type,
                         glFormat.normalized, attr.offset);

    glVertexAttribBinding(attr.location, attr.binding);
  }

  glBindVertexArray(0);
  GraphicsPipelineHandle handle = GeneratePipelineHandle();
  GLPipeline glPipeline{
      .programId = program,
      .vaoId = vao,
  };
  pipelines[handle] = glPipeline;
  return handle;
}
ARUI::Render::ShaderModuleHandle
ARUI::Render::OpenGLRenderDevice::CreateShaderModule(
    const ShaderModuleDesc &desc) {
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
void ARUI::Render::OpenGLRenderDevice::Destroy(GraphicsPipelineHandle handle) {

  glDeleteProgram(pipelines[handle].programId);
  glDeleteVertexArrays(1, &pipelines[handle].vaoId);
  pipelines.erase(handle);
}
void ARUI::Render::OpenGLRenderDevice::Destroy(ShaderModuleHandle handle) {

  glDeleteShader(shaderModules[handle].id);
  shaderModules.erase(handle);
}
void ARUI::Render::OpenGLRenderDevice::Destroy(ImageHandle handle) {
  glDeleteTextures(1, &textures[handle].id);
  textures.erase(handle);
}
void ARUI::Render::OpenGLRenderDevice::Destroy(BufferHandle) {
  // TODO: Implement this pure virtual method.
  assert(false && "Method `DestroyBuffer` is not implemented.");
}
std::unique_ptr<ARUI::Render::IRenderCommandList>
ARUI::Render::OpenGLRenderDevice::CreateCommandList(QueueType type) {
  return std::make_unique<OpenGLCommandList>(this);
}
void ARUI::Render::OpenGLRenderDevice::Submit(
    const IRenderCommandList &commandList) {
  const OpenGLCommandList &glCommandList =
      dynamic_cast<const OpenGLCommandList &>(commandList);
  auto commandQueue = glCommandList.GetCommandQueue();
  for (auto &command : commandQueue) {

    std::visit([this](auto &&cmd) { cmd.Execute(this); }, command);
  }
}

ARUI::Render::RenderCapabilities
ARUI::Render::OpenGLRenderDevice::GetGLCapabilities() {
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
