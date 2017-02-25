// ArithmeticExceptions.h (Oclgrind)
// Copyright (c) 2015, Moritz Pflanzer,
// Imperial College London. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/Plugin.h"

namespace oclgrind
{
    class ArithmeticExceptions : public Plugin
    {
        public:
            ArithmeticExceptions(const Context *context) : Plugin(context){};

            virtual void instructionExecuted(const WorkItem *workItem,
                    const llvm::Instruction *instruction,
                    const TypedValue& result) override;
            //virtual void kernelBegin(const KernelInvocation *kernelInvocation) override;
            //virtual void kernelEnd(const KernelInvocation *kernelInvocation) override;

        private:
            void logArithmeticException() const;
    };
}
