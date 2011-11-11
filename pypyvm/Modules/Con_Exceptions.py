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
       "Assert_Exception", "File_Exception", "Import_Exception", "Key_Exception", \
       "Mod_Defn_Exception", "Parameters_Exception", "Slot_Exception", "System_Exit_Exception", \
       "Type_Exception", "Unassigned_Var_Exception"])
    vm.set_builtin(BUILTIN_EXCEPTIONS_MODULE, mod)
    
    return mod


def import_(vm):
    (mod,),_ = vm.decode_args("O")

    mod.set_defn(vm, "Exception", vm.get_builtin(BUILTIN_EXCEPTION_CLASS))
    _mk_simple_exception(vm, mod, "User_Exception", superclass=vm.get_builtin(BUILTIN_EXCEPTION_CLASS))
    _mk_simple_exception(vm, mod, "Internal_Exception", superclass=vm.get_builtin(BUILTIN_EXCEPTION_CLASS))

    _mk_simple_exception(vm, mod, "Assert_Exception")
    _mk_simple_exception(vm, mod, "File_Exception")
    _mk_simple_exception(vm, mod, "Import_Exception", init_func=_Import_Exception_init_func)
    _mk_simple_exception(vm, mod, "Key_Exception", init_func=_Key_Exception_init_func)
    _mk_simple_exception(vm, mod, "Mod_Defn_Exception")
    _mk_simple_exception(vm, mod, "Parameters_Exception")
    _mk_simple_exception(vm, mod, "Slot_Exception", init_func=_Slot_Exception_init_func)
    _mk_simple_exception(vm, mod, "System_Exit_Exception", init_func=_System_Exit_Exception_init_func)
    _mk_simple_exception(vm, mod, "Type_Exception", init_func=_Type_Exception_init_func)
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


def _Key_Exception_init_func(vm):
    (self, k),_ = vm.decode_args("OO")
    k_s = type_check_string(vm, vm.get_slot_apply(k, "to_str"))
    self.set_slot(vm, "msg", Con_String(vm, "Key '%s' not found." % k_s.v))
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def _Slot_Exception_init_func(vm):
    (self, n, o),_ = vm.decode_args("OSO")
    assert isinstance(n, Con_String)
    name = type_check_string(vm, o.get_slot(vm, "instance_of").get_slot(vm, "name"))
    msg = "No such slot '%s' in instance of '%s'." % (n.v, name.v)
    self.set_slot(vm, "msg", Con_String(vm, msg))
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def _System_Exit_Exception_init_func(vm):
    (self, code),_ = vm.decode_args("OO")
    self.set_slot(vm, "code", code)
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def _Type_Exception_init_func(vm):
    (self, should_be, o, extra),_ = vm.decode_args(mand="OOO", opt="O")
    if extra:
        msg = "Expected '%s' to be conformant to " % \
          type_check_string(vm, extra).v
    else:
        msg = "Expected to be conformant to "
    msg += type_check_string(vm, vm.get_slot_apply(should_be, "path")).v
    o_path = type_check_string(vm, vm.get_slot_apply(o.get_slot(vm, "instance_of"), "path"))
    msg += ", but got instance of %s." % o_path.v
    self.set_slot(vm, "msg", Con_String(vm, msg))
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))