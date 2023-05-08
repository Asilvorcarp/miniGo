from ctypes import CFUNCTYPE, c_int
import sys

import llvmlite.ir as ll
import llvmlite.binding as llvm
llvm.initialize()

# get ir string from file "main.o.ll"
ir = open("main.o.ll").read()

# the ModuleRef
mod = llvm.parse_assembly(ir)

mod.verify()

print(mod.triple)
print(mod.get_function("main"))

llvm.shutdown()