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
      ["Exception",
       "Internal_Exception",
         "VM_Exception", "System_Exit_Exception",
       "User_Exception",
         "Apply_Exception", "Assert_Exception", "Bounds_Exception", "Field_Exception",
         "Import_Exception", "Indices_Exception", "Key_Exception", "Mod_Defn_Exception",
         "NDIf_Exception", "Number_Exception", "Parameters_Exception", "Slot_Exception",
         "Type_Exception", "Unassigned_Var_Exception", "Unpack_Exception",
       "IO_Exception",
         "File_Exception"])
    vm.set_builtin(BUILTIN_EXCEPTIONS_MODULE, mod)
    
    return mod


@con_object_proc
def import_(vm):
    (mod,),_ = vm.decode_args("O")

    mod.set_defn(vm, "Exception", vm.get_builtin(BUILTIN_EXCEPTION_CLASS))

    internal_exception = _mk_simple_exception(vm, mod, "Internal_Exception", \
      superclass=vm.get_builtin(BUILTIN_EXCEPTION_CLASS))
    _mk_simple_exception(vm, mod, "System_Exit_Exception", \
      init_func=_System_Exit_Exception_init_func, superclass=internal_exception)
    _mk_simple_exception(vm, mod, "VM_Exception", superclass=internal_exception)

    _mk_simple_exception(vm, mod, "User_Exception", superclass=vm.get_builtin(BUILTIN_EXCEPTION_CLASS))
    _mk_simple_exception(vm, mod, "Apply_Exception", init_func=_Apply_Exception_init_func)
    _mk_simple_exception(vm, mod, "Assert_Exception")
    _mk_simple_exception(vm, mod, "Bounds_Exception", init_func=_Bounds_Exception_init_func)
    _mk_simple_exception(vm, mod, "Field_Exception", init_func=_Field_Exception_init_func)
    _mk_simple_exception(vm, mod, "Import_Exception", init_func=_Import_Exception_init_func)
    _mk_simple_exception(vm, mod, "Indices_Exception", init_func=_Indices_Exception_init_func)
    _mk_simple_exception(vm, mod, "Key_Exception", init_func=_Key_Exception_init_func)
    _mk_simple_exception(vm, mod, "Mod_Defn_Exception")
    _mk_simple_exception(vm, mod, "NDIf_Exception")
    _mk_simple_exception(vm, mod, "Number_Exception", init_func=_Number_Exception_init_func)
    _mk_simple_exception(vm, mod, "Parameters_Exception")
    _mk_simple_exception(vm, mod, "Slot_Exception", init_func=_Slot_Exception_init_func)
    _mk_simple_exception(vm, mod, "Type_Exception", init_func=_Type_Exception_init_func)
    _mk_simple_exception(vm, mod, "Unassigned_Var_Exception")
    _mk_simple_exception(vm, mod, "Unpack_Exception", init_func=_Unpack_Exception_init_func)

    io_exception = _mk_simple_exception(vm, mod, "IO_Exception")
    _mk_simple_exception(vm, mod, "File_Exception", superclass=io_exception)

    return vm.get_builtin(BUILTIN_NULL_OBJ)


def _mk_simple_exception(vm, mod, n, init_func=None, superclass=None):
    class_class = vm.get_builtin(BUILTIN_CLASS_CLASS)

    if superclass is None:
        superclass = mod.get_defn(vm, "Internal_Exception")
    assert isinstance(superclass, Con_Class)
    ex = vm.get_slot_apply(class_class, "new", [Con_String(vm, n), Con_List(vm, [superclass]), mod])
    if init_func is not None:
        ex.set_field(vm, "init", new_c_con_func(vm, Con_String(vm, "init"), True, init_func, ex))
    mod.set_defn(vm, n, ex)
    return ex


