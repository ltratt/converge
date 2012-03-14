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


from pypy.rlib import rarithmetic
from pypy.rpython.lltypesystem import lltype, rffi
from pypy.rpython.tool import rffi_platform as platform
from pypy.translator.tool.cbuild import ExternalCompilationInfo
from Builtins import *



eci             = ExternalCompilationInfo(includes=["time.h", "sys/time.h"])

class CConfig:
    _compilation_info_ = eci
    TIMEVAL            = platform.Struct("struct timeval", [("tv_sec", rffi.LONG), ("tv_usec", rffi.LONG)])
    TIMEZONE           = platform.Struct("struct timezone", [])
    TIMESPEC           = platform.Struct("struct timespec", [("tv_sec", rffi.TIME_T), ("tv_nsec", rffi.LONG)])
    CLOCK_MONOTONIC    = platform.DefinedConstantInteger("CLOCK_MONOTONIC")

cconfig         = platform.configure(CConfig)

TIMEVAL         = cconfig["TIMEVAL"]
TIMEVALP        = lltype.Ptr(TIMEVAL)
TIMEZONE        = cconfig["TIMEZONE"]
TIMEZONEP       = lltype.Ptr(TIMEZONE)
TIMESPEC        = cconfig["TIMESPEC"]
TIMESPECP       = lltype.Ptr(TIMESPEC)
CLOCK_MONOTONIC = cconfig["CLOCK_MONOTONIC"]

gettimeofday    = rffi.llexternal('gettimeofday', [TIMEVALP, TIMEZONEP], rffi.INT, compilation_info=eci)
if platform.has("clock_gettime", "#include <sys/time.h>"):
    HAS_CLOCK_GETTIME = True
    clock_gettime   = rffi.llexternal('clock_gettime', [rffi.INT, TIMESPECP], rffi.INT, compilation_info=eci)
else:
    HAS_CLOCK_GETTIME = False


def init(vm):
    return new_c_con_module(vm, "C_Time", "C_Time", __file__, import_, \
      ["current", "current_mono"])


@con_object_proc
def import_(vm):
    (mod,),_ = vm.decode_args("O")
    
    new_c_con_func_for_mod(vm, "current", current, mod)
    if HAS_CLOCK_GETTIME:
        new_c_con_func_for_mod(vm, "current_mono", current_mono, mod)
    else:
        # OS X, and maybe other OSs, doesn't have clock_gettime, so we fall back on a less accurate
        # timing method as being better than nothing.
        new_c_con_func_for_mod(vm, "current_mono", current, mod)

    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def current(vm):
    _,_ = vm.decode_args()

    with lltype.scoped_alloc(TIMEVAL) as tp:
        if gettimeofday(tp, lltype.nullptr(TIMEZONEP.TO)) != 0:
            raise Exception("XXX")
        sec = int(tp.c_tv_sec)
        usec = int(tp.c_tv_usec)

    return Con_List(vm, [Con_Int(vm, sec), Con_Int(vm, usec * 1000)])


@con_object_proc
def current_mono(vm):
    _,_ = vm.decode_args()

    with lltype.scoped_alloc(TIMESPEC) as ts:
        if clock_gettime(CLOCK_MONOTONIC, ts) != 0:
            raise Exception("XXX")
        sec = rarithmetic.r_int(ts.c_tv_sec)
        nsec = rarithmetic.r_int(ts.c_tv_nsec)

    return Con_List(vm, [Con_Int(vm, sec), Con_Int(vm, nsec)])
