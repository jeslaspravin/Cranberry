/*!
 * \file SmartPointers.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include <memory>

template <typename T>
using SharedPtr = std::shared_ptr<T>;

template <typename T, typename DeleterType = std::default_delete<T>>
using UniquePtr = std::unique_ptr<T, DeleterType>;

template <typename T>
using WeakPtr = std::weak_ptr<T>;