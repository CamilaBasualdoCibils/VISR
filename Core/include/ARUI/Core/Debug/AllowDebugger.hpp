#pragma once
#include <cstdlib>
#if defined(__linux__)
#include <sys/prctl.h>
#endif

namespace ARUI::Debug {
inline void AllowConfiguredDebuggerAttach() noexcept {
#if defined(__linux__)
  if (const char *enabled = std::getenv("ARUI_GDB_ATTACH");
      enabled && enabled[0] == '1')
    (void)prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);
#endif
}
} // namespace ARUI::Debug
