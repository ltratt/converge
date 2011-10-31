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
    mod = new_c_con_module(vm, "Exceptions", "Exceptions", __file__, import_, \
      ["Exception", "User_Exception", "Internal_Exception",
       "Assert_Exception", "Import_Exception", "Mod_Defn_Exception", "Slot_Exception",
       "System_Exit_Exception", "Unassigned_Var_Exception"])
    vm.set_builtin(BUILTIN_EXCEPTIONS_MODULE, mod)
    
    return mod


def import_(vm):
    (mod,),_ = vm.decode_args("O")

    mod.set_defn(vm, "Exception", vm.get_builtin(BUILTIN_EXCEPTION_CLASS))
    _mk_simple_exception(vm, mod, "User_Exception", superclass=vm.get_builtin(BUILTIN_EXCEPTION_CLASS))
    _mk_simple_exception(vm, mod, "Internal_Exception", superclass=vm.get_builtin(BUILTIN_EXCEPTION_CLASS))

    _mk_simple_exception(vm, mod, "Assert_Exception")
    _mk_simple_exception(vm, mod, "Import_Exception", init_func=_Import_Exception_init_func)
    _mk_simple_exception(vm, mod, "Mod_Defn_Exception")
    _mk_simple_exception(vm, mod, "Slot_Exception", init_func=_Slot_Exception_init_func)
    _mk_simple_exception(vm, mod, "System_Exit_Exception", init_func=_System_Exit_Exception_init_func)
    _mk_simple_exception(vm, mod, "Unassigned_Var_Exception")

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def _mk_simple_exception(vm, mod, n, init_func=None, superclass=None):
    class_class = vm.get_builtin(BUILTIN_CLASS_CLASS)

    if superclass is None:
        superclass = mod.get_defn(vm, "Internal_Exception")
    assert isinstance(superclass, Con_Class)
    ex = vm.get_slot_apply(class_class, "new", [Con_String(vm, n), Con_List(vm, [superclass]), mod])
    if init_func is not None:
        ex.set_field(vm, "init", new_c_con_func(vm, Con_String(vm, "init"), True, init_func, ex))
    mod.set_defn(vm, n, ex)


def _Import_Exception_init_func(vm):
    (self, mod_id),_ = vm.decode_args("OS")
    assert isinstance(mod_id, Con_String)
    self.set_slot(vm, "msg", Con_String(vm, "Unable to import '%s'." % mod_id.v))
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def _Slot_Exception_init_func(vm):
    (self, n, o),_ = vm.decode_args("OSO")
    vm.type_check(n, Con_String)
    assert isinstance(n, Con_String)
    inst_n_o = vm.get_slot_apply(o.get_slot(vm, "instance_of"), "to_str")
    vm.type_check(inst_n_o, Con_String)
    assert isinstance(inst_n_o, Con_String)
    msg = "No such slot '%s' in instance of '%s'." % (n.v, inst_n_o.v)
    self.set_slot(vm, "msg", Con_String(vm, msg))
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def _System_Exit_Exception_init_func(vm):
    (self, code),_ = vm.decode_args("OO")
    self.set_slot(vm, "code", code)
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))