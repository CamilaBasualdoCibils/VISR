#include "ARUI/Presentation/RuntimePresentationController.hpp"

#include <vector>

namespace ARUI::Presentation {
namespace {
Runtime::CommitOptions CommitOptions(Transition transition) {
  return {.duration = transition.duration,
          .easing = transition.easing == Easing::EaseInOut
                        ? Runtime::Easing::EaseInOut
                        : Runtime::Easing::Linear};
}
} // namespace

RuntimePresentationController::RuntimePresentationController(
    Runtime::RuntimeTree &runtime, std::mutex &runtimeMutex)
    : runtime_(runtime), runtimeMutex_(runtimeMutex) {}

PresentationNodeID
RuntimePresentationController::CreateNode(PresentationNodeID parent,
                                          const Language::LNode &node,
                                          Transition transition) {
  const std::scoped_lock lock(runtimeMutex_);
  auto transaction = runtime_.BeginTransaction();
  const auto id = transaction.Insert(parent, node);
  transaction.Commit(CommitOptions(transition));
  return id;
}
PresentationNodeID
RuntimePresentationController::CreateTree(PresentationNodeID parent,
                                          const Language::LNode &tree,
                                          Transition transition) {
  const std::scoped_lock lock(runtimeMutex_);
  auto transaction = runtime_.BeginTransaction();
  const auto id = transaction.InsertTree(parent, tree);
  transaction.Commit(CommitOptions(transition));
  return id;
}
void RuntimePresentationController::Remove(PresentationNodeID node,
                                           Transition transition) {
  const std::scoped_lock lock(runtimeMutex_);
  auto transaction = runtime_.BeginTransaction();
  transaction.Remove(node);
  transaction.Commit(CommitOptions(transition));
}
void RuntimePresentationController::Move(PresentationNodeID node,
                                         PresentationNodeID newParent,
                                         Transition transition) {
  const std::scoped_lock lock(runtimeMutex_);
  auto transaction = runtime_.BeginTransaction();
  transaction.Move(node, newParent);
  transaction.Commit(CommitOptions(transition));
}
void RuntimePresentationController::SetStyle(PresentationNodeID node,
                                             Language::Style style,
                                             Transition transition) {
  const std::scoped_lock lock(runtimeMutex_);
  auto transaction = runtime_.BeginTransaction();
  transaction.SetStyle(node, std::move(style));
  transaction.Commit(CommitOptions(transition));
}
void RuntimePresentationController::SetVisibility(PresentationNodeID node,
                                                  bool visible,
                                                  Transition transition) {
  const std::scoped_lock lock(runtimeMutex_);
  auto transaction = runtime_.BeginTransaction();
  transaction.SetVisibility(node, visible);
  transaction.Commit(CommitOptions(transition));
}
void RuntimePresentationController::ReorderChildren(
    PresentationNodeID parent, std::span<const PresentationNodeID> children,
    Transition transition) {
  const std::scoped_lock lock(runtimeMutex_);
  auto transaction = runtime_.BeginTransaction();
  transaction.ReorderChildren(parent, children);
  transaction.Commit(CommitOptions(transition));
}
void RuntimePresentationController::Reset(Transition transition) {
  const std::scoped_lock lock(runtimeMutex_);
  const std::vector<Runtime::NodeID> roots(runtime_.RootChildren().begin(),
                                           runtime_.RootChildren().end());
  auto transaction = runtime_.BeginTransaction();
  for (const auto root : roots)
    transaction.Remove(root);
  transaction.Commit(CommitOptions(transition));
}
} // namespace ARUI::Presentation
