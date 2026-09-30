#include "ARUI/Runtime/RuntimeTree.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace ARUI::Runtime {

RNode *RuntimeTree::Get(NodeID id) {
  const auto it = nodes_.find(id);
  return it == nodes_.end() ? nullptr : &it->second;
}

const RNode *RuntimeTree::Get(NodeID id) const {
  const auto it = nodes_.find(id);
  return it == nodes_.end() ? nullptr : &it->second;
}

const std::vector<NodeID> &RuntimeTree::RootChildren() const noexcept {
  return rootChildren_;
}

RuntimeTransaction RuntimeTree::BeginTransaction() {
  return RuntimeTransaction{*this};
}

RuntimeTransaction::RuntimeTransaction(RuntimeTree &tree)
    : tree_(&tree), nodes_(tree.nodes_), rootChildren_(tree.rootChildren_),
      nextID_(tree.nextID_), baseRevision_(tree.revision_) {}

RuntimeTransaction::RuntimeTransaction(RuntimeTransaction &&other) noexcept
    : tree_(std::exchange(other.tree_, nullptr)),
      nodes_(std::move(other.nodes_)),
      rootChildren_(std::move(other.rootChildren_)), nextID_(other.nextID_),
      baseRevision_(other.baseRevision_), finished_(other.finished_) {
  other.finished_ = true;
}

RuntimeTransaction &
RuntimeTransaction::operator=(RuntimeTransaction &&other) noexcept {
  if (this == &other)
    return *this;
  tree_ = std::exchange(other.tree_, nullptr);
  nodes_ = std::move(other.nodes_);
  rootChildren_ = std::move(other.rootChildren_);
  nextID_ = other.nextID_;
  baseRevision_ = other.baseRevision_;
  finished_ = other.finished_;
  other.finished_ = true;
  return *this;
}

void RuntimeTransaction::RequireActive() const {
  if (finished_ || tree_ == nullptr)
    throw std::logic_error("runtime transaction is no longer active");
}

bool RuntimeTransaction::ParentExists(NodeID id) const {
  return id == RootNodeID || nodes_.contains(id);
}

RNode &RuntimeTransaction::Require(NodeID id) {
  const auto it = nodes_.find(id);
  if (it == nodes_.end())
    throw std::out_of_range("runtime node does not exist");
  return it->second;
}

const RNode &RuntimeTransaction::Require(NodeID id) const {
  const auto it = nodes_.find(id);
  if (it == nodes_.end())
    throw std::out_of_range("runtime node does not exist");
  return it->second;
}

std::vector<NodeID> &RuntimeTransaction::ChildrenOf(NodeID parent) {
  return parent == RootNodeID ? rootChildren_ : Require(parent).children;
}

const std::vector<NodeID> &RuntimeTransaction::ChildrenOf(NodeID parent) const {
  return parent == RootNodeID ? rootChildren_ : Require(parent).children;
}

NodeID RuntimeTransaction::Insert(NodeID parent, const Language::LNode &node) {
  RequireActive();
  if (!ParentExists(parent))
    throw std::out_of_range("runtime parent does not exist");
  if (parent == RootNodeID && node.type != Language::LNodeType::Surface)
    throw std::invalid_argument("runtime root nodes must be surfaces");
  if (!node.children.empty())
    throw std::invalid_argument(
        "Insert accepts one node; use InsertTree for descendants");

  const NodeID id = nextID_++;
  nodes_.emplace(id, RNode{.id = id,
                           .type = node.type,
                           .surfaceType = node.surfaceType,
                           .parent = parent,
                           .attributes = node.attributes,
                           .style = node.style});
  ChildrenOf(parent).push_back(id);
  return id;
}

NodeID RuntimeTransaction::InsertTree(NodeID parent,
                                      const Language::LNode &tree) {
  RequireActive();
  Language::LNode root{.id = tree.id,
                       .type = tree.type,
                       .surfaceType = tree.surfaceType,
                       .attributes = tree.attributes,
                       .style = tree.style};
  const NodeID rootID = Insert(parent, root);
  for (const auto &child : tree.children)
    InsertTree(rootID, child);
  return rootID;
}

void RuntimeTransaction::RemoveSubtree(NodeID node) {
  const auto children = Require(node).children;
  for (const NodeID child : children)
    RemoveSubtree(child);
  nodes_.erase(node);
}

void RuntimeTransaction::Remove(NodeID node) {
  RequireActive();
  const NodeID parent = Require(node).parent;
  auto &siblings = ChildrenOf(parent);
  siblings.erase(std::find(siblings.begin(), siblings.end(), node));
  RemoveSubtree(node);
}

void RuntimeTransaction::Move(NodeID node, NodeID newParent) {
  RequireActive();
  const NodeID oldParent = Require(node).parent;
  if (!ParentExists(newParent))
    throw std::out_of_range("runtime parent does not exist");
  if (newParent == RootNodeID &&
      Require(node).type != Language::LNodeType::Surface)
    throw std::invalid_argument("runtime root nodes must be surfaces");
  if (node == newParent)
    throw std::invalid_argument("a node cannot parent itself");

  for (NodeID ancestor = newParent; ancestor != RootNodeID;
       ancestor = Require(ancestor).parent) {
    if (ancestor == node)
      throw std::invalid_argument("moving the node would create a cycle");
  }
  if (oldParent == newParent)
    return;

  auto &oldSiblings = ChildrenOf(oldParent);
  oldSiblings.erase(std::find(oldSiblings.begin(), oldSiblings.end(), node));
  ChildrenOf(newParent).push_back(node);
  Require(node).parent = newParent;
}

void RuntimeTransaction::SetStyle(NodeID node, Language::Style style) {
  RequireActive();
  Require(node).style = std::move(style);
}

void RuntimeTransaction::SetVisibility(NodeID node, bool visible) {
  RequireActive();
  Require(node).visible = visible;
}

void RuntimeTransaction::ReorderChildren(NodeID parent,
                                         std::span<const NodeID> children) {
  RequireActive();
  if (!ParentExists(parent))
    throw std::out_of_range("runtime parent does not exist");
  const auto &current = ChildrenOf(parent);
  if (children.size() != current.size())
    throw std::invalid_argument(
        "new child order must contain every child exactly once");

  const std::unordered_set<NodeID> expected(current.begin(), current.end());
  const std::unordered_set<NodeID> supplied(children.begin(), children.end());
  if (expected.size() != current.size() || supplied.size() != children.size() ||
      expected != supplied)
    throw std::invalid_argument(
        "new child order must contain every child exactly once");
  ChildrenOf(parent).assign(children.begin(), children.end());
}

void RuntimeTransaction::Commit(CommitOptions options) {
  RequireActive();
  if (tree_->revision_ != baseRevision_)
    throw std::logic_error("runtime tree changed after transaction began");

  tree_->nodes_.swap(nodes_);
  tree_->rootChildren_.swap(rootChildren_);
  tree_->nextID_ = nextID_;
  tree_->lastCommitOptions_ = options;
  ++tree_->revision_;
  finished_ = true;
}

} // namespace ARUI::Runtime
