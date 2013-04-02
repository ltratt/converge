# Copyright (c) 2012 King's College London, created by Laurence Tratt
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


import sys
from rpython.rtyper.lltypesystem import lltype, rffi
from rpython.rtyper.tool import rffi_platform as platform
from rpython.translator.tool.cbuild import ExternalCompilationInfo
import Config
from Builtins import *



eci            = ExternalCompilationInfo(includes=["stdlib.h", "time.h", "sys/time.h"])

class CConfig:
    _compilation_info_ = eci
    TIMEVAL            = platform.Struct("struct timeval", [("tv_sec", rffi.LONG), ("tv_usec", rffi.LONG)])
    TIMEZONE           = platform.Struct("struct timezone", [])

cconfig        = platform.configure(CConfig)

TIMEVAL        = cconfig["TIMEVAL"]
TIMEVALP       = lltype.Ptr(TIMEVAL)
TIMEZONE       = cconfig["TIMEZONE"]
TIMEZONEP      = lltype.Ptr(TIMEZONE)

gettimeofday   = rffi.llexternal('gettimeofday', [TIMEVALP, TIMEZONEP], rffi.INT, compilation_info=eci)
if platform.has("random", "#include <stdlib.h>"):
    HAS_RANDOM = True
    random     = rffi.llexternal("random", [], rffi.LONG, compilation_info=eci)
    srandom    = rffi.llexternal("srandom", [rffi.INT], lltype.Void, compilation_info=eci)
else:
    HAS_RANDOM = False
    rand       = rffi.llexternal("rand", [], rffi.LONG, compilation_info=eci)
    srand      = rffi.llexternal("srand", [rffi.INT], lltype.Void, compilation_info=eci)

if platform.has("srandomdev", "#include <stdlib.h>"):
    HAS_SRANDOMDEV = True
    srandomdev = rffi.llexternal("srandomdev", [], lltype.Void, compilation_info=eci)
else:
    HAS_SRANDOMDEV = False



def init(vm):
    mod = new_c_con_module(vm, "Random", "Random", __file__, import_, \
      ["pluck", "random", "shuffle"])
    vm.set_builtin(BUILTIN_SYS_MODULE, mod)
        
    return mod


@con_object_proc
def import_(vm):
    (mod,),_ = vm.decode_args("O")

    if HAS_SRANDOMDEV and HAS_RANDOM:
        srandomdev()
    else:
        with lltype.scoped_alloc(TIMEVAL) as tp:
            if gettimeofday(tp, lltype.nullptr(TIMEZONEP.TO)) != 0:
                raise Exception("XXX")
            seed = rarithmetic.r_int(tp.c_tv_sec) ^ rarithmetic.r_int(tp.c_tv_usec)
        
        if HAS_RANDOM:
            srandom(seed)
        else:
            srand(seed)

    new_c_con_func_for_mod(vm, "pluck", pluck, mod)
    new_c_con_func_for_mod(vm, "random", random_func, mod)
    new_c_con_func_for_mod(vm, "shuffle", shuffle, mod)
    
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_gen
def pluck(vm):
    (col_o,),_ = vm.decode_args(opt="O")

    num_elems = Builtins.type_check_int(vm, vm.get_slot_apply(col_o, "len")).v
    while 1:
        if HAS_RANDOM:
            i = random() % num_elems
        else:
            i = rand() % num_elems
    
        yield vm.get_slot_apply(col_o, "get", [Con_Int(vm, i)])


@con_object_proc
def random_func(vm):
    _,_ = vm.decode_args()

    if HAS_RANDOM:
        return Con_Int(vm, random())
    else:
        return Con_Int(vm, rand())


@con_object_proc
def shuffle(vm):
    (col_o,),_ = vm.decode_args(opt="O")

    num_elems = Builtins.type_check_int(vm, vm.get_slot_apply(col_o, "len")).v
    for i in range(num_elems - 1, 0, -1):
        if HAS_RANDOM:
            j = random() % (i + 1)
        else:
            j = rand() % (i + 1)
        
        i_o = Con_Int(vm, i)
        j_o = Con_Int(vm, j)
        ith = vm.get_slot_apply(col_o, "get", [i_o])
        jth = vm.get_slot_apply(col_o, "get", [j_o])
        vm.get_slot_apply(col_o, "set", [i_o, jth])
        vm.get_slot_apply(col_o, "set", [j_o, ith])
    
    return vm.get_builtin(BUILTIN_NULL_OBJ)