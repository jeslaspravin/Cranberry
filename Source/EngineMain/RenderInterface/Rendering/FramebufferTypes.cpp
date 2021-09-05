#include "FramebufferTypes.h"

#include <unordered_map>


namespace ERenderPassFormat
{
    String toString(Type renderpassFormat)
    {
        switch (renderpassFormat)
        {
        case ERenderPassFormat::Generic:
            return "Generic";
        case ERenderPassFormat::Multibuffers:
            return "Multibuffer";
        case ERenderPassFormat::Depth:
            return "Depth";
        }
        return "";
    }
}

bool GenericRenderPassProperties::operator==(const GenericRenderPassProperties& otherProperties) const
{
    return renderpassAttachmentFormat == otherProperties.renderpassAttachmentFormat && multisampleCount == otherProperties.multisampleCount
        && bOneRtPerFormat == otherProperties.bOneRtPerFormat;
}