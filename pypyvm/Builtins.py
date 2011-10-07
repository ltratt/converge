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

from pypy.rlib.jit import *
from pypy.rlib.objectmodel import UnboxedValue
from pypy.rpython.lltypesystem import lltype, rffi

NUM_BUILTINS = 40

from Core import *
import Target, VM




BUILTIN_NULL_OBJ = 0
BUILTIN_FAIL_OBJ = 1

# Core atom defs

BUILTIN_ATOM_DEF_OBJECT = 2
BUILTIN_SLOTS_ATOM_DEF_OBJECT = 3
BUILTIN_CLASS_ATOM_DEF_OBJECT = 4
BUILTIN_VM_ATOM_DEF_OBJECT = 5
BUILTIN_THREAD_ATOM_DEF_OBJECT = 6
BUILTIN_FUNC_ATOM_DEF_OBJECT = 7
BUILTIN_STRING_ATOM_DEF_OBJECT = 8
BUILTIN_CON_STACK_ATOM_DEF_OBJECT = 9
BUILTIN_LIST_ATOM_DEF_OBJECT = 10
BUILTIN_DICT_ATOM_DEF_OBJECT = 11
BUILTIN_MODULE_ATOM_DEF_OBJECT = 12
BUILTIN_INT_ATOM_DEF_OBJECT = 13
BUILTIN_UNIQUE_ATOM_DEF_OBJECT = 14
BUILTIN_CLOSURE_ATOM_DEF_OBJECT = 15
BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT = 16
BUILTIN_EXCEPTION_ATOM_DEF_OBJECT = 17
BUILTIN_SET_ATOM_DEF_OBJECT = 18

# Core classes

BUILTIN_OBJECT_CLASS = 19
BUILTIN_CLASS_CLASS = 20
BUILTIN_VM_CLASS = 21
BUILTIN_THREAD_CLASS = 22
BUILTIN_FUNC_CLASS = 23
BUILTIN_STRING_CLASS = 24
BUILTIN_CON_STACK_CLASS = 25
BUILTIN_LIST_CLASS = 26
BUILTIN_DICT_CLASS = 27
BUILTIN_MODULE_CLASS = 28
BUILTIN_INT_CLASS = 29
BUILTIN_CLOSURE_CLASS = 30
BUILTIN_PARTIAL_APPLICATION_CLASS = 31
BUILTIN_EXCEPTION_CLASS = 32
BUILTIN_SET_CLASS = 33
BUILTIN_NUMBER_CLASS = 34

BUILTIN_C_FILE_MODULE = 35
BUILTIN_EXCEPTIONS_MODULE = 36
BUILTIN_SYS_MODULE = 37

# Floats

BUILTIN_FLOAT_ATOM_DEF_OBJECT = 38
BUILTIN_FLOAT_CLASS = 39




################################################################################
# Con_Object
#

class Con_Object(Con_Thingy):
    __slots__ = ()



class Con_Boxed_Object(Con_Object):
    __slots__ = ("slots",)
    _immutable_fields = ("slots",)

    def __init__(self, vm):
        self.slots = {}


    def get_slot(self, n):
        if n == "get_slot":
            raise Exception("XXX")
        
        if n in self.slots:
            v = self.slots[n]
        else:
            raise Exception("XXX")

        if isinstance(v, Con_Func):
            raise Exception("XXX")
        
        return v


    def set_slot(self, n, v):
        self.slots[n] = v



class Con_Unboxed_Object(Con_Object):
    __slots__ = ()



def bootstrap_con_object(vm):
    object_class = Con_Class(vm, "Object", [], None)
    vm.set_builtin(BUILTIN_OBJECT_CLASS, object_class)
    class_class = Con_Class(vm, "Class", [object_class], None)
    vm.set_builtin(BUILTIN_CLASS_CLASS, class_class)
    
    vm.set_builtin(BUILTIN_NULL_OBJ, Con_Boxed_Object(vm))
    vm.set_builtin(BUILTIN_FAIL_OBJ, Con_Boxed_Object(vm))




################################################################################
# Con_Class
#

class Con_Class(Con_Boxed_Object):
    __slots__ = ("name", "supers")
    _immutable_fields = ("supers",)

    def __init__(self, vm, name, supers, container):
        Con_Boxed_Object.__init__(self, vm)
        
        self.name = name
        self.supers = supers

        self.set_slot("container", container)




def bootstrap_con_class():
    class_class = Con_Class("Class", [object_class], None)





################################################################################
# Con_Module
#

