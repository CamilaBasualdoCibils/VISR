#include "VISR/Render/RenderGraph.hpp"

#include "VISR/Render/IRenderDevice.hpp"

namespace VISR::Render {

void RenderGraph::Compile() { m_frameGraph.compile(); }

void RenderGraph::Execute(IRenderDevice &device) {
  m_frameGraph.execute(&device);
}

} // namespace VISR::Render
