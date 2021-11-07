#include "RenderInterface/Rendering/FramebufferTypes.h"

#include <unordered_map>


namespace ERenderPassFormat
{
    String toString(Type renderpassFormat)
    {
#define CASE_MACRO(Format) case ERenderPassFormat::##Format: \
    return #Format;

        switch (renderpassFormat)
        {
            FOR_EACH_RENDERPASS_FORMAT(CASE_MACRO)
        }
        return "";
    }
}

bool GenericRenderPassProperties::operator==(const GenericRenderPassProperties& otherProperties) const
{
    return renderpassAttachmentFormat == otherProperties.renderpassAttachmentFormat && multisampleCount == otherProperties.multisampleCount
        && bOneRtPerFormat == otherProperties.bOneRtPerFormat;
}