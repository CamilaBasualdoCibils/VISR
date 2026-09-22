#pragma once

#include <boost/describe.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
namespace ARUI {
struct Pose {
  glm::vec3 position{0.0f};
  glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
  [[nodiscard]]
  glm::mat4 ToMatrix() const {
    return glm::translate(glm::mat4{1.0f}, position) *
           glm::mat4_cast(orientation);
  }

  [[nodiscard]]
  glm::vec3 Forward() const {
    return orientation * glm::vec3{0.0f, 0.0f, -1.0f};
  }

  [[nodiscard]]
  glm::vec3 Right() const {
    return orientation * glm::vec3{1.0f, 0.0f, 0.0f};
  }

  [[nodiscard]]
  glm::vec3 Up() const {
    return orientation * glm::vec3{0.0f, 1.0f, 0.0f};
  }
};
enum class Space {
  World,
  Body,
  Head,
};
BOOST_DESCRIBE_ENUM(Space, World, Body, Head);
struct SpatialPose {
  Space relativeTo;
  Pose pose;
};
} // namespace ARUI