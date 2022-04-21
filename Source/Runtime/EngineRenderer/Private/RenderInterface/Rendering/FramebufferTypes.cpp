/*!
 * \file FramebufferTypes.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Rendering/FramebufferTypes.h"

#include <unordered_map>

bool GenericRenderPassProperties::operator==(const GenericRenderPassProperties &otherProperties) const
{
    return renderpassAttachmentFormat == otherProperties.renderpassAttachmentFormat && multisampleCount == otherProperties.multisampleCount
           && bOneRtPerFormat == otherProperties.bOneRtPerFormat;
}