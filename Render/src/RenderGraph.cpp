#include "ARUI/Render/RenderGraph.hpp"

#include "ARUI/Render/IRenderDevice.hpp"

namespace ARUI::Render {

void RenderGraph::Compile() { m_frameGraph.compile(); }

void RenderGraph::Execute(IRenderDevice &device) {
  m_frameGraph.execute(&device);
}

} // namespace ARUI::Render
