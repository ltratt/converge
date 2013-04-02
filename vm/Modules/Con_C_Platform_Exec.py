# Copyright (c) 2011 King's College London, created by Laurence Tratt
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.


import os
from rpython.rtyper.lltypesystem import lltype, rffi
from rpython.rtyper.tool import rffi_platform as platform
from rpython.rlib import rarithmetic, rposix
from rpython.translator.tool.cbuild import ExternalCompilationInfo
from Builtins import *



eci    = ExternalCompilationInfo(includes=["stdlib.h"])

system = rffi.llexternal("system", [rffi.CCHARP], rffi.INT, compilation_info=eci)

class CConfig:
    _compilation_info_ = eci

cconfig = platform.configure(CConfig)



def init(vm):
    return new_c_con_module(vm, "C_Platform_Exec", "C_Platform_Exec", __file__, import_, \
      ["sh_cmd"])


@con_object_proc
def import_(vm):
    (mod,),_ = vm.decode_args("O")
    
    new_c_con_func_for_mod(vm, "sh_cmd", sh_cmd, mod)
    
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def sh_cmd(vm):
    (cmd_o,),_ = vm.decode_args("S")
    assert isinstance(cmd_o, Con_String)

    r = system(cmd_o.v)
    if r == -1:
        vm.raise_helper("Exception", [Con_String(vm, os.strerror(rposix.get_errno()))])

    return Con_Int(vm, os.WEXITSTATUS(r))
