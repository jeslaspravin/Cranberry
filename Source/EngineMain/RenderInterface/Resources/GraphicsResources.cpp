#include "GraphicsResources.h"
#include "../../Core/String/String.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../../Core/Logger/Logger.h"

ResourceTypesGraph::TypeNode recursivelyInsert(const GraphicsResourceType* type, const GraphicsResourceType* upUntil
    , ResourceTypesGraph::TypeNode* childNode = nullptr)
{
    ResourceTypesGraph::TypeNode newNode;
    newNode.type = type;
    if (childNode)
    {
        newNode.childs.push_back(*childNode);
    }
    if (type != upUntil && !type->isRootType())
    {
        return recursivelyInsert(type->getParent(), upUntil, &newNode);
    }
    return newNode;
}

void ResourceTypesGraph::lazyInsert(const GraphicsResourceType* type)
{
    insertWaitQueue.push_back(type);
}

void ResourceTypesGraph::insertType(const GraphicsResourceType* type, TypeNode* fromNode /*= nullptr*/)
{
    if (fromNode == nullptr)
    {
        if (root.type == nullptr)
        {
            // Insert entire tree in first insert
            root = recursivelyInsert(type, nullptr);
            return;
        }
        // Initiate walking down the graph to find furthest possible parent that is already inserted
        insertType(type, &root);
    }
    else
    {
        for (TypeNode& node : fromNode->childs)
        {
            if (type->isChildOf(node.type))
            {
                // If parent of type then use that as next node and continue
                return insertType(type, &node);
            }
        }
        // Add nodes from type to furthest parent
        TypeNode nodeToMerge = recursivelyInsert(type, fromNode->type);
        debugAssert(nodeToMerge.type == fromNode->type);
        fromNode->childs.insert(fromNode->childs.end(), nodeToMerge.childs.cbegin(), nodeToMerge.childs.cend());
    }
}

void ResourceTypesGraph::graphAllChilds(TypeNode* fromNode, std::vector<const GraphicsResourceType*>& outChilds, bool bRecursively) const
{
    outChilds.reserve(outChilds.size() + fromNode->childs.size());
    for (TypeNode& child : fromNode->childs)
    {
        outChilds.push_back(child.type);
    }
    if (bRecursively)
    {
        for (TypeNode& child : fromNode->childs)
        {
            graphAllChilds(&child, outChilds,bRecursively);
        }
    }
}

void ResourceTypesGraph::findChildsOf(const GraphicsResourceType* type, std::vector<const GraphicsResourceType*>& outChilds, bool bRecursively /*= false*/)
{
    // TODO(Jeslas) : Remove this to some sort of latent task at engine startup and make this function const
    if (insertWaitQueue.size() > 0)
    {
        for (const GraphicsResourceType* resourceType : insertWaitQueue)
        {
            insertType(resourceType);
        }
        insertWaitQueue.clear();
    }

    TypeNode* node = &root;
    while (!node->isLeaf() && node->type != type)
    {
        for (TypeNode& child : node->childs)
        {
            if (type->isChildOf(child.type))
            {
                node = &child;
                break;
            }
        }
    }
    graphAllChilds(node, outChilds, bRecursively);
}

void GraphicsResourceType::registerResource(GraphicsResource* resource)
{
    unregisterResource(resource);
    registeredResources.push_front(resource);
}

void GraphicsResourceType::unregisterResource(GraphicsResource* resource)
{
    registeredResources.remove(resource);
}

void GraphicsResourceType::allRegisteredResources(std::vector<GraphicsResource*>& outResources) const
{
    outResources.insert(outResources.end(), registeredResources.cbegin(), registeredResources.cend());
}

void GraphicsResourceType::allChildDefaultResources(std::vector<GraphicsResource*>& outResources, bool bRecursively /*= false*/) const
{
    std::vector<const GraphicsResourceType*> childResourceTypes;
    getTypeGraph().findChildsOf(this, childResourceTypes, bRecursively);

    outResources.reserve(outResources.size() + childResourceTypes.size());
    for (const GraphicsResourceType* type : childResourceTypes)
    {
        outResources.push_back(type->getDefault());
    }
}

bool GraphicsResourceType::isChildOf(const GraphicsResourceType* otherType) const
{
    return this == otherType || verifyParent(otherType);
}

ResourceTypesGraph& GraphicsResourceType::getTypeGraph() const
{
    static ResourceTypesGraph typeGraph;
    return typeGraph;
}

GraphicsResourceType::GraphicsResourceType(GraphicsResource* resource, DeleteFn deleteFunc)
{
    defaultResource = resource;
    deleteResource = deleteFunc;
    getTypeGraph().lazyInsert(this);
}

GraphicsResourceType::~GraphicsResourceType()
{
    if (defaultResource)
    {
        (*deleteResource)(defaultResource);
    }
}

DEFINE_GRAPHICS_RESOURCE(GraphicsResource)

void GraphicsResource::reinitResources()
{
    privateType()->registerResource(this);
}

void GraphicsResource::release()
{
    privateType()->unregisterResource(this);
}

String GraphicsResource::getResourceName() const
{
    return "";
}
