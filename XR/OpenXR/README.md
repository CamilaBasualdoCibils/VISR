# OpenXR backend

The OpenXR target is graphics-backend neutral. `OpenXRTrackingProvider` owns
only OpenXR instance/session/frame, space, tracking, environment, and swapchain
lifecycle. `OpenXRPresenter` coordinates frames and accepts an injected
`IOpenXRGraphicsBinding`; neither class includes or links GLFW, EGL, OpenGL,
GLEW, Vulkan, or a concrete ARUI render backend.

Graphics API integration lives outside the OpenXR target. The current
`ARUI::RenderOpenGLOpenXR` adapter is owned by the OpenGL backend integration:
it creates the API-specific session binding, stores native swapchain images,
and copies renderer output. The OpenGL render device itself owns its surfaceless
EGL context. A future Vulkan backend can implement the same binding contract
without changing OpenXR runtime code.
