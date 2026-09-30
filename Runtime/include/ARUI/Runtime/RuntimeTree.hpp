#pragma once

#include "ARUI/Language/node.hpp"

#include <chrono>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace ARUI::Runtime {

using NodeID = uint64_t;
inline constexpr NodeID RootNodeID = 0;

struct RNode {
  NodeID id{};
  Language::LNodeType type{};
  Language::SurfaceType surfaceType{Language::SurfaceType::Plane};
  NodeID parent{RootNodeID};
  std::vector<NodeID> children;
  Language::Attributes attributes;
  Language::Style style;
  bool visible{true};
};

enum class Easing { Linear, EaseInOut };

struct CommitOptions {
  std::chrono::milliseconds duration{};
  Easing easing{Easing::Linear};
};

class RuntimeTransaction;

class RuntimeTree {
public:
  RuntimeTree() = default;

  [[nodiscard]] NodeID Root() const noexcept { return RootNodeID; }
  [[nodiscard]] RNode *Get(NodeID id);
  [[nodiscard]] const RNode *Get(NodeID id) const;
  [[nodiscard]] const std::vector<NodeID> &RootChildren() const noexcept;
  [[nodiscard]] uint64_t Revision() const noexcept { return revision_; }
  [[nodiscard]] const CommitOptions &LastCommitOptions() const noexcept {
    return lastCommitOptions_;
  }

  [[nodiscard]] RuntimeTransaction BeginTransaction();

private:
  friend class RuntimeTransaction;

  std::unordered_map<NodeID, RNode> nodes_;
  std::vector<NodeID> rootChildren_;
  NodeID nextID_{1};
  uint64_t revision_{};
  CommitOptions lastCommitOptions_{};
};

class RuntimeTransaction {
public:
  explicit RuntimeTransaction(RuntimeTree &tree);
  RuntimeTransaction(const RuntimeTransaction &) = delete;
  RuntimeTransaction &operator=(const RuntimeTransaction &) = delete;
  RuntimeTransaction(RuntimeTransaction &&other) noexcept;
  RuntimeTransaction &operator=(RuntimeTransaction &&other) noexcept;

  // Inserts one node. Use InsertTree when the language node has children.
  NodeID Insert(NodeID parent, const Language::LNode &node);
  NodeID InsertTree(NodeID parent, const Language::LNode &tree);
  void Remove(NodeID node);
  void Move(NodeID node, NodeID newParent);
  void SetStyle(NodeID node, Language::Style style);
  void SetVisibility(NodeID node, bool visible);
  void ReorderChildren(NodeID parent, std::span<const NodeID> children);
  void ReorderChildren(NodeID parent, const std::vector<NodeID> &children) {
    ReorderChildren(parent, std::span<const NodeID>{children});
  }

  void Commit(CommitOptions options = {});

private:
  using NodeMap = std::unordered_map<NodeID, RNode>;

  [[nodiscard]] bool ParentExists(NodeID id) const;
  [[nodiscard]] RNode &Require(NodeID id);
  [[nodiscard]] const RNode &Require(NodeID id) const;
  [[nodiscard]] std::vector<NodeID> &ChildrenOf(NodeID parent);
  [[nodiscard]] const std::vector<NodeID> &ChildrenOf(NodeID parent) const;
  void RequireActive() const;
  void RemoveSubtree(NodeID node);

  RuntimeTree *tree_{};
  NodeMap nodes_;
  std::vector<NodeID> rootChildren_;
  NodeID nextID_{};
  uint64_t baseRevision_{};
  bool finished_{};
};

} // namespace ARUI::Runtime
