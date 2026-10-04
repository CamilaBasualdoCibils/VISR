#include "ARUI/Presentation/RuntimePresentationController.hpp"

#include <stdexcept>
#include <vector>

namespace ARUI::Presentation {
namespace {
Language::LNode ToLanguageTree(const Runtime::RuntimeTree &runtime, Runtime::NodeID id) {
  const auto *source = runtime.Get(id);
  if (!source)
    throw std::out_of_range("runtime node does not exist");
  Language::LNode node{.id = source->id, .type = source->type,
                       .surfaceType = source->surfaceType,
                       .attributes = source->attributes, .style = source->style};
  for (const auto child : source->children)
    node.children.push_back(ToLanguageTree(runtime, child));
  return node;
}
}
RuntimePresentationController::RuntimePresentationController(Runtime::RuntimeTree &runtime, std::mutex &runtimeMutex) : runtime_(runtime), runtimeMutex_(runtimeMutex) {}
Language::LNode RuntimePresentationController::GetActiveTree() const {
  const std::scoped_lock lock(runtimeMutex_);
  const auto &roots = runtime_.RootChildren();
  if (roots.empty()) return Language::LSurface();
  if (roots.size() != 1) throw std::logic_error("active presentation must have one root");
  return ToLanguageTree(runtime_, roots.front());
}
void RuntimePresentationController::SetActiveTree(Language::LNode tree) {
  if (tree.type != Language::LNodeType::Surface) throw std::invalid_argument("active tree root must be a surface");
  Clear();
  AddTree(Runtime::RootNodeID, std::move(tree));
}
void RuntimePresentationController::AddTree(Language::LNodeID parent, Language::LNode tree) {
  const std::scoped_lock lock(runtimeMutex_); auto transaction = runtime_.BeginTransaction();
  transaction.InsertTree(parent, tree); transaction.Commit();
}
void RuntimePresentationController::UpdateNode(Language::LNodeID node, Language::LNode replacement) {
  const std::scoped_lock lock(runtimeMutex_); auto transaction = runtime_.BeginTransaction();
  transaction.Replace(node, replacement); transaction.Commit();
}
void RuntimePresentationController::Move(Language::LNodeID node, Language::LNodeID newParent) {
  const std::scoped_lock lock(runtimeMutex_); auto transaction = runtime_.BeginTransaction(); transaction.Move(node, newParent); transaction.Commit();
}
void RuntimePresentationController::Remove(Language::LNodeID node) {
  const std::scoped_lock lock(runtimeMutex_); auto transaction = runtime_.BeginTransaction(); transaction.Remove(node); transaction.Commit();
}
void RuntimePresentationController::Clear() {
  const std::scoped_lock lock(runtimeMutex_); const std::vector<Runtime::NodeID> roots(runtime_.RootChildren().begin(), runtime_.RootChildren().end()); auto transaction = runtime_.BeginTransaction(); for (auto root : roots) transaction.Remove(root); transaction.Commit();
}
} // namespace ARUI::Presentation