class Con_Module(Con_Boxed_Object):
    __slots__ = ("is_bc", "bc", "name", "id_", "src_path", "imps", "tlvars_map", "consts",
      "init_func", "values", "closure", "initialized")
    _immutable_fields_ = ("is_bc", "bc", "name", "id_", "src_path", "imps", "tlvars_map", "init_func", "consts")

    def __init__(self, vm, is_bc, bc, name, id_, src_path, imps, tlvars_map, num_consts, init_func):
        Con_Boxed_Object.__init__(self, vm)

        self.is_bc = is_bc # True for bytecode modules; False for RPython modules
        self.bc = bc
        self.name = name
        self.id_ = id_
        self.src_path = src_path
        self.imps = imps
        self.tlvars_map = tlvars_map
        self.consts = [None] * num_consts
        self.init_func = init_func
        
        self.values = []
        if is_bc:
            self.closure = None
        else:
            self.closure = [None] * len(tlvars_map)

        self.initialized = False


    def import_(self, vm):
        if self.initialized:
            return
        
        if self.is_bc:
            v, self.closure = vm.apply_closure(self.init_func, [self])
        else:
            v = vm.apply(self.init_func, [self])
        self.initialized = True
        return


    def get_defn(self, n):
        return self.closure[self.tlvars_map[n]]


    def set_defn(self, n, o):
        self.closure[self.tlvars_map[n]] = o


    def get_const_create_off(self, vm, i):
        create_off = Target.read_word(self.bc, Target.BC_MOD_CONSTANTS_CREATE_OFFSETS)
        off = Target.read_word(self.bc, create_off + i * Target.INTSIZE)
        return off



def bootstrap_con_module():
    pass



def new_c_con_module(vm, name, id_, src_path, names):
    tlvars_map = {}
    i = 0
    for j in names:
        tlvars_map[j] = i
        i += 1
    return Con_Module(vm, False, lltype.nullptr(rffi.CCHARP.TO), name, id_, src_path, [], tlvars_map, 0, None)


def new_bc_con_module(vm, bc, name, id_, src_path, imps, tlvars_map, num_consts):
    return Con_Module(vm, True, bc, name, id_, src_path, imps, tlvars_map, num_consts, None)





################################################################################
# Con_Func
#

class Con_Func(Con_Boxed_Object):
    __slots__ = ("name", "is_bound", "pc", "num_params", "num_vars", "container_closure")
    _immutable_fields_ = ("name", "is_bound", "pc", "num_params", "num_vars", "container_closure")

    def __init__(self, vm, name, is_bound, pc, num_params, num_vars, container, container_closure):
        Con_Boxed_Object.__init__(self, vm)
    
        self.name = name
        self.is_bound = is_bound
        self.pc = pc
        self.num_params = num_params
        self.num_vars = num_vars
        self.container_closure = container_closure
        
        self.set_slot("container", container)


    def __repr__(self):
        return "<Func %s>" % self.name.v


def bootstrap_con_func():
    pass


def new_c_con_func(vm, name, is_bound, func, container):
    cnd = container
    while not isinstance(cnd, Con_Module):
        cnd = cnd.get_slot("container")
    return Con_Func(vm, name, is_bound, VM.Py_PC(cnd, func), 0, 0, container, None)


def new_bc_con_func(vm, name, is_bound, pc, num_params, num_vars, container, container_closure):
    return Con_Func(vm, name, is_bound, pc, num_params, num_vars, container, container_closure)



################################################################################
# Con_Number
#

class Con_Number(Con_Object):
    pass




################################################################################
# Con_Int
#

class Con_Int(Con_Unboxed_Object):
    __slots__ = ("v",)
    _immutable_fields_ = ("v",)


    def __init__(self, v):
        self.v = v

    def eq(self, vm, o):
        if not isinstance(o, Con_Int):
            raise Exception("XXX")
        else:
            return self.v == o.v


    def gt(self, vm, o):
        if not isinstance(o, Con_Int):
            raise Exception("XXX")
        else:
            return self.v > o.v


    def sub(self, vm, o):
        if not isinstance(o, Con_Int):
            raise Exception("XXX")
        else:
            return Con_Int(self.v - o.v)



def new_con_int(vm, v):
    return Con_Int(v)



################################################################################
# Con_String
#

class Con_String(Con_Boxed_Object):
    __slots__ = ("v",)
    _immutable_fields_ = ("v",)

    def __init__(self, vm, v):
        Con_Boxed_Object.__init__(self, vm)
        self.v = v



def new_con_string(vm, v):
    return Con_String(vm, v)
