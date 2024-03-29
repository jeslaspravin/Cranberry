# 
# \file StringUtilities.cmake
# 
# \author Jeslas Pravin
# \date January 2022
# \copyright
#  Copyright (C) Jeslas Pravin, 2022-2023
#  @jeslaspravin pravinjeslas@gmail.com
#  License can be read in LICENSE file at this repository's root
# 

# Makes regex search pattern to match one of values in list
function (make_match_any_pattern_from_list)
    set(one_value_args OUT_PATTERN)
    set(multi_value_args LIST)
    cmake_parse_arguments(match_any_pattern "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (list_merged "")
    foreach (item ${match_any_pattern_LIST})
        string (APPEND list_merged "${item} ")
    endforeach ()

    string (STRIP ${list_merged} temp)
    string (REPLACE " " "|" pattern ${temp})
    
    set (${match_any_pattern_OUT_PATTERN} "(${pattern})" PARENT_SCOPE)
endfunction ()