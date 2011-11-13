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
    mod = new_c_con_module(vm, "Sys", "Sys", __file__, import_, \
      ["print", "println", "stdin", "stdout", "stderr", "vm_path", "program_path", "argv", \
       "version", "version_date", "exit"])
    vm.set_builtin(BUILTIN_SYS_MODULE, mod)
        
    return mod


def import_(vm):
    (mod,),_ = vm.decode_args("O")

    mod.set_defn(vm, "argv", Con_List(vm, [Con_String(vm, x) for x in vm.argv[2:]]))

    new_c_con_func_for_mod(vm, "exit", exit, mod)
    new_c_con_func_for_mod(vm, "println", println, mod)
    
    file_mod = vm.get_builtin(BUILTIN_C_FILE_MODULE)
    file_class = file_mod.get_defn(vm, "File")
    mod.set_defn(vm, "stdin", \
      vm.get_slot_apply(file_class, "new", [Con_Int(vm, 0), Con_String(vm, "r")]))
    mod.set_defn(vm, "stdout", \
      vm.get_slot_apply(file_class, "new", [Con_Int(vm, 1), Con_String(vm, "w")]))
    mod.set_defn(vm, "stderr", \
      vm.get_slot_apply(file_class, "new", [Con_Int(vm, 2), Con_String(vm, "w")]))
    
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def exit(vm):
    (c,),_ = vm.decode_args(opt="I")

    raise vm.raise_helper("System_Exit_Exception", [c])


def println(vm):
    _,vargs = vm.decode_args(vargs=True)
    for o in vargs:
        if isinstance(o, Con_String):
            print o.v,
        else:
            s = type_check_string(vm, vm.get_slot_apply(o, "to_str"))
            print s.v,
    print ""

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))