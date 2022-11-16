/*!
 * \file FlatTree.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Containers/BitArray.h"
#include "Types/Containers/SparseVector.h"
#include "Types/CoreDefines.h"

// Inspired from book 3D graphics rendering cook book - Chapter 7 Using data oriented design for a scene
// graph(https://www.packtpub.com/product/3d-graphics-rendering-cookbook/9781838986193) Each node
// contains index to its first child and a sibling(who will be child of node's parent)
template <typename DataType, typename IndexType = SizeT>
class FlatTree
{
public:
    using ValueType = DataType;
    using SizeType = IndexType;
    using NodeIdx = SizeType;

    using value_type = ValueType;
    using size_type = SizeType;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    CONST_EXPR static const NodeIdx InvalidIdx = ~(NodeIdx)0;

    struct Node
    {
        NodeIdx parent = InvalidIdx;
        NodeIdx firstChild = InvalidIdx;
        NodeIdx nextSibling = InvalidIdx;

        // Index of this node and its corresponding data
        NodeIdx index = InvalidIdx;
    };

private:
    SparseVector<Node, BitArraySparsityPolicy> nodes;
    std::vector<ValueType> treeData;

public:
    // Read functions
    FORCE_INLINE SizeType size() const { return (SizeType)nodes.size(); }
    NODISCARD FORCE_INLINE bool empty() const { return nodes.empty(); }
    NODISCARD FORCE_INLINE bool isValid(NodeIdx index) const { return index != InvalidIdx && nodes.isValid(index); }

    FORCE_INLINE reference operator[](NodeIdx index) noexcept
    {
        fatalAssertf(isValid(index), "Index %llu is invalid", index);
        return treeData[index];
    }
    FORCE_INLINE const_reference operator[](NodeIdx index) const noexcept
    {
        fatalAssertf(isValid(index), "Index %llu is invalid", index);
        return treeData[index];
    }
    FORCE_INLINE const Node &getNode(NodeIdx index) const noexcept
    {
        fatalAssertf(isValid(index), "Index %llu is invalid", index);
        return nodes[index];
    }

    FORCE_INLINE bool hasChild(NodeIdx parent) const { return isValid(parent) && isValid(nodes[parent].firstChild); }
    /**
     * Out list of children. Will be ordered so that parent of an object will never appear after children
     */
    void getChildren(std::vector<NodeIdx> &children, NodeIdx parent, bool bRecurse = false) const
    {
        if (!isValid(parent) || !isValid(nodes[parent].firstChild))
        {
            return;
        }
#if 0 // Minimum stack memory foot print but does depth first traversal, This is not used as well
        const Node *currNode = &nodes[nodes[parent].firstChild];
        children.emplace_back(currNode->index);
        if (bRecurse)
        {
            getChildren(children, currNode->index, bRecurse);
        }
        while (isValid(currNode->nextSibling))
        {
            children.emplace_back(currNode->nextSibling);
            if (bRecurse)
            {
                getChildren(children, currNode->nextSibling, bRecurse);
            }
            currNode = &nodes[currNode->nextSibling];
        }
#else // Little more stack memory usage but does breadth first traversal
        SizeT stageStartIdx = children.size();
        // Find all children for current parent(all node in this breadth)
        {
            const Node *currNode = &nodes[nodes[parent].firstChild];
            children.emplace_back(currNode->index);
            while (isValid(currNode->nextSibling))
            {
                children.emplace_back(currNode->nextSibling);
                currNode = &nodes[currNode->nextSibling];
            }
        }
        SizeT stageEndIdx = children.size();
        if (bRecurse)
        {
            for (SizeT idx = stageStartIdx; idx < stageEndIdx; ++idx)
            {
                getChildren(children, children[idx], true);
            }
        }
#endif
    }
    std::vector<NodeIdx> getChildren(NodeIdx parent, bool bRecurse = false) const
    {
        std::vector<NodeIdx> children;
        getChildren(children, parent, bRecurse);
        return children;
    }
    FORCE_INLINE void getAll(std::vector<NodeIdx> &outNodes) const
    {
        outNodes.reserve(outNodes.size() + size());
        SizeType num = SizeType(nodes.totalCount());
        for (NodeIdx i = 0; i < num; ++i)
        {
            if (isValid(i))
            {
                outNodes.emplace_back(i);
            }
        }
    }
    FORCE_INLINE std::vector<NodeIdx> getAll() const
    {
        std::vector<NodeIdx> outNodes;
        getAll(outNodes);
        return outNodes;
    }

    // All nodes with not parents
    FORCE_INLINE void getAllRoots(std::vector<NodeIdx> &roots) const
    {
        SizeType num = SizeType(nodes.totalCount());
        for (NodeIdx i = 0; i < num; ++i)
        {
            if (isValid(i) && !isValid(nodes[i].parent))
            {
                // If there is no parent there must not be any siblings
                debugAssert(!isValid(nodes[i].nextSibling));
                roots.emplace_back(i);
            }
        }
    }
    std::vector<NodeIdx> getAllRoots() const
    {
        std::vector<NodeIdx> roots;
        getAllRoots(roots);
        return roots;
    }

    // Modifiers
    FORCE_INLINE void clear()
    {
        nodes.clear();
        treeData.clear();
    }

    // Adds a data to tree and creates a new node for it
    NodeIdx add(const_reference value, NodeIdx parent = InvalidIdx)
    {
        NodeIdx newNodeIdx = NodeIdx(nodes.get());
        resizeDataToIndex(newNodeIdx);
        treeData[newNodeIdx] = value;

        nodes[newNodeIdx].index = newNodeIdx;

        debugAssert(parent == InvalidIdx || isValid(parent));
        if (isValid(parent))
        {
            linkChildTo(parent, newNodeIdx);
        }
        return newNodeIdx;
    }
    // Removes a node and its entire child branches
    void remove(NodeIdx nodeIdx)
    {
        if (!isValid(nodeIdx))
            return;

        Node &node = nodes[nodeIdx];
        if (isValid(node.parent))
        {
            unlinkChildFrom(node.parent, nodeIdx);
        }

        // Assuming that tree won't be very deep
        // Since removing firstChild links nextSibling to firstChild we can continue this same condition
        // check until end
        while (isValid(node.firstChild))
        {
            remove(node.firstChild);
        }

        if CONST_EXPR (std::is_destructible_v<ValueType>)
        {
            treeData[nodeIdx].~ValueType();
        }
        nodes.reset(nodeIdx);
    }

    void relinkTo(NodeIdx nodeIdx, NodeIdx newParent = InvalidIdx)
    {
        if (!isValid(nodeIdx) || nodes[nodeIdx].parent == newParent)
            return;

        Node &node = nodes[nodeIdx];
        if (isValid(node.parent))
        {
            unlinkChildFrom(node.parent, nodeIdx);
        }

        if (isValid(newParent))
        {
            linkChildTo(newParent, nodeIdx);
        }
    }

    friend FORCE_INLINE OutputStream &operator<<(OutputStream &stream, const FlatTree &tree)
    {
        stream << '\n';

        String str{ TCHAR("    ") };
        std::vector<NodeIdx> allRoots = tree.getAllRoots();
        for (NodeIdx nodeIdx : allRoots)
        {
            tree.printTree(stream, nodeIdx, str);
        }
        return stream;
    }

