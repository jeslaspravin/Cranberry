using System;
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

        public virtual ModeExecutionResult execute(string[] args)
        {
            return ModeExecutionResult.Failure;
        }
    }
}
