#pragma once

#include <type_traits>

template <typename DataType>
using IsIndexable = std::disjunction<std::is_array<DataType>, std::is_pointer<DataType>>;
template <typename DataType>
using IndexableType = std::conditional_t<std::is_pointer_v<DataType>, std::remove_pointer_t<DataType>, std::remove_all_extents_t<DataType>>;