private:
    FORCE_INLINE void resizeDataToIndex(NodeIdx idx)
    {
        if (idx >= treeData.size())
            treeData.resize(idx + 1);
    }

    // Removes child from parent's child list, Assumes both parent and child idx are valid
    void unlinkChildFrom(NodeIdx parentIdx, NodeIdx childIdx)
    {
        Node *currNode = &nodes[parentIdx];
        // If parent has no child leave
        if (currNode->firstChild == InvalidIdx)
            return;
        // If first child is the child we are looking for?
        if (currNode->firstChild == childIdx)
        {
            Node &sibling = nodes[currNode->firstChild];
            currNode->firstChild = sibling.nextSibling;
            sibling.nextSibling = InvalidIdx; // Mark as invalid index, As if we are just changing
                                              // hierarchy it is important
            sibling.parent = InvalidIdx;
            return;
        }

        // First child is not the expected so go through the nextSiblings until found/end
        currNode = &nodes[currNode->firstChild];
        while (currNode->nextSibling != InvalidIdx)
        {
            if (currNode->nextSibling == childIdx)
            {
                Node &sibling = nodes[currNode->nextSibling];
                currNode->nextSibling = sibling.nextSibling;
                sibling.nextSibling = InvalidIdx; // Mark as invalid index, As if we are just
                                                  // changing hierarchy it is important
                sibling.parent = InvalidIdx;
                return;
            }
            currNode = &nodes[currNode->nextSibling];
        }
    }
    void linkChildTo(NodeIdx parentIdx, NodeIdx childIdx)
    {
        nodes[childIdx].parent = parentIdx;
        nodes[childIdx].nextSibling = InvalidIdx; // Just being double secure

        Node *currNode = &nodes[parentIdx];
        // No child for this parent
        if (currNode->firstChild == InvalidIdx)
        {
            currNode->firstChild = childIdx;
            return;
        }

        currNode = &nodes[currNode->firstChild];
        while (currNode->nextSibling != InvalidIdx)
        {
            currNode = &nodes[currNode->nextSibling];
        }
        currNode->nextSibling = childIdx;
    }

    void printTree(OutputStream &stream, NodeIdx parent, const String &prefix) const;
};

template <typename DataType, typename IndexType>
void FlatTree<DataType, IndexType>::printTree(OutputStream &stream, NodeIdx parent, const String &prefix) const
{
    stream << prefix << parent << '\n';
    String newPrefix(prefix + TCHAR("|    "));
    std::vector<NodeIdx> children = getChildren(parent);
    for (NodeIdx nodeIdx : children)
    {
        printTree(stream, nodeIdx, newPrefix);
    }
}