@con_object_proc
def _Apply_Exception_init_func(vm):
    (self, o_o),_ = vm.decode_args("OO")
    p = type_check_string(vm, vm.get_slot_apply(o_o.get_slot(vm, "instance_of"), "path")).v
    self.set_slot(vm, "msg", Con_String(vm, "Do not know how to apply instance of '%s'." % p))
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Bounds_Exception_init_func(vm):
    (self, idx_o, upper_o),_ = vm.decode_args("OII")
    assert isinstance(idx_o, Con_Int)
    assert isinstance(upper_o, Con_Int)
    if idx_o.v < 0:
        msg = "%d below lower bound 0." % idx_o.v
    else:
        msg = "%d exceeds upper bound %d." % (idx_o.v, upper_o.v)
    self.set_slot(vm, "msg", Con_String(vm, msg))
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Field_Exception_init_func(vm):
    (self, n_o, class_o),_ = vm.decode_args("OSO")
    assert isinstance(n_o, Con_String)
    classp = type_check_string(vm, vm.get_slot_apply(class_o, "path")).v
    msg = "No such field '%s' in class '%s'." % (n_o.v, classp)
    self.set_slot(vm, "msg", Con_String(vm, msg))
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Import_Exception_init_func(vm):
    (self, mod_id),_ = vm.decode_args("OS")
    assert isinstance(mod_id, Con_String)
    self.set_slot(vm, "msg", Con_String(vm, "Unable to import '%s'." % mod_id.v))
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Indices_Exception_init_func(vm):
    (self, lower_o, upper_o),_ = vm.decode_args("OII")
    assert isinstance(lower_o, Con_Int)
    assert isinstance(upper_o, Con_Int)
    msg = "Lower bound %d exceeds upper bound %d" % (lower_o.v, upper_o.v)
    self.set_slot(vm, "msg", Con_String(vm, msg))
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Key_Exception_init_func(vm):
    (self, k),_ = vm.decode_args("OO")
    k_s = type_check_string(vm, vm.get_slot_apply(k, "to_str"))
    self.set_slot(vm, "msg", Con_String(vm, "Key '%s' not found." % k_s.v))
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Number_Exception_init_func(vm):
    (self, o),_ = vm.decode_args("OO")
    o_s = type_check_string(vm, vm.get_slot_apply(o, "to_str"))
    self.set_slot(vm, "msg", Con_String(vm, "Number '%s' not valid." % o_s.v))
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Slot_Exception_init_func(vm):
    (self, n, o),_ = vm.decode_args("OSO")
    assert isinstance(n, Con_String)
    name = type_check_string(vm, o.get_slot(vm, "instance_of").get_slot(vm, "name"))
    msg = "No such slot '%s' in instance of '%s'." % (n.v, name.v)
    self.set_slot(vm, "msg", Con_String(vm, msg))
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _System_Exit_Exception_init_func(vm):
    (self, code),_ = vm.decode_args("OO")
    self.set_slot(vm, "code", code)
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Type_Exception_init_func(vm):
    (self, should_be, o, extra),_ = vm.decode_args(mand="OSO", opt="O")
    assert isinstance(should_be, Con_String)
    if extra is not None:
        msg = "Expected '%s' to be conformant to " % \
          type_check_string(vm, extra).v
    else:
        msg = "Expected to be conformant to "
    msg += should_be.v
    o_path = type_check_string(vm, vm.get_slot_apply(o.get_slot(vm, "instance_of"), "path"))
    msg += ", but got instance of %s." % o_path.v
    self.set_slot(vm, "msg", Con_String(vm, msg))
    return vm.get_builtin(BUILTIN_NULL_OBJ)

@con_object_proc
def _Unpack_Exception_init_func(vm):
    (self, expected_o, got_o),_ = vm.decode_args("OII")
    assert isinstance(expected_o, Con_Int)
    assert isinstance(got_o, Con_Int)
    msg = "Unpack of %d elements failed, as %d elements present" % (expected_o.v, got_o.v)
    self.set_slot(vm, "msg", Con_String(vm, msg))
    return vm.get_builtin(BUILTIN_NULL_OBJ)
