/*!
 * \file ArchiveTypes.h
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include <type_traits>
#include <concepts>

class ArchiveBase;

template <typename Type>
concept ArchiveTypeName = std::is_base_of_v<ArchiveBase, Type>;

template <typename Type, typename ArchiveType>
concept ArchivableType = ArchiveTypeName<ArchiveType> && requires(Type value, ArchiveType archive) {
    {
        archive << value
    } -> std::convertible_to<ArchiveType &>;
};