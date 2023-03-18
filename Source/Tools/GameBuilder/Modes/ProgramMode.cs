/*!
 * \file ProgramMode.cs
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GameBuilder.Modes
{
    enum ModeExecutionResult
    {
        Success,
        Failure,
        Processing
    }

    class ProgramMode
    {
        public virtual ModeExecutionResult execute(Dictionary<string,List<string>> consoleArgs)
        {
            return ModeExecutionResult.Failure;
        }
    }
}
