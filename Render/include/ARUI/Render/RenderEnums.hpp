#pragma once
#include <cstdint>
#include <type_traits>
namespace ARUI::Render {

enum class ImageFormat : uint32_t {
  Undefiend,

  // Red
  R8_UNORM,
  R8_SNORM,
  R8_USCALED,
  R8_SSCALED,
  R8_UINT,
  R8_SINT,
  R8_SRGB,

  // RG
  R8G8_UNORM,
  R8G8_SNORM,
  R8G8_USCALED,
  R8G8_SSCALED,
  R8G8_UINT,
  R8G8_SINT,
  R8G8_SRGB,
  // RGB
  R8G8B8_UNORM,
  R8G8B8_SNORM,
  R8G8B8_USCALED,
  R8G8B8_SSCALED,
  R8G8B8_UINT,
  R8G8B8_SINT,
  R8G8B8_SRGB,

  //Depth
  DEPTH16,
  DEPTH32,
  DEPTH24_STENCIL8,
};
enum class ImageType : uint32_t {
  Image1D,
  Image2D,
  Image3D,

  Image1DArray,
  Image2DArray,
  Cube,
  CubeArray
};
enum class ImageUsageFlags : uint32_t {
  Sampled = 1 << 0,
  Storage = 1 << 1,
  ColorAttachment = 1 << 2,
  DepthAttachment = 1 << 3,
  TransferSrc = 1 << 4,
  TransferDst = 1 << 5,
};
using ImageUsage = std::underlying_type<ImageUsageFlags>::type;
enum class BufferUsageFlags : uint32_t {
  VertexBuffer = 1 << 0,
  IndexBuffer = 1 << 1,
  UniformBuffer = 1 << 2
};
using BufferUsage = std::underlying_type<BufferUsageFlags>::type;

enum class QueueType : uint32_t { Graphics, Compute, Transfer };
enum class ShaderStageFlags : uint32_t { Vertex, Fragment, Compute };
enum class PrimitiveTopology { Triangles, TriangleStrip, Lines, Points };

enum class CullMode { None, Front, Back };

enum class FrontFace { Clockwise, CounterClockwise };

enum class PolygonMode { Fill, Line, Point };
enum class CompareOp {
  Never,
  Less,
  LessEqual,
  Equal,
  GreaterEqual,
  Greater,
  NotEqual,
  Always
};
enum class BlendFactor {
  Zero,
  One,
  SrcAlpha,
  OneMinusSrcAlpha,
  DstAlpha,
  OneMinusDstAlpha
};

enum class BlendOp { Add, Subtract, ReverseSubtract, Min, Max };
enum class VertexFormat {
  Float,
  Float2,
  Float3,
  Float4,

  UInt,
  UInt2,
  UInt3,
  UInt4
};
enum class BindingType {
  UniformBuffer,
  StorageBuffer,
  SampledTexture,
  StorageTexture,
  Sampler
};

} // namespace ARUI::Render