/*!
 * \file LogFile.cs
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace GameBuilder.Logger
{
    class LogFile:LogBase
    {
        public LogFile(string filePath)
        {
            FileStream fileStream = File.Create(filePath);
            fileStream.Close();
            File.SetCreationTime(filePath, DateTime.Now);

            fileStream = File.OpenWrite(filePath);
            writer = new StreamWriter(fileStream);
            //writer.AutoFlush = true;
        }
    }
}
