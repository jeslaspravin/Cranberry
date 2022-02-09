/*!
 * \file StringRegex.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */
#pragma once

#include "String/String.h"

#include <regex>

using StringRegex = std::basic_regex<BaseString::value_type>;
using TCharMatch = std::match_results<BaseString::const_pointer>;
using StringMatch = std::match_results<BaseString::const_iterator>;
using TCharSubmatch = std::sub_match<BaseString::const_pointer>;
using StringSubmatch = std::sub_match<BaseString::const_iterator>;
