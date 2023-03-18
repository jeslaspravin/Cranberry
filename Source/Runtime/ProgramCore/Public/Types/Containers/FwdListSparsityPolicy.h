/*!
 * \file FwdListSparsityPolicy.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Platform/PlatformAssertionErrors.h"

#include <forward_list>

class FwdListSparsityPolicy
{
public:
    using ValueType = SizeT;
    using SparsityContainerType = std::forward_list<ValueType>;
    using SizeType = typename SparsityContainerType::size_type;

    using value_type = ValueType;
    using size_type = SizeType;

    // Will be sorted ascending, Index lists available
    SparsityContainerType sparsityTags;

private:
    void add(SizeType idx)
    {
        debugAssert(!isFree(idx)); // Double add

        if (sparsityTags.empty() || idx > sparsityTags.front())
        {
            sparsityTags.emplace_front(idx);
            sparsityTags.sort();
        }
        else
        {
            sparsityTags.emplace_front(idx);
        }
    }

public:
    void clear() { sparsityTags.clear(); }

    // Sets the value at idx to be occupied
    void set(SizeType idx)
    {
        if (sparsityTags.empty() || idx < sparsityTags.front())
        {
            return;
        }
        // lower_bound gives first itr that is not less than idx(either greater than or equal idx) or end
        // itr does it using binary search
        for (auto itr = std::lower_bound(sparsityTags.begin(), sparsityTags.end(), idx); itr != sparsityTags.end() && ((*itr) == idx);)
        {
            itr = sparsityTags.erase_after(itr);
        }
    }
    // Sets the value at idx to be free
    FORCE_INLINE void reset(SizeType idx) { add(idx); }

    FORCE_INLINE SizeT pop_free()
    {
        debugAssert(!sparsityTags.empty());
        SizeT idx = sparsityTags.front();
        sparsityTags.pop_front();
        return idx;
    }
    FORCE_INLINE void push_free(SizeType idx) { add(idx); }
    bool isFree(SizeType idx) const { return std::binary_search(sparsityTags.cbegin(), sparsityTags.cend(), idx); }
    // true if not free slots found
    FORCE_INLINE bool empty() const { return sparsityTags.empty(); }
    // Number of sparse slots
    CONST_EXPR SizeType size() const { return (SizeType)std::distance(sparsityTags.cbegin(), sparsityTags.cend()); }

    // Irrelevant interface functions
    CONST_EXPR void reserve(SizeType count) {}
    CONST_EXPR void resize(SizeType count, bool bSet = 0) {}
};