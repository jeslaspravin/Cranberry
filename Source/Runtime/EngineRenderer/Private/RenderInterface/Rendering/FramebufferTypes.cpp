#include "RenderInterface/Rendering/FramebufferTypes.h"

#include <unordered_map>

bool GenericRenderPassProperties::operator==(const GenericRenderPassProperties& otherProperties) const
{
    return renderpassAttachmentFormat == otherProperties.renderpassAttachmentFormat && multisampleCount == otherProperties.multisampleCount
        && bOneRtPerFormat == otherProperties.bOneRtPerFormat;
}