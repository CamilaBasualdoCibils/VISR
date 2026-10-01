#include "ARUI/XR/OpenXR/OpenXRTrackingProvider.hpp"

#include <EGL/egl.h>
#include <GL/glew.h>
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_PLATFORM_EGL
#include <openxr/openxr_platform.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace ARUI::OpenXR {
namespace {
constexpr std::array<std::string_view, 3> jointNames = {"head", "left_hand",
                                                        "right_hand"};
constexpr std::array<std::string_view, XR_HAND_JOINT_COUNT_EXT> handJointNames = {
    "palm", "wrist", "thumb_metacarpal", "thumb_proximal", "thumb_distal",
    "thumb_tip", "index_metacarpal", "index_proximal",
    "index_intermediate", "index_distal", "index_tip", "middle_metacarpal",
    "middle_proximal", "middle_intermediate", "middle_distal", "middle_tip",
    "ring_metacarpal", "ring_proximal", "ring_intermediate", "ring_distal",
    "ring_tip", "little_metacarpal", "little_proximal",
    "little_intermediate", "little_distal", "little_tip"};

constexpr std::array<std::string_view, XR_FULL_BODY_JOINT_COUNT_META> bodyJointNames = {
    "body_root", "body_hips", "body_spine_lower", "body_spine_middle",
    "body_spine_upper", "body_chest", "body_neck", "body_head",
    "body_left_shoulder", "body_left_scapula", "body_left_arm_upper", "body_left_arm_lower",
    "body_left_hand_wrist_twist", "body_right_shoulder", "body_right_scapula", "body_right_arm_upper",
    "body_right_arm_lower", "body_right_hand_wrist_twist", "body_left_hand_palm", "body_left_hand_wrist",
    "body_left_hand_thumb_metacarpal", "body_left_hand_thumb_proximal", "body_left_hand_thumb_distal", "body_left_hand_thumb_tip",
    "body_left_hand_index_metacarpal", "body_left_hand_index_proximal", "body_left_hand_index_intermediate", "body_left_hand_index_distal",
    "body_left_hand_index_tip", "body_left_hand_middle_metacarpal", "body_left_hand_middle_proximal", "body_left_hand_middle_intermediate",
    "body_left_hand_middle_distal", "body_left_hand_middle_tip", "body_left_hand_ring_metacarpal", "body_left_hand_ring_proximal",
    "body_left_hand_ring_intermediate", "body_left_hand_ring_distal", "body_left_hand_ring_tip", "body_left_hand_little_metacarpal",
    "body_left_hand_little_proximal", "body_left_hand_little_intermediate", "body_left_hand_little_distal", "body_left_hand_little_tip",
    "body_right_hand_palm", "body_right_hand_wrist", "body_right_hand_thumb_metacarpal", "body_right_hand_thumb_proximal",
    "body_right_hand_thumb_distal", "body_right_hand_thumb_tip", "body_right_hand_index_metacarpal", "body_right_hand_index_proximal",
    "body_right_hand_index_intermediate", "body_right_hand_index_distal", "body_right_hand_index_tip", "body_right_hand_middle_metacarpal",
    "body_right_hand_middle_proximal", "body_right_hand_middle_intermediate", "body_right_hand_middle_distal", "body_right_hand_middle_tip",
    "body_right_hand_ring_metacarpal", "body_right_hand_ring_proximal", "body_right_hand_ring_intermediate", "body_right_hand_ring_distal",
    "body_right_hand_ring_tip", "body_right_hand_little_metacarpal", "body_right_hand_little_proximal", "body_right_hand_little_intermediate",
    "body_right_hand_little_distal", "body_right_hand_little_tip", "body_left_upper_leg", "body_left_lower_leg",
    "body_left_foot_ankle_twist", "body_left_foot_ankle", "body_left_foot_subtalar", "body_left_foot_transverse",
    "body_left_foot_ball", "body_right_upper_leg", "body_right_lower_leg", "body_right_foot_ankle_twist",
    "body_right_foot_ankle", "body_right_foot_subtalar", "body_right_foot_transverse", "body_right_foot_ball"};

const auto &HandJointPaths() {
  static const auto names = [] {
    std::array<std::string, 2 * XR_HAND_JOINT_COUNT_EXT> result;
    for (size_t hand = 0; hand < 2; ++hand)
      for (size_t joint = 0; joint < XR_HAND_JOINT_COUNT_EXT; ++joint)
        result[hand * XR_HAND_JOINT_COUNT_EXT + joint] =
            std::string(hand == 0 ? "left_hand_" : "right_hand_") +
            std::string(handJointNames[joint]);
    return result;
  }();
  return names;
}

XrPosef IdentityPose() {
  return {{0.0F, 0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 0.0F}};
}

TrackedPose ToTrackedPose(const XrSpaceLocation &location) {
  TrackedPose result;
  result.positionValid =
      (location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;
  result.orientationValid =
      (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0;
  result.positionTracked =
      (location.locationFlags & XR_SPACE_LOCATION_POSITION_TRACKED_BIT) != 0;
  result.orientationTracked =
      (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT) != 0;
  if (result.positionValid) {
    const auto &p = location.pose.position;
    result.pose.position = {p.x, p.y, p.z};
  }
  if (result.orientationValid) {
    const auto &q = location.pose.orientation;
    result.pose.orientation = {q.w, q.x, q.y, q.z};
  }
  return result;
}

TrackedPose ToTrackedPose(const XrBodyJointLocationFB &location) {
  XrSpaceLocation space{XR_TYPE_SPACE_LOCATION};
  space.locationFlags = location.locationFlags;
  space.pose = location.pose;
  return ToTrackedPose(space);
}

TrackedPose ToTrackedPose(const XrHandJointLocationEXT &location) {
  XrSpaceLocation space{XR_TYPE_SPACE_LOCATION};
  space.locationFlags = location.locationFlags;
  space.pose = location.pose;
  return ToTrackedPose(space);
}

RenderView ToRenderView(const char *name, const XrView &eye,
                        glm::ivec2 extent) {
  Pose pose;
  pose.position = {eye.pose.position.x, eye.pose.position.y,
                   eye.pose.position.z};
  pose.orientation = {eye.pose.orientation.w, eye.pose.orientation.x,
                      eye.pose.orientation.y, eye.pose.orientation.z};
  constexpr float nearPlane = 0.05F;
  constexpr float farPlane = 100.0F;
  return {.Name = name,
          .view = glm::inverse(pose.ToMatrix()),
          .projection = glm::frustum(std::tan(eye.fov.angleLeft) * nearPlane,
                                     std::tan(eye.fov.angleRight) * nearPlane,
                                     std::tan(eye.fov.angleDown) * nearPlane,
                                     std::tan(eye.fov.angleUp) * nearPlane,
                                     nearPlane, farPlane),
          .viewportSize = extent};
}

PFN_xrVoidFunction GetEGLProcAddress(const char *name) {
  return reinterpret_cast<PFN_xrVoidFunction>(eglGetProcAddress(name));
}
} // namespace

struct OpenXRTrackingProvider::State {
  XrInstance instance{XR_NULL_HANDLE};
  XrSession session{XR_NULL_HANDLE};
  XrSpace localSpace{XR_NULL_HANDLE};
  XrSpace headSpace{XR_NULL_HANDLE};
  std::array<XrSpace, 2> handSpaces{XR_NULL_HANDLE, XR_NULL_HANDLE};
  XrActionSet actionSet{XR_NULL_HANDLE};
  XrAction gripAction{XR_NULL_HANDLE};
  std::array<XrPath, 2> handPaths{XR_NULL_PATH, XR_NULL_PATH};
  std::array<std::optional<TrackedPose>, 3> poses{};
  std::array<std::array<std::optional<TrackedPose>, XR_HAND_JOINT_COUNT_EXT>, 2> handJoints{};
  std::array<XrHandTrackerEXT, 2> handTrackers{XR_NULL_HANDLE, XR_NULL_HANDLE};
  PFN_xrCreateHandTrackerEXT createHandTracker{};
  PFN_xrDestroyHandTrackerEXT destroyHandTracker{};
  PFN_xrLocateHandJointsEXT locateHandJoints{};
  bool supportsHandTracking{};
  std::array<bool, 2> handTracked{};
  std::array<bool, 2> warnedHandLocate{};
  XrBodyTrackerFB bodyTracker{XR_NULL_HANDLE};
  PFN_xrCreateBodyTrackerFB createBodyTracker{};
  PFN_xrDestroyBodyTrackerFB destroyBodyTracker{};
  PFN_xrLocateBodyJointsFB locateBodyJoints{};
  PFN_xrGetBodySkeletonFB getBodySkeleton{};
  std::vector<std::optional<TrackedPose>> bodyJoints;
  std::vector<int32_t> bodyParents;
  bool supportsBodyTracking{};
  bool bodyTracked{};
  bool bodySkeletonReady{};
  bool warnedBodyLocate{};
  float bodyConfidence{};
  uint32_t skeletonChangedCount{};
  std::array<XrSwapchain, 2> swapchains{XR_NULL_HANDLE, XR_NULL_HANDLE};
  std::array<std::vector<XrSwapchainImageOpenGLKHR>, 2> swapchainImages;
  std::array<glm::ivec2, 2> extents{};
  std::array<XrView, 2> locatedViews{};
  std::array<RenderView, 2> frameViews{};
  XrEnvironmentBlendMode blendMode{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};
  bool supportsAlphaBlend{};
  bool passthroughEnabled{};
  XrEnvironmentDepthProviderMETA depthProvider{XR_NULL_HANDLE};
  XrEnvironmentDepthSwapchainMETA depthSwapchain{XR_NULL_HANDLE};
  PFN_xrCreateEnvironmentDepthProviderMETA createDepthProvider{};
  PFN_xrDestroyEnvironmentDepthProviderMETA destroyDepthProvider{};
  PFN_xrStartEnvironmentDepthProviderMETA startDepthProvider{};
  PFN_xrStopEnvironmentDepthProviderMETA stopDepthProvider{};
  PFN_xrCreateEnvironmentDepthSwapchainMETA createDepthSwapchain{};
  PFN_xrDestroyEnvironmentDepthSwapchainMETA destroyDepthSwapchain{};
  PFN_xrEnumerateEnvironmentDepthSwapchainImagesMETA enumerateDepthImages{};
  PFN_xrGetEnvironmentDepthSwapchainStateMETA getDepthSwapchainState{};
  PFN_xrAcquireEnvironmentDepthImageMETA acquireDepthImage{};
  std::vector<XrSwapchainImageOpenGLKHR> depthImages;
  uint32_t depthWidth{};
  uint32_t depthHeight{};
  bool supportsDepth{};
  bool depthStarted{};
  bool depthAttemptedThisFrame{};
  bool warnedDepthAcquire{};
  std::optional<XRDepthFrame> depthFrame;
  XrTime predictedTime{};
  bool actionsAttached{};
  bool running{};
  bool frameBegun{};
  bool frameShouldRender{};
  bool viewsValid{};
  bool warnedSubmission{};
};

OpenXRTrackingProvider::OpenXRTrackingProvider()
    : state_(std::make_unique<State>()) {}

OpenXRTrackingProvider::~OpenXRTrackingProvider() {
  if (!state_)
    return;
  if (state_->depthStarted && state_->stopDepthProvider)
    state_->stopDepthProvider(state_->depthProvider);
  if (state_->depthSwapchain != XR_NULL_HANDLE &&
      state_->destroyDepthSwapchain)
    state_->destroyDepthSwapchain(state_->depthSwapchain);
  if (state_->depthProvider != XR_NULL_HANDLE &&
      state_->destroyDepthProvider)
    state_->destroyDepthProvider(state_->depthProvider);
  if (state_->bodyTracker != XR_NULL_HANDLE && state_->destroyBodyTracker)
    state_->destroyBodyTracker(state_->bodyTracker);
  if (state_->destroyHandTracker)
    for (XrHandTrackerEXT tracker : state_->handTrackers)
      if (tracker != XR_NULL_HANDLE)
        state_->destroyHandTracker(tracker);
  for (XrSwapchain swapchain : state_->swapchains)
    if (swapchain != XR_NULL_HANDLE)
      xrDestroySwapchain(swapchain);
  for (XrSpace space : state_->handSpaces)
    if (space != XR_NULL_HANDLE)
      xrDestroySpace(space);
  if (state_->headSpace != XR_NULL_HANDLE)
    xrDestroySpace(state_->headSpace);
  if (state_->localSpace != XR_NULL_HANDLE)
    xrDestroySpace(state_->localSpace);
  if (state_->session != XR_NULL_HANDLE)
    xrDestroySession(state_->session);
  if (state_->actionSet != XR_NULL_HANDLE)
    xrDestroyActionSet(state_->actionSet);
  if (state_->instance != XR_NULL_HANDLE)
    xrDestroyInstance(state_->instance);
}

std::unique_ptr<OpenXRTrackingProvider> OpenXRTrackingProvider::TryCreate() {
  auto provider =
      std::unique_ptr<OpenXRTrackingProvider>(new OpenXRTrackingProvider());
  if (provider->Initialize()) {
    spdlog::info("OpenXR EGL stereo session created");
    return provider;
  }
  spdlog::info("OpenXR EGL stereo session unavailable; using manual views");
  return nullptr;
}

bool OpenXRTrackingProvider::Initialize() {
  const EGLDisplay display = eglGetCurrentDisplay();
  const EGLContext context = eglGetCurrentContext();
  if (display == EGL_NO_DISPLAY || context == EGL_NO_CONTEXT)
    return false;
  EGLint configId = 0;
  if (eglQueryContext(display, context, EGL_CONFIG_ID, &configId) != EGL_TRUE)
    return false;
  const EGLint configAttributes[] = {EGL_CONFIG_ID, configId, EGL_NONE};
  EGLConfig config{};
  EGLint configCount = 0;
  if (eglChooseConfig(display, configAttributes, &config, 1, &configCount) !=
          EGL_TRUE ||
      configCount == 0)
    return false;

  uint32_t extensionCount = 0;
  if (xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount,
                                             nullptr) != XR_SUCCESS)
    return false;
  std::vector<XrExtensionProperties> extensions(extensionCount,
                                                {XR_TYPE_EXTENSION_PROPERTIES});
  if (xrEnumerateInstanceExtensionProperties(nullptr, extensionCount,
                                             &extensionCount,
                                             extensions.data()) != XR_SUCCESS)
    return false;
  const auto hasExtension = [&](const char *name) {
    return std::any_of(extensions.begin(), extensions.end(),
                       [&](const XrExtensionProperties &extension) {
                         return std::strcmp(extension.extensionName, name) == 0;
                       });
  };
  if (!hasExtension(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME) ||
      !hasExtension(XR_MNDX_EGL_ENABLE_EXTENSION_NAME))
    return false;

  const bool depthExtension =
      hasExtension(XR_META_ENVIRONMENT_DEPTH_EXTENSION_NAME);
  const bool handExtension = hasExtension(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
  const bool bodyExtension = hasExtension(XR_FB_BODY_TRACKING_EXTENSION_NAME);
  const bool fullBodyExtension =
      bodyExtension && hasExtension(XR_META_BODY_TRACKING_FULL_BODY_EXTENSION_NAME);
  std::vector<const char *> enabledExtensions = {
      XR_KHR_OPENGL_ENABLE_EXTENSION_NAME, XR_MNDX_EGL_ENABLE_EXTENSION_NAME};
  if (handExtension)
    enabledExtensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
  else
    spdlog::warn("OpenXR runtime does not advertise XR_EXT_hand_tracking");
  if (bodyExtension)
    enabledExtensions.push_back(XR_FB_BODY_TRACKING_EXTENSION_NAME);
  if (fullBodyExtension)
    enabledExtensions.push_back(XR_META_BODY_TRACKING_FULL_BODY_EXTENSION_NAME);
  if (depthExtension)
    enabledExtensions.push_back(XR_META_ENVIRONMENT_DEPTH_EXTENSION_NAME);
  XrInstanceCreateInfo instanceInfo{XR_TYPE_INSTANCE_CREATE_INFO};
  std::strncpy(instanceInfo.applicationInfo.applicationName, "ARUI Simulator",
               XR_MAX_APPLICATION_NAME_SIZE - 1);
  std::strncpy(instanceInfo.applicationInfo.engineName, "ARUI",
               XR_MAX_ENGINE_NAME_SIZE - 1);
  instanceInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
  instanceInfo.enabledExtensionCount = std::size(enabledExtensions);
  instanceInfo.enabledExtensionNames = enabledExtensions.data();
  if (xrCreateInstance(&instanceInfo, &state_->instance) != XR_SUCCESS)
    return false;

  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  XrSystemId system = XR_NULL_SYSTEM_ID;
  if (xrGetSystem(state_->instance, &systemInfo, &system) != XR_SUCCESS)
    return false;
  if (handExtension) {
    XrSystemHandTrackingPropertiesEXT handProperties{XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
    XrSystemProperties properties{XR_TYPE_SYSTEM_PROPERTIES};
    properties.next = &handProperties;
    const XrResult result =
        xrGetSystemProperties(state_->instance, system, &properties);
    state_->supportsHandTracking =
        result == XR_SUCCESS && handProperties.supportsHandTracking == XR_TRUE;
    if (!state_->supportsHandTracking)
      spdlog::warn("OpenXR system hand tracking unavailable: properties={}, supported={}",
                   static_cast<int>(result), handProperties.supportsHandTracking);
  }

  if (bodyExtension) {
    XrSystemBodyTrackingPropertiesFB bodyProperties{
        XR_TYPE_SYSTEM_BODY_TRACKING_PROPERTIES_FB};
    XrSystemPropertiesBodyTrackingFullBodyMETA fullProperties{
        XR_TYPE_SYSTEM_PROPERTIES_BODY_TRACKING_FULL_BODY_META};
    if (fullBodyExtension)
      bodyProperties.next = &fullProperties;
    XrSystemProperties properties{XR_TYPE_SYSTEM_PROPERTIES};
    properties.next = &bodyProperties;
    const XrResult result =
        xrGetSystemProperties(state_->instance, system, &properties);
    state_->supportsBodyTracking =
        result == XR_SUCCESS && bodyProperties.supportsBodyTracking == XR_TRUE;
    const bool supportsFull = state_->supportsBodyTracking &&
        fullBodyExtension && fullProperties.supportsFullBodyTracking == XR_TRUE;
    if (state_->supportsBodyTracking)
      state_->bodyJoints.resize(
          supportsFull ? static_cast<size_t>(XR_FULL_BODY_JOINT_COUNT_META)
                       : static_cast<size_t>(XR_BODY_JOINT_COUNT_FB));
    spdlog::info("OpenXR body tracking: supported={}, full-body={}",
                 state_->supportsBodyTracking, supportsFull);
  } else {
    spdlog::info("OpenXR runtime does not advertise XR_FB_body_tracking");
  }

  if (depthExtension) {
    XrSystemEnvironmentDepthPropertiesMETA depthProperties{
        XR_TYPE_SYSTEM_ENVIRONMENT_DEPTH_PROPERTIES_META};
    XrSystemProperties properties{XR_TYPE_SYSTEM_PROPERTIES};
    properties.next = &depthProperties;
    const XrResult result =
        xrGetSystemProperties(state_->instance, system, &properties);
    state_->supportsDepth =
        result == XR_SUCCESS &&
        depthProperties.supportsEnvironmentDepth == XR_TRUE;
    spdlog::info("OpenXR environment depth supported={}",
                 state_->supportsDepth);
  }

  PFN_xrGetOpenGLGraphicsRequirementsKHR getRequirements = nullptr;
  if (xrGetInstanceProcAddr(
          state_->instance, "xrGetOpenGLGraphicsRequirementsKHR",
          reinterpret_cast<PFN_xrVoidFunction *>(&getRequirements)) !=
          XR_SUCCESS ||
      !getRequirements)
    return false;
  XrGraphicsRequirementsOpenGLKHR requirements{
      XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
  if (getRequirements(state_->instance, system, &requirements) != XR_SUCCESS)
    return false;
  GLint major = 0;
  GLint minor = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  const XrVersion glVersion = XR_MAKE_VERSION(major, minor, 0);
  if (glVersion < requirements.minApiVersionSupported ||
      glVersion > requirements.maxApiVersionSupported)
    return false;

  uint32_t blendModeCount = 0;
  if (xrEnumerateEnvironmentBlendModes(
          state_->instance, system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
          0, &blendModeCount, nullptr) != XR_SUCCESS ||
      blendModeCount == 0)
    return false;
  std::vector<XrEnvironmentBlendMode> blendModes(blendModeCount);
  if (xrEnumerateEnvironmentBlendModes(
          state_->instance, system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
          blendModeCount, &blendModeCount, blendModes.data()) != XR_SUCCESS)
    return false;
  state_->blendMode = blendModes.front();
  state_->supportsAlphaBlend =
      std::find(blendModes.begin(), blendModes.end(),
                XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND) != blendModes.end();
  spdlog::info("OpenXR alpha-blend passthrough supported={}",
               state_->supportsAlphaBlend);

  uint32_t viewCount = 0;
  if (xrEnumerateViewConfigurationViews(
          state_->instance, system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
          0, &viewCount, nullptr) != XR_SUCCESS ||
      viewCount != 2)
    return false;
  std::array<XrViewConfigurationView, 2> viewConfigs{};
  for (auto &view : viewConfigs)
    view.type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
  if (xrEnumerateViewConfigurationViews(
          state_->instance, system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
          viewConfigs.size(), &viewCount, viewConfigs.data()) != XR_SUCCESS)
    return false;

  XrGraphicsBindingEGLMNDX binding{XR_TYPE_GRAPHICS_BINDING_EGL_MNDX};
  binding.getProcAddress = GetEGLProcAddress;
  binding.display = display;
  binding.config = config;
  binding.context = context;
  XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
  sessionInfo.next = &binding;
  sessionInfo.systemId = system;
  const XrResult sessionResult =
      xrCreateSession(state_->instance, &sessionInfo, &state_->session);
  if (sessionResult != XR_SUCCESS) {
    spdlog::warn("OpenXR xrCreateSession failed: {}",
                 static_cast<int>(sessionResult));
    return false;
  }

  uint32_t formatCount = 0;
  if (xrEnumerateSwapchainFormats(state_->session, 0, &formatCount, nullptr) !=
          XR_SUCCESS ||
      formatCount == 0)
    return false;
  std::vector<int64_t> formats(formatCount);
  if (xrEnumerateSwapchainFormats(state_->session, formatCount, &formatCount,
                                  formats.data()) != XR_SUCCESS)
    return false;
  int64_t colorFormat = formats.front();
  if (std::find(formats.begin(), formats.end(), GL_RGBA8) != formats.end())
    colorFormat = GL_RGBA8;
  else if (std::find(formats.begin(), formats.end(), GL_SRGB8_ALPHA8) !=
           formats.end())
    colorFormat = GL_SRGB8_ALPHA8;

  for (size_t i = 0; i < 2; ++i) {
    state_->extents[i] = {
        static_cast<int>(viewConfigs[i].recommendedImageRectWidth),
        static_cast<int>(viewConfigs[i].recommendedImageRectHeight)};
    XrSwapchainCreateInfo swapchainInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.format = colorFormat;
    swapchainInfo.sampleCount = 1;
    swapchainInfo.width = state_->extents[i].x;
    swapchainInfo.height = state_->extents[i].y;
    swapchainInfo.faceCount = 1;
    swapchainInfo.arraySize = 1;
    swapchainInfo.mipCount = 1;
    if (xrCreateSwapchain(state_->session, &swapchainInfo,
                          &state_->swapchains[i]) != XR_SUCCESS)
      return false;
    uint32_t imageCount = 0;
    if (xrEnumerateSwapchainImages(state_->swapchains[i], 0, &imageCount,
                                   nullptr) != XR_SUCCESS ||
        imageCount == 0)
      return false;
    state_->swapchainImages[i].resize(imageCount);
    for (auto &image : state_->swapchainImages[i])
      image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
    if (xrEnumerateSwapchainImages(
            state_->swapchains[i], imageCount, &imageCount,
            reinterpret_cast<XrSwapchainImageBaseHeader *>(
                state_->swapchainImages[i].data())) != XR_SUCCESS)
      return false;
  }

  XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  spaceInfo.poseInReferenceSpace = IdentityPose();
  spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  if (xrCreateReferenceSpace(state_->session, &spaceInfo,
                             &state_->localSpace) != XR_SUCCESS)
    return false;
  spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  if (xrCreateReferenceSpace(state_->session, &spaceInfo, &state_->headSpace) !=
      XR_SUCCESS)
    return false;

  if (state_->supportsDepth) {
    const auto getProc = [&](const char *name, auto &proc) {
      return xrGetInstanceProcAddr(
                 state_->instance, name,
                 reinterpret_cast<PFN_xrVoidFunction *>(&proc)) == XR_SUCCESS &&
             proc != nullptr;
    };
    if (getProc("xrCreateEnvironmentDepthProviderMETA",
                state_->createDepthProvider) &&
        getProc("xrDestroyEnvironmentDepthProviderMETA",
                state_->destroyDepthProvider) &&
        getProc("xrStartEnvironmentDepthProviderMETA",
                state_->startDepthProvider) &&
        getProc("xrStopEnvironmentDepthProviderMETA",
                state_->stopDepthProvider) &&
        getProc("xrCreateEnvironmentDepthSwapchainMETA",
                state_->createDepthSwapchain) &&
        getProc("xrDestroyEnvironmentDepthSwapchainMETA",
                state_->destroyDepthSwapchain) &&
        getProc("xrEnumerateEnvironmentDepthSwapchainImagesMETA",
                state_->enumerateDepthImages) &&
        getProc("xrGetEnvironmentDepthSwapchainStateMETA",
                state_->getDepthSwapchainState) &&
        getProc("xrAcquireEnvironmentDepthImageMETA",
                state_->acquireDepthImage)) {
      XrEnvironmentDepthProviderCreateInfoMETA providerInfo{
          XR_TYPE_ENVIRONMENT_DEPTH_PROVIDER_CREATE_INFO_META};
      XrResult result = state_->createDepthProvider(
          state_->session, &providerInfo, &state_->depthProvider);
      if (result == XR_SUCCESS) {
        XrEnvironmentDepthSwapchainCreateInfoMETA swapchainInfo{
            XR_TYPE_ENVIRONMENT_DEPTH_SWAPCHAIN_CREATE_INFO_META};
        result = state_->createDepthSwapchain(
            state_->depthProvider, &swapchainInfo, &state_->depthSwapchain);
      }
      if (result == XR_SUCCESS) {
        XrEnvironmentDepthSwapchainStateMETA swapchainState{
            XR_TYPE_ENVIRONMENT_DEPTH_SWAPCHAIN_STATE_META};
        result = state_->getDepthSwapchainState(state_->depthSwapchain,
                                                &swapchainState);
        if (result == XR_SUCCESS) {
          state_->depthWidth = swapchainState.width;
          state_->depthHeight = swapchainState.height;
        }
      }
      uint32_t imageCount = 0;
      if (result == XR_SUCCESS)
        result = state_->enumerateDepthImages(state_->depthSwapchain, 0,
                                               &imageCount, nullptr);
      if (result == XR_SUCCESS && imageCount > 0) {
        state_->depthImages.resize(imageCount);
        for (auto &image : state_->depthImages)
          image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
        result = state_->enumerateDepthImages(
            state_->depthSwapchain, imageCount, &imageCount,
            reinterpret_cast<XrSwapchainImageBaseHeader *>(
                state_->depthImages.data()));
      }
      if (result == XR_SUCCESS && imageCount > 0) {
        result = state_->startDepthProvider(state_->depthProvider);
        state_->depthStarted = result == XR_SUCCESS;
      }
      if (result != XR_SUCCESS || imageCount == 0) {
        spdlog::warn("OpenXR environment depth initialization failed: {}",
                     static_cast<int>(result));
        state_->supportsDepth = false;
      } else {
        spdlog::info("OpenXR environment depth ready: {}x{}, {} images",
                     state_->depthWidth, state_->depthHeight, imageCount);
      }
    } else {
      state_->supportsDepth = false;
      spdlog::warn("OpenXR environment depth functions unavailable");
    }
  }

  if (state_->supportsHandTracking) {
    const auto getProc = [&](const char *name, auto &proc) {
      return xrGetInstanceProcAddr(state_->instance, name,
          reinterpret_cast<PFN_xrVoidFunction *>(&proc)) == XR_SUCCESS && proc;
    };
    if (getProc("xrCreateHandTrackerEXT", state_->createHandTracker) &&
        getProc("xrDestroyHandTrackerEXT", state_->destroyHandTracker) &&
        getProc("xrLocateHandJointsEXT", state_->locateHandJoints)) {
      for (size_t hand = 0; hand < 2; ++hand) {
        XrHandTrackerCreateInfoEXT createInfo{XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
        createInfo.hand = hand == 0 ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
        createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
        const XrResult result = state_->createHandTracker(
            state_->session, &createInfo, &state_->handTrackers[hand]);
        if (result != XR_SUCCESS)
          spdlog::warn("OpenXR hand tracker {} unavailable: {}", hand,
                       static_cast<int>(result));
      }
      state_->supportsHandTracking = std::any_of(
          state_->handTrackers.begin(), state_->handTrackers.end(),
          [](XrHandTrackerEXT tracker) { return tracker != XR_NULL_HANDLE; });
      if (state_->supportsHandTracking)
        spdlog::info("OpenXR articulated hand tracking available");
    } else {
      state_->supportsHandTracking = false;
    }
  }

  if (state_->supportsBodyTracking) {
    const auto getProc = [&](const char *name, auto &proc) {
      return xrGetInstanceProcAddr(state_->instance, name,
          reinterpret_cast<PFN_xrVoidFunction *>(&proc)) == XR_SUCCESS && proc;
    };
    if (getProc("xrCreateBodyTrackerFB", state_->createBodyTracker) &&
        getProc("xrDestroyBodyTrackerFB", state_->destroyBodyTracker) &&
        getProc("xrLocateBodyJointsFB", state_->locateBodyJoints) &&
        getProc("xrGetBodySkeletonFB", state_->getBodySkeleton)) {
      XrBodyTrackerCreateInfoFB info{XR_TYPE_BODY_TRACKER_CREATE_INFO_FB};
      info.bodyJointSet = state_->bodyJoints.size() ==
                                  XR_FULL_BODY_JOINT_COUNT_META
                              ? XR_BODY_JOINT_SET_FULL_BODY_META
                              : XR_BODY_JOINT_SET_DEFAULT_FB;
      XrResult result = state_->createBodyTracker(
          state_->session, &info, &state_->bodyTracker);
      if (result != XR_SUCCESS &&
          info.bodyJointSet == XR_BODY_JOINT_SET_FULL_BODY_META) {
        spdlog::warn("OpenXR full-body tracker creation failed: {}; trying upper body",
                     static_cast<int>(result));
        info.bodyJointSet = XR_BODY_JOINT_SET_DEFAULT_FB;
        state_->bodyJoints.resize(XR_BODY_JOINT_COUNT_FB);
        result = state_->createBodyTracker(
            state_->session, &info, &state_->bodyTracker);
      }
      if (result == XR_SUCCESS) {
        state_->bodyParents.assign(state_->bodyJoints.size(), -1);
        spdlog::info("OpenXR body tracker created with {} joints",
                     state_->bodyJoints.size());
      } else {
        spdlog::warn("OpenXR body tracker creation failed: {}",
                     static_cast<int>(result));
        state_->supportsBodyTracking = false;
        state_->bodyJoints.clear();
      }
    } else {
      spdlog::warn("OpenXR body tracking functions unavailable");
      state_->supportsBodyTracking = false;
      state_->bodyJoints.clear();
    }
  }

  XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
  std::strncpy(actionSetInfo.actionSetName, "simulator_tracking",
               XR_MAX_ACTION_SET_NAME_SIZE - 1);
  std::strncpy(actionSetInfo.localizedActionSetName, "Simulator Tracking",
               XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
  if (xrCreateActionSet(state_->instance, &actionSetInfo, &state_->actionSet) !=
      XR_SUCCESS)
    return true;
  if (xrStringToPath(state_->instance, "/user/hand/left",
                     &state_->handPaths[0]) != XR_SUCCESS ||
      xrStringToPath(state_->instance, "/user/hand/right",
                     &state_->handPaths[1]) != XR_SUCCESS)
    return true;

  XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
  actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
  actionInfo.countSubactionPaths = 2;
  actionInfo.subactionPaths = state_->handPaths.data();
  std::strncpy(actionInfo.actionName, "grip_pose", XR_MAX_ACTION_NAME_SIZE - 1);
  std::strncpy(actionInfo.localizedActionName, "Grip Pose",
               XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
  if (xrCreateAction(state_->actionSet, &actionInfo, &state_->gripAction) !=
      XR_SUCCESS)
    return true;

  constexpr std::array profiles = {
      "/interaction_profiles/khr/simple_controller",
      "/interaction_profiles/oculus/touch_controller",
      "/interaction_profiles/htc/vive_controller",
      "/interaction_profiles/valve/index_controller",
      "/interaction_profiles/microsoft/motion_controller"};
  for (const char *profileName : profiles) {
    XrPath profile = XR_NULL_PATH;
    XrPath leftGrip = XR_NULL_PATH;
    XrPath rightGrip = XR_NULL_PATH;
    if (xrStringToPath(state_->instance, profileName, &profile) != XR_SUCCESS ||
        xrStringToPath(state_->instance, "/user/hand/left/input/grip/pose",
                       &leftGrip) != XR_SUCCESS ||
        xrStringToPath(state_->instance, "/user/hand/right/input/grip/pose",
                       &rightGrip) != XR_SUCCESS)
      continue;
    const std::array bindings = {
        XrActionSuggestedBinding{state_->gripAction, leftGrip},
        XrActionSuggestedBinding{state_->gripAction, rightGrip}};
    XrInteractionProfileSuggestedBinding suggestion{
        XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    suggestion.interactionProfile = profile;
    suggestion.countSuggestedBindings = bindings.size();
    suggestion.suggestedBindings = bindings.data();
    xrSuggestInteractionProfileBindings(state_->instance, &suggestion);
  }

  XrSessionActionSetsAttachInfo attachInfo{
      XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
  attachInfo.countActionSets = 1;
  attachInfo.actionSets = &state_->actionSet;
  if (xrAttachSessionActionSets(state_->session, &attachInfo) != XR_SUCCESS)
    return true;
  state_->actionsAttached = true;
  for (size_t i = 0; i < state_->handSpaces.size(); ++i) {
    XrActionSpaceCreateInfo handInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
    handInfo.action = state_->gripAction;
    handInfo.subactionPath = state_->handPaths[i];
    handInfo.poseInActionSpace = IdentityPose();
    xrCreateActionSpace(state_->session, &handInfo, &state_->handSpaces[i]);
  }
  return true;
}

std::optional<TrackedPose>
OpenXRTrackingProvider::GetPose(std::string_view jointName) {
  const auto found = std::find(jointNames.begin(), jointNames.end(), jointName);
  if (found != jointNames.end())
    return state_->poses[static_cast<size_t>(found - jointNames.begin())];
  const auto &paths = HandJointPaths();
  for (size_t i = 0; i < paths.size(); ++i)
    if (jointName == paths[i])
      return state_->handJoints[i / XR_HAND_JOINT_COUNT_EXT]
                               [i % XR_HAND_JOINT_COUNT_EXT];
  if (state_->supportsBodyTracking)
    for (size_t i = 0; i < state_->bodyJoints.size(); ++i)
      if (jointName == bodyJointNames[i])
        return state_->bodyJoints[i];
  return std::nullopt;
}

size_t OpenXRTrackingProvider::GetJoints(std::span<std::string_view> names) {
  size_t count = 0;
  for (const auto name : jointNames) {
    if (count == names.size())
      return count;
    names[count++] = name;
  }
  if (state_->supportsHandTracking)
    for (const auto &name : HandJointPaths()) {
      if (count == names.size())
        return count;
      names[count++] = name;
    }
  if (state_->supportsBodyTracking)
    for (size_t i = 0; i < state_->bodyJoints.size(); ++i) {
      if (count == names.size())
        return count;
      names[count++] = bodyJointNames[i];
    }
  return count;
}

bool OpenXRTrackingProvider::SupportsHandTracking() const noexcept {
  return state_->supportsHandTracking;
}

bool OpenXRTrackingProvider::IsHandTracked(size_t hand) const noexcept {
  return hand < state_->handTracked.size() && state_->handTracked[hand];
}

bool OpenXRTrackingProvider::SupportsBodyTracking() const noexcept {
  return state_->supportsBodyTracking;
}

bool OpenXRTrackingProvider::IsBodyTracked() const noexcept {
  return state_->bodyTracked;
}

float OpenXRTrackingProvider::GetBodyConfidence() const noexcept {
  return state_->bodyConfidence;
}

size_t OpenXRTrackingProvider::GetBodyJointCount() const noexcept {
  return state_->supportsBodyTracking ? state_->bodyJoints.size() : 0;
}

std::string_view
OpenXRTrackingProvider::GetBodyJointName(size_t joint) const noexcept {
  return joint < GetBodyJointCount() ? bodyJointNames[joint] : std::string_view{};
}

std::optional<TrackedPose>
OpenXRTrackingProvider::GetBodyJointPose(size_t joint) const {
  return joint < GetBodyJointCount() ? state_->bodyJoints[joint] : std::nullopt;
}

int32_t OpenXRTrackingProvider::GetBodyJointParent(size_t joint) const noexcept {
  return joint < state_->bodyParents.size() ? state_->bodyParents[joint] : -1;
}

bool OpenXRTrackingProvider::SupportsPassthrough() const {
  return state_->supportsAlphaBlend;
}

bool OpenXRTrackingProvider::SupportsDepth() const {
  return state_->supportsDepth && state_->depthStarted;
}

bool OpenXRTrackingProvider::IsPassthroughEnabled() const {
  return state_->passthroughEnabled;
}

bool OpenXRTrackingProvider::SetPassthroughEnabled(bool enabled) {
  if (enabled && !SupportsPassthrough())
    return false;
  state_->passthroughEnabled = enabled;
  return true;
}

std::optional<XRDepthFrame> OpenXRTrackingProvider::GetDepthFrame() {
  if (!state_->frameBegun || !SupportsDepth())
    return std::nullopt;
  if (state_->depthAttemptedThisFrame)
    return state_->depthFrame;
  state_->depthAttemptedThisFrame = true;

  XrEnvironmentDepthImageAcquireInfoMETA acquireInfo{
      XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_ACQUIRE_INFO_META};
  acquireInfo.space = state_->localSpace;
  acquireInfo.displayTime = state_->predictedTime;
  XrEnvironmentDepthImageMETA image{XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_META};
  const XrResult result = state_->acquireDepthImage(
      state_->depthProvider, &acquireInfo, &image);
  if (result == XR_ENVIRONMENT_DEPTH_NOT_AVAILABLE_META)
    return std::nullopt;
  if (result != XR_SUCCESS ||
      image.swapchainIndex >= state_->depthImages.size()) {
    if (!state_->warnedDepthAcquire)
      spdlog::warn("OpenXR environment depth acquisition failed: {}",
                   static_cast<int>(result));
    state_->warnedDepthAcquire = true;
    return std::nullopt;
  }
  state_->warnedDepthAcquire = false;

  XRDepthFrame frame;
  frame.nativeImage = state_->depthImages[image.swapchainIndex].image;
  frame.imageIndex = image.swapchainIndex;
  frame.width = state_->depthWidth;
  frame.height = state_->depthHeight;
  frame.nearZ = image.nearZ;
  frame.farZ = image.farZ;
  frame.displayTime = state_->predictedTime;
  for (size_t eye = 0; eye < 2; ++eye) {
    const auto &view = image.views[eye];
    auto &target = frame.views[eye];
    target.pose.position = {view.pose.position.x, view.pose.position.y,
                            view.pose.position.z};
    target.pose.orientation = {view.pose.orientation.w,
                               view.pose.orientation.x,
                               view.pose.orientation.y,
                               view.pose.orientation.z};
    target.angleLeft = view.fov.angleLeft;
    target.angleRight = view.fov.angleRight;
    target.angleUp = view.fov.angleUp;
    target.angleDown = view.fov.angleDown;
  }
  state_->depthFrame = frame;
  return state_->depthFrame;
}

bool OpenXRTrackingProvider::IsRunning() const noexcept {
  return state_->running;
}

bool OpenXRTrackingProvider::HasFrameViews() const noexcept {
  return state_->viewsValid;
}

const std::array<RenderView, 2> &
OpenXRTrackingProvider::GetFrameViews() const noexcept {
  return state_->frameViews;
}

glm::ivec2
OpenXRTrackingProvider::GetRecommendedExtent(size_t eye) const noexcept {
  return state_->extents[eye];
}

void OpenXRTrackingProvider::PollEvents() {
  XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
  while (xrPollEvent(state_->instance, &event) == XR_SUCCESS) {
    if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
      const auto &changed =
          *reinterpret_cast<const XrEventDataSessionStateChanged *>(&event);
      if (changed.session == state_->session) {
        if (changed.state == XR_SESSION_STATE_READY) {
          XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
          beginInfo.primaryViewConfigurationType =
              XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
          state_->running =
              xrBeginSession(state_->session, &beginInfo) == XR_SUCCESS;
        } else if (changed.state == XR_SESSION_STATE_STOPPING) {
          state_->running = false;
          xrEndSession(state_->session);
        } else if (changed.state == XR_SESSION_STATE_EXITING ||
                   changed.state == XR_SESSION_STATE_LOSS_PENDING) {
          state_->running = false;
        }
      }
    } else if (event.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
      state_->running = false;
    }
    event = {XR_TYPE_EVENT_DATA_BUFFER};
  }
}

void OpenXRTrackingProvider::BeginFrame() {
  state_->viewsValid = false;
  state_->depthAttemptedThisFrame = false;
  state_->depthFrame.reset();
  state_->poses.fill(std::nullopt);
  state_->handTracked.fill(false);
  for (auto &joints : state_->handJoints)
    joints.fill(std::nullopt);
  state_->bodyTracked = false;
  state_->bodyConfidence = 0.0F;
  std::fill(state_->bodyJoints.begin(), state_->bodyJoints.end(), std::nullopt);
  if (!state_->running || state_->frameBegun)
    return;

  XrFrameState frame{XR_TYPE_FRAME_STATE};
  XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
  if (xrWaitFrame(state_->session, &waitInfo, &frame) != XR_SUCCESS)
    return;
  XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
  if (xrBeginFrame(state_->session, &beginInfo) != XR_SUCCESS)
    return;
  state_->frameBegun = true;
  state_->frameShouldRender = frame.shouldRender == XR_TRUE;
  state_->predictedTime = frame.predictedDisplayTime;

  XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
  locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  locateInfo.displayTime = state_->predictedTime;
  locateInfo.space = state_->localSpace;
  XrViewState viewState{XR_TYPE_VIEW_STATE};
  uint32_t viewCount = 0;
  for (auto &view : state_->locatedViews)
    view.type = XR_TYPE_VIEW;
  if (xrLocateViews(state_->session, &locateInfo, &viewState,
                    state_->locatedViews.size(), &viewCount,
                    state_->locatedViews.data()) == XR_SUCCESS &&
      viewCount == 2 &&
      (viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) &&
      (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT)) {
    state_->viewsValid = true;
    state_->frameViews[0] =
        ToRenderView("Left Eye", state_->locatedViews[0], state_->extents[0]);
    state_->frameViews[1] =
        ToRenderView("Right Eye", state_->locatedViews[1], state_->extents[1]);
  }

  const auto locate = [&](XrSpace space, size_t index) {
    if (space == XR_NULL_HANDLE)
      return;
    XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
    if (xrLocateSpace(space, state_->localSpace, state_->predictedTime,
                      &location) == XR_SUCCESS)
      state_->poses[index] = ToTrackedPose(location);
  };
  locate(state_->headSpace, 0);

  if (state_->actionsAttached) {
    const XrActiveActionSet activeSet{state_->actionSet, XR_NULL_PATH};
    XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets = &activeSet;
    if (xrSyncActions(state_->session, &syncInfo) == XR_SUCCESS) {
      for (size_t i = 0; i < state_->handSpaces.size(); ++i) {
        XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = state_->gripAction;
        getInfo.subactionPath = state_->handPaths[i];
        XrActionStatePose actionState{XR_TYPE_ACTION_STATE_POSE};
        if (xrGetActionStatePose(state_->session, &getInfo, &actionState) ==
                XR_SUCCESS &&
            actionState.isActive)
          locate(state_->handSpaces[i], i + 1);
      }
    }
  }

  if (state_->supportsHandTracking) {
    XrHandJointsLocateInfoEXT locateInfo{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
    locateInfo.baseSpace = state_->localSpace;
    locateInfo.time = state_->predictedTime;
    for (size_t hand = 0; hand < 2; ++hand) {
      if (state_->handTrackers[hand] == XR_NULL_HANDLE)
        continue;
      std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT> locations{};
      XrHandJointLocationsEXT joints{XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
      joints.jointCount = locations.size();
      joints.jointLocations = locations.data();
      const XrResult result = state_->locateHandJoints(
          state_->handTrackers[hand], &locateInfo, &joints);
      if (result != XR_SUCCESS) {
        if (!state_->warnedHandLocate[hand])
          spdlog::warn("OpenXR xrLocateHandJointsEXT hand {} failed: {}",
                       hand, static_cast<int>(result));
        state_->warnedHandLocate[hand] = true;
        continue;
      }
      state_->warnedHandLocate[hand] = false;
      if (joints.isActive != XR_TRUE)
        continue;
      state_->handTracked[hand] = true;
      for (size_t joint = 0; joint < locations.size(); ++joint) {
        auto pose = ToTrackedPose(locations[joint]);
        if (pose.positionValid || pose.orientationValid)
          state_->handJoints[hand][joint] = pose;
      }
      // A natural hand pose takes precedence over a controller grip pose.
      const auto &palm = state_->handJoints[hand][XR_HAND_JOINT_PALM_EXT];
      if (palm && palm->positionValid && palm->orientationValid)
        state_->poses[hand + 1] = palm;
    }
  }

  if (state_->supportsBodyTracking && state_->bodyTracker != XR_NULL_HANDLE) {
    XrBodyJointsLocateInfoFB locateInfo{XR_TYPE_BODY_JOINTS_LOCATE_INFO_FB};
    locateInfo.baseSpace = state_->localSpace;
    locateInfo.time = state_->predictedTime;
    std::vector<XrBodyJointLocationFB> locations(state_->bodyJoints.size());
    XrBodyJointLocationsFB body{XR_TYPE_BODY_JOINT_LOCATIONS_FB};
    body.jointCount = locations.size();
    body.jointLocations = locations.data();
    const XrResult result =
        state_->locateBodyJoints(state_->bodyTracker, &locateInfo, &body);
    if (result != XR_SUCCESS) {
      if (!state_->warnedBodyLocate)
        spdlog::warn("OpenXR xrLocateBodyJointsFB failed: {}",
                     static_cast<int>(result));
      state_->warnedBodyLocate = true;
    } else {
      state_->warnedBodyLocate = false;
      state_->bodyTracked = body.isActive == XR_TRUE;
      state_->bodyConfidence = state_->bodyTracked ? body.confidence : 0.0F;
      if (state_->bodyTracked) {
        for (size_t i = 0; i < locations.size(); ++i) {
          auto pose = ToTrackedPose(locations[i]);
          if (pose.positionValid || pose.orientationValid)
            state_->bodyJoints[i] = pose;
        }
        if (!state_->bodySkeletonReady ||
            body.skeletonChangedCount != state_->skeletonChangedCount) {
          std::vector<XrBodySkeletonJointFB> hierarchy(locations.size());
          XrBodySkeletonFB skeleton{XR_TYPE_BODY_SKELETON_FB};
          skeleton.jointCount = hierarchy.size();
          skeleton.joints = hierarchy.data();
          if (state_->getBodySkeleton(state_->bodyTracker, &skeleton) ==
              XR_SUCCESS) {
            std::fill(state_->bodyParents.begin(), state_->bodyParents.end(), -1);
            for (const auto &joint : hierarchy)
              if (joint.joint >= 0 &&
                  static_cast<size_t>(joint.joint) < state_->bodyParents.size())
                state_->bodyParents[joint.joint] = joint.parentJoint;
            state_->bodySkeletonReady = true;
            state_->skeletonChangedCount = body.skeletonChangedCount;
          }
        }
      }
    }
  }
}

void OpenXRTrackingProvider::PresentFrame(
    const std::array<uint32_t, 2> &textures) {
  if (!state_->frameBegun)
    return;

  bool copied = state_->frameShouldRender && state_->viewsValid;
  if (copied) {
    for (size_t i = 0; i < 2; ++i) {
      uint32_t index = 0;
      XrSwapchainImageAcquireInfo acquireInfo{
          XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
      if (xrAcquireSwapchainImage(state_->swapchains[i], &acquireInfo,
                                  &index) != XR_SUCCESS) {
        copied = false;
        break;
      }
      XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
      waitInfo.timeout = XR_INFINITE_DURATION;
      if (xrWaitSwapchainImage(state_->swapchains[i], &waitInfo) !=
          XR_SUCCESS) {
        copied = false;
        break;
      }
      GLuint readFramebuffer = 0;
      GLuint drawFramebuffer = 0;
      glCreateFramebuffers(1, &readFramebuffer);
      glCreateFramebuffers(1, &drawFramebuffer);
      glNamedFramebufferTexture(readFramebuffer, GL_COLOR_ATTACHMENT0,
                                textures[i], 0);
      glNamedFramebufferTexture(drawFramebuffer, GL_COLOR_ATTACHMENT0,
                                state_->swapchainImages[i][index].image, 0);
      if (glCheckNamedFramebufferStatus(readFramebuffer, GL_READ_FRAMEBUFFER) ==
              GL_FRAMEBUFFER_COMPLETE &&
          glCheckNamedFramebufferStatus(drawFramebuffer, GL_DRAW_FRAMEBUFFER) ==
              GL_FRAMEBUFFER_COMPLETE) {
        glBlitNamedFramebuffer(readFramebuffer, drawFramebuffer, 0, 0,
                               state_->extents[i].x, state_->extents[i].y, 0, 0,
                               state_->extents[i].x, state_->extents[i].y,
                               GL_COLOR_BUFFER_BIT, GL_NEAREST);
      } else {
        copied = false;
      }
      glDeleteFramebuffers(1, &readFramebuffer);
      glDeleteFramebuffers(1, &drawFramebuffer);
      glFlush();
      XrSwapchainImageReleaseInfo releaseInfo{
          XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
      if (xrReleaseSwapchainImage(state_->swapchains[i], &releaseInfo) !=
          XR_SUCCESS) {
        copied = false;
      }
      if (!copied)
        break;
    }
  }

  std::array<XrCompositionLayerProjectionView, 2> projectionViews{};
  XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  layer.layerFlags = state_->passthroughEnabled
                         ? XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT : 0;
  const XrCompositionLayerBaseHeader *layers[] = {
      reinterpret_cast<const XrCompositionLayerBaseHeader *>(&layer)};
  if (copied) {
    for (size_t i = 0; i < 2; ++i) {
      auto &view = projectionViews[i];
      view.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
      view.pose = state_->locatedViews[i].pose;
      view.fov = state_->locatedViews[i].fov;
      view.subImage.swapchain = state_->swapchains[i];
      view.subImage.imageRect.extent = {state_->extents[i].x,
                                        state_->extents[i].y};
    }
    layer.space = state_->localSpace;
    layer.viewCount = projectionViews.size();
    layer.views = projectionViews.data();
  }
  XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
  endInfo.displayTime = state_->predictedTime;
  endInfo.environmentBlendMode =
      state_->passthroughEnabled ? XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND
                                 : state_->blendMode;
  endInfo.layerCount = copied ? 1 : 0;
  endInfo.layers = copied ? layers : nullptr;
  const XrResult endResult = xrEndFrame(state_->session, &endInfo);
  if ((endResult != XR_SUCCESS ||
       (state_->frameShouldRender && state_->viewsValid && !copied)) &&
      !state_->warnedSubmission) {
    spdlog::warn("OpenXR frame submission: copied={}, xrEndFrame={}", copied,
                 static_cast<int>(endResult));
    state_->warnedSubmission = true;
  } else if (endResult == XR_SUCCESS && copied) {
    state_->warnedSubmission = false;
  }
  state_->frameBegun = false;
  state_->depthFrame.reset();
}

} // namespace ARUI::OpenXR
