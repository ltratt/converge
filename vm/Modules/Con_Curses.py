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


import sys
from rpython.rtyper.lltypesystem import lltype, rffi
from rpython.rtyper.tool import rffi_platform as platform
from rpython.translator.tool.cbuild import ExternalCompilationInfo
from Builtins import *



eci             = ExternalCompilationInfo(includes=["curses.h", "term.h", "unistd.h"], \
                    libraries=["curses"])


if platform.has("setupterm", "#include <curses.h>\n#include <term.h>"):
    HAVE_CURSES = True
    setupterm   = rffi.llexternal("setupterm", [rffi.CCHARP, rffi.INT, rffi.INTP], rffi.INT, \
                    compilation_info=eci)
    tigetstr    = rffi.llexternal("tigetstr", [rffi.CCHARP], rffi.CCHARP, compilation_info=eci)
else:
    HAVE_CURSES = False

class CConfig:
    _compilation_info_ = eci
    OK                 = platform.DefinedConstantInteger("OK")
    STDOUT_FILENO      = platform.DefinedConstantInteger("STDOUT_FILENO")

cconfig = platform.configure(CConfig)

OK                     = cconfig["OK"]
STDOUT_FILENO          = cconfig["STDOUT_FILENO"]



def init(vm):
    return new_c_con_module(vm, "Curses", "Curses", __file__, import_, \
      ["Curses_Exception", "setupterm", "tigetstr"])


@con_object_proc
def import_(vm):
    (mod,),_ = vm.decode_args("O")
    
    class_class = vm.get_builtin(BUILTIN_CLASS_CLASS)
    user_exception_class = vm.get_builtin(BUILTIN_EXCEPTIONS_MODULE). \
      get_defn(vm, "User_Exception")
    curses_exception = vm.get_slot_apply(class_class, "new", \
      [Con_String(vm, "Curses_Exception"), Con_List(vm, [user_exception_class]), mod])
    mod.set_defn(vm, "Curses_Exception", curses_exception)
    new_c_con_func_for_mod(vm, "setupterm", setupterm_func, mod)
    new_c_con_func_for_mod(vm, "tigetstr", tigetstr_func, mod)

    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def setupterm_func(vm):
    mod = vm.get_funcs_mod()
    (term_o, file_o), _ = vm.decode_args(opt="sO")
    
    if HAVE_CURSES:
        if term_o:
            assert isinstance(term_o, Con_String)
            raise Exception("XXX")
        else:
            term = None

        if file_o:
            fd = type_check_int(vm, vm.get_slot_apply(file_o, "fileno")).v
        else:
            fd = STDOUT_FILENO

        with lltype.scoped_alloc(rffi.INTP.TO, 1) as erret:
            if setupterm(term, fd, erret) != OK:
                ec = int(erret[0])
                if ec == -1:
                    msg = "Can't find terminfo database."
                elif ec == 0:
                    msg = "Terminal not found or not enough information known about it."
                elif ec == 1:
                    msg = "Terminal is hardcopy."
                else:
                    raise Exception("XXX")

                cex_class = mod.get_defn(vm, "Curses_Exception")
                vm.raise_(vm.get_slot_apply(cex_class, "new", [Con_String(vm, msg)]))
    else:
        raise Exception("XXX")
    
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def tigetstr_func(vm):
    mod = vm.get_funcs_mod()
    (capname_o,), _ = vm.decode_args("S")
    assert isinstance(capname_o, Con_String)

    if HAVE_CURSES:
        r = tigetstr(capname_o.v)
        if rffi.cast(rffi.LONG, r) == -1 or rffi.cast(rffi.LONG, r) == -1:
            msg = "'%s' not found or absent." % capname_o.v
            cex_class = mod.get_defn(vm, "Curses_Exception")
            vm.raise_(vm.get_slot_apply(cex_class, "new", [Con_String(vm, msg)]))
        return Con_String(vm, rffi.charp2str(r))
    else:
        raise Exception("XXX")
