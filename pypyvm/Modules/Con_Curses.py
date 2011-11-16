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
from Builtins import *




def init(vm):
    return new_c_con_module(vm, "Curses", "Curses", __file__, import_, \
      ["Curses_Exception", "setupterm", "tigetstr"])


def import_(vm):
    (mod,),_ = vm.decode_args("O")
    
    class_class = vm.get_builtin(BUILTIN_CLASS_CLASS)
    int_exception_class = vm.get_mod("Exceptions").get_defn(vm, "Internal_Exception")
    curses_exception = vm.get_slot_apply(class_class, "new", \
      [Con_String(vm, "Curses_Exception"), Con_List(vm, [int_exception_class]), mod])
    mod.set_defn(vm, "Curses_Exception", curses_exception)
    new_c_con_func_for_mod(vm, "setupterm", setupterm, mod)
    new_c_con_func_for_mod(vm, "tigetstr", tigetstr, mod)

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def setupterm(vm):
    (term_o, file_o), _ = vm.decode_args(opt="sO")
    
    if term_o is not None:
        raise Exception("XXX")

    fd = 1
    if file_o is not None:
        file_no_o = type_check_int(vm, vm.get_slot_apply(file_o, "fileno"))
        fd = file_no_o.v

    #_curses.setupterm(None, fd)
    
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def tigetstr(vm):
    (capname_o), _ = vm.decode_args(opt="S")

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))