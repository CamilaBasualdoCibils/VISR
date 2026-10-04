#pragma once

#include "VISR/Render/RenderEnums.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <optional>
#include <unordered_map>
namespace VISR::Render::OpenGL {
const std::unordered_map<ImageFormat, GLenum> textureFormatToGLenumMap = {
    {ImageFormat::R8_UNORM, GL_R8},
    {ImageFormat::R8_SNORM, GL_R8_SNORM},
    {ImageFormat::R8_USCALED, GL_R8UI},
    {ImageFormat::R8_SSCALED, GL_R8I},
    {ImageFormat::R8_UINT, GL_R8UI},
    {ImageFormat::R8_SINT, GL_R8I},
    {ImageFormat::R8_SRGB, GL_R8},
    {ImageFormat::R8G8_UNORM, GL_RG8},
    {ImageFormat::R8G8_SNORM, GL_RG8_SNORM},
    {ImageFormat::R8G8_USCALED, GL_RG8UI},
    {ImageFormat::R8G8_SSCALED, GL_RG8I},
    {ImageFormat::R8G8_UINT, GL_RG8UI},
    {ImageFormat::R8G8_SINT, GL_RG8I},
    {ImageFormat::R8G8_SRGB, GL_RG8},
    {ImageFormat::R8G8B8_UNORM, GL_RGB8},
    {ImageFormat::R8G8B8_SNORM, GL_RGB8_SNORM},
    {ImageFormat::R8G8B8_USCALED, GL_RGB8UI},
    {ImageFormat::R8G8B8_SSCALED, GL_RGB8I},
    {ImageFormat::R8G8B8_UINT, GL_RGB8UI},
    {ImageFormat::R8G8B8_SINT, GL_RGB8I},
    {ImageFormat::R8G8B8_SRGB, GL_RGB8},
    {ImageFormat::R8G8B8A8_UNORM, GL_RGBA8},
    {ImageFormat::R8G8B8A8_SRGB, GL_SRGB8_ALPHA8},
    {ImageFormat::DEPTH24_STENCIL8, GL_DEPTH24_STENCIL8}};
inline std::optional<GLenum> GetGLImageFormat(ImageFormat format) {
  auto it = textureFormatToGLenumMap.find(format);
  if (it != textureFormatToGLenumMap.end()) {
    return it->second;
  }
  return std::nullopt;
}
const std::unordered_map<BufferUsageFlags, GLenum> bufferUsageToGLenumMap = {
    {BufferUsageFlags::VertexBuffer, GL_ARRAY_BUFFER},
    {BufferUsageFlags::IndexBuffer, GL_ELEMENT_ARRAY_BUFFER},
    {BufferUsageFlags::UniformBuffer, GL_UNIFORM_BUFFER}};
inline std::optional<GLenum> GetGLBufferUsageFlags(BufferUsageFlags usage) {
  auto it = bufferUsageToGLenumMap.find(usage);
  if (it != bufferUsageToGLenumMap.end()) {
    return it->second;
  }
  return std::nullopt;
}
inline bool BufferUsageBitsToGl(BufferUsage usage, GLenum &glUsage) {
  glUsage = 0;
  for (const auto &usagePair : bufferUsageToGLenumMap) {
    if ((uint32_t)usage & (uint32_t)usagePair.first) {
      glUsage |= usagePair.second;
    }
  }
  return glUsage != 0;
}
const std::unordered_map<ShaderStageFlags, GLenum> shaderStageToGLenumMap = {
    {ShaderStageFlags::Vertex, GL_VERTEX_SHADER},
    {ShaderStageFlags::Fragment, GL_FRAGMENT_SHADER},
    {ShaderStageFlags::Compute, GL_COMPUTE_SHADER}};
inline std::optional<GLenum> GetGLShaderStage(ShaderStageFlags stage) {
  auto it = shaderStageToGLenumMap.find(stage);
  if (it != shaderStageToGLenumMap.end()) {
    return it->second;
  }
  return std::nullopt;
}
const std::unordered_map<PrimitiveTopology, GLenum>
    primitiveTopologyToGLenumMap = {
        {PrimitiveTopology::Points, GL_POINTS},
        {PrimitiveTopology::Lines, GL_LINES},
        {PrimitiveTopology::Triangles, GL_TRIANGLES}};
inline std::optional<GLenum>
GetGLPrimitiveTopology(PrimitiveTopology topology) {
  auto it = primitiveTopologyToGLenumMap.find(topology);
  if (it != primitiveTopologyToGLenumMap.end()) {
    return it->second;
  }
  return std::nullopt;
}
struct GLVertexFormat {
  GLenum type;
  GLint componentCount;
  GLboolean normalized;
};
const std::unordered_map<VertexFormat, GLVertexFormat> vertexFormatToGLenumMap =
    {{VertexFormat::Float, GLVertexFormat{.type = GL_FLOAT,
                                          .componentCount = 1,
                                          .normalized = GL_FALSE}},
     {VertexFormat::Float2, GLVertexFormat{.type = GL_FLOAT,
                                           .componentCount = 2,
                                           .normalized = GL_FALSE}},
     {VertexFormat::Float3, GLVertexFormat{.type = GL_FLOAT,
                                           .componentCount = 3,
                                           .normalized = GL_FALSE}},
     {VertexFormat::Float4, GLVertexFormat{.type = GL_FLOAT,
                                           .componentCount = 4,
                                           .normalized = GL_FALSE}},
     {VertexFormat::UInt, GLVertexFormat{.type = GL_UNSIGNED_INT,
                                         .componentCount = 1,
                                         .normalized = GL_FALSE}},
     {VertexFormat::UInt2, GLVertexFormat{.type = GL_UNSIGNED_INT,
                                          .componentCount = 2,
                                          .normalized = GL_FALSE}},
     {VertexFormat::UInt3, GLVertexFormat{.type = GL_UNSIGNED_INT,
                                          .componentCount = 3,
                                          .normalized = GL_FALSE}},
     {VertexFormat::UInt4, GLVertexFormat{.type = GL_UNSIGNED_INT,
                                          .componentCount = 4,
                                          .normalized = GL_FALSE}}};

inline std::optional<GLVertexFormat> GetGLVertexFormat(VertexFormat format) {

  auto it = vertexFormatToGLenumMap.find(format);
  if (it != vertexFormatToGLenumMap.end()) {
    return it->second;
  }
  return std::nullopt;
}
const std::unordered_map<ImageType, GLenum> imageTypeToGLenumMap = {
    {ImageType::Image1D, GL_TEXTURE_1D},
    {ImageType::Image2D, GL_TEXTURE_2D},
    {ImageType::Image3D, GL_TEXTURE_3D},
    {ImageType::Image1DArray, GL_TEXTURE_1D_ARRAY},
    {ImageType::Image2DArray, GL_TEXTURE_2D_ARRAY},
    {ImageType::Cube, GL_TEXTURE_CUBE_MAP},
    {ImageType::CubeArray, GL_TEXTURE_CUBE_MAP_ARRAY}};
inline std::optional<GLenum> GetGLImageType(ImageType type) {
  auto it = imageTypeToGLenumMap.find(type);
  if (it != imageTypeToGLenumMap.end()) {
    return it->second;
  }
  return std::nullopt;
}
} // namespace VISR::Render::OpenGL