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
from pypy.rpython.lltypesystem import lltype, rffi
from pypy.rpython.tool import rffi_platform as platform
from pypy.translator.tool.cbuild import ExternalCompilationInfo
from Builtins import *
from Core import *



DEFAULT_ENTRIES_ALLOC = 256


eci         = ExternalCompilationInfo(includes=["string.h"])
memmove     = rffi.llexternal("memmove", [rffi.CCHARP, rffi.CCHARP, rffi.SIZE_T], rffi.CCHARP, compilation_info=eci)
class CConfig:
    _compilation_info_ = eci
cconfig = platform.configure(CConfig)



def init(vm):
    return new_c_con_module(vm, "Array", "Array", __file__, import_, \
      ["Array_Exception", "Array"])


@con_object_proc
def import_(vm):
    (mod,),_ = vm.decode_args("O")

    class_class = vm.get_builtin(BUILTIN_CLASS_CLASS)
    user_exception_class = vm.get_builtin(BUILTIN_EXCEPTIONS_MODULE). \
      get_defn(vm, "User_Exception")
    array_exception = vm.get_slot_apply(class_class, "new", \
      [Con_String(vm, "Array_Exception"), Con_List(vm, [user_exception_class]), mod])
    mod.set_defn(vm, "Array_Exception", array_exception)

    bootstrap_array_class(vm, mod)

    return vm.get_builtin(BUILTIN_NULL_OBJ)



################################################################################
# class Array
#

TYPE_I32 = 0
TYPE_I64 = 1
TYPE_F   = 2

class Array(Con_Boxed_Object):
    __slots__ = ("type_name", "type", "type_size", "big_endian", "data", "num_entries",
      "entries_alloc")
    _immutable_fields_ = ("type_name", "type", "big_endian")


    def __init__(self, vm, instance_of, type_name, data_o):
        Con_Boxed_Object.__init__(self, vm, instance_of)
        self.type_name = type_name
        if type_name == "i":
            if Target.INTSIZE == 4:
                self.type = TYPE_I32
                self.type_size = 4
            else:
                assert Target.INTSIZE == 8
                self.type = TYPE_I64
                self.type_size = 8
            self._auto_endian()
        elif type_name == "i32":
            self.type = TYPE_I32
            self.type_size = 4
            self._auto_endian()
        elif type_name == "i32be":
            self.type = TYPE_I32
            self.type_size = 4
            self.big_endian = True
        elif type_name == "i32le":
            self.type = TYPE_I32
            self.type_size = 4
            self.big_endian = False
        elif type_name == "i64":
            self.type = TYPE_I64
            self.type_size = 8
            self._auto_endian()
        elif type_name == "i64be":
            self.type = TYPE_I64
            self.type_size = 8
            self.big_endian = True
        elif type_name == "i64le":
            self.type = TYPE_I64
            self.type_size = 8
            self.big_endian = False
        elif type_name == "f":
            self.type = TYPE_F
            self.type_size = 8
            self.big_endian = False # irrelevant for floats
        else:
            mod = vm.get_funcs_mod()
            aex_class = mod.get_defn(vm, "Array_Exception")
            vm.raise_(vm.get_slot_apply(aex_class, "new", \
              [Con_String(vm, "Unknown array type '%s'." % type)]))

        if data_o is not None:
            if isinstance(data_o, Con_String):
                data = data_o.v
                i = len(data)
                self._alignment_check(vm, i)
                self.num_entries = self.entries_alloc = i // self.type_size
                self.data = lltype.malloc(rffi.CCHARP.TO, i, flavor="raw")
                i -= 1
                while i >= 0:
                    self.data[i] = data[i]
                    i -= 1
            else:
                self.num_entries = 0
                self.entries_alloc = type_check_int(vm, vm.get_slot_apply(data_o, "len")).v
                self.data = lltype.malloc(rffi.CCHARP.TO, self.entries_alloc * self.type_size, flavor="raw")
                vm.get_slot_apply(self, "extend", [data_o])
        else:
            self.num_entries = 0
            self.entries_alloc = DEFAULT_ENTRIES_ALLOC
            self.data = lltype.malloc(rffi.CCHARP.TO, self.entries_alloc * self.type_size, flavor="raw")


    def __del__(self):
        lltype.free(self.data, flavor="raw")


    def _auto_endian(self):
        if ENDIANNESS == "BIG_ENDIAN":
            self.big_endian = True
        else:
            assert ENDIANNESS == "LITTLE_ENDIAN"
            self.big_endian = False


    def _alignment_check(self, vm, i):
        if i % self.type_size == 0:
            return

        mod = vm.get_funcs_mod()
        aex_class = mod.get_defn(vm, "Array_Exception")
        vm.raise_(vm.get_slot_apply(aex_class, "new", \
          [Con_String(vm, "Data of len %d not aligned to a multiple of %d." % (i, self.type_size))]))


@con_object_proc
def _new_func_Array(vm):
    (class_, type_o, data_o),_ = vm.decode_args("CS", opt="O")
    assert isinstance(type_o, Con_String)

    a_o = Array(vm, class_, type_o.v, data_o)
    if data_o is None:
        data_o = vm.get_builtin(BUILTIN_NULL_OBJ)
    vm.get_slot_apply(a_o, "init", [type_o, data_o])

    return a_o


@con_object_proc
def Array_append(vm):
    (self, o_o),_ = vm.decode_args("!O", self_of=Array)
    assert isinstance(self, Array)

    _append(vm, self, o_o)
    objectmodel.keepalive_until_here(self)
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def Array_extend(vm):
    (self, o_o),_ = vm.decode_args("!O", self_of=Array)
    assert isinstance(self, Array)

    if isinstance(o_o, Array) and self.type_size == o_o.type_size:
        _check_room(vm, self, o_o.num_entries)
        memmove(rffi.ptradd(self.data, self.num_entries * self.type_size), \
          o_o.data, o_o.num_entries * o_o.type_size)
        self.num_entries += o_o.num_entries
    else:
        vm.pre_get_slot_apply_pump(o_o, "iter")
        while 1:
            e_o = vm.apply_pump()
            if not e_o:
                break
            _append(vm, self, e_o)
    objectmodel.keepalive_until_here(self)

    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def Array_extend_from_string(vm):
    (self, s_o),_ = vm.decode_args("!S", self_of=Array)
    assert isinstance(self, Array)
    assert isinstance(s_o, Con_String)

    s = s_o.v
    i = len(s)
    self._alignment_check(vm, i)
    _check_room(vm, self, i // self.type_size)
    i -= 1
    p = self.num_entries * self.type_size
    while i >= 0:
        self.data[p + i] = s[i]
        i -= 1
    self.num_entries += len(s) // self.type_size
    objectmodel.keepalive_until_here(self)

    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def Array_get(vm):
    (self, i_o),_ = vm.decode_args("!I", self_of=Array)
    assert isinstance(self, Array)
    assert isinstance(i_o, Con_Int)

    i = translate_idx(vm, i_o.v, self.num_entries)
    o = _get_obj(vm, self, i)
    objectmodel.keepalive_until_here(self)
    return o


@con_object_proc
def Array_get_slice(vm):
    mod = vm.get_funcs_mod()
    (self, i_o, j_o),_ = vm.decode_args("!", opt="ii", self_of=Array)
    assert isinstance(self, Array)

    i, j = translate_slice_idx_objs(vm, i_o, j_o, self.num_entries)
    # This does a double allocation, so isn't very efficient. It is pleasingly simple though.
    data = rffi.charpsize2str(rffi.ptradd(self.data, i * self.type_size), int((j - i) * self.type_size))
    objectmodel.keepalive_until_here(self)
    return Array(vm, mod.get_defn(vm, "Array"), self.type_name, Con_String(vm, data))


@con_object_gen
def Array_iter(vm):
    (self, i_o, j_o),_ = vm.decode_args("!", opt="ii", self_of=Array)
    assert isinstance(self, Array)

    i, j = translate_slice_idx_objs(vm, i_o, j_o, self.num_entries)
    for k in range(i, j):
        yield _get_obj(vm, self, k)
    objectmodel.keepalive_until_here(self)


@con_object_proc
def Array_len(vm):
    (self,),_ = vm.decode_args("!", self_of=Array)
    assert isinstance(self, Array)

    return Con_Int(vm, self.num_entries)


@con_object_proc
def Array_len_bytes(vm):
    (self,),_ = vm.decode_args("!", self_of=Array)
    assert isinstance(self, Array)

    return Con_Int(vm, self.num_entries * self.type_size)


@con_object_proc
def Array_serialize(vm):
    (self,),_ = vm.decode_args("!", self_of=Array)
    assert isinstance(self, Array)

    data = rffi.charpsize2str(self.data, self.num_entries * self.type_size)
    objectmodel.keepalive_until_here(self) # XXX I don't really understand why this is needed
    return Con_String(vm, data)


@con_object_proc
def Array_set(vm):
    (self, i_o, o_o),_ = vm.decode_args("!IO", self_of=Array)
    assert isinstance(self, Array)
    assert isinstance(i_o, Con_Int)

    i = translate_idx(vm, i_o.v, self.num_entries)
    _set_obj(vm, self, i, o_o)
    objectmodel.keepalive_until_here(self)
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def Array_to_str(vm):
    (self,),_ = vm.decode_args("!", self_of=Array)
    assert isinstance(self, Array)

    data = rffi.charpsize2str(self.data, self.num_entries * self.type_size)
    objectmodel.keepalive_until_here(self)
    return Con_String(vm, data)


def _append(vm, self, o):
    _check_room(vm, self, 1)
    _set_obj(vm, self, self.num_entries, o)
    self.num_entries += 1


def _check_room(vm, self, i):
    if self.num_entries + i < self.entries_alloc:
        return
    o_data = self.data
    self.entries_alloc = int((self.entries_alloc + i + 1) * 1.25)
    assert self.entries_alloc > self.num_entries + i
    self.data = lltype.malloc(rffi.CCHARP.TO, self.entries_alloc * self.type_size, flavor="raw")
    memmove(self.data, o_data, self.num_entries * self.type_size)
    lltype.free(o_data, flavor="raw")


def _get_obj(vm, self, i):
    if self.type == TYPE_I64:
        value = rffi.cast(rffi.LONGP, self.data)[i]
        return Con_Int(vm, rffi.cast(lltype.Signed, value))
    elif self.type == TYPE_I32:
        return Con_Int(vm, rffi.cast(lltype.Signed, rffi.cast(rffi.INTP, self.data)[i]))
    elif self.type == TYPE_F:
        return Con_Float(vm, rffi.cast(rffi.DOUBLE, rffi.cast(rffi.DOUBLEP, self.data)[i]))
    else:
        raise Exception("XXX")


def _set_obj(vm, self, i, o):
    if self.type == TYPE_I64:
        rffi.cast(rffi.LONGP, self.data)[i] = rffi.cast(rffi.LONG, type_check_number(vm, o).as_int())
    elif self.type == TYPE_I32:
        rffi.cast(rffi.INTP, self.data)[i] = rffi.cast(rffi.INT, type_check_number(vm, o).as_int())
    elif self.type == TYPE_F:
        rffi.cast(rffi.DOUBLEP, self.data)[i] = rffi.cast(rffi.DOUBLE, type_check_number(vm, o).as_float())
    else:
        raise Exception("XXX")


def bootstrap_array_class(vm, mod):
    array_class = Con_Class(vm, Con_String(vm, "Array"), [vm.get_builtin(BUILTIN_OBJECT_CLASS)], mod)
    mod.set_defn(vm, "Array", array_class)
    array_class.new_func = new_c_con_func(vm, Con_String(vm, "new_Array"), False, _new_func_Array, mod)

    new_c_con_func_for_class(vm, "append", Array_append, array_class)
    new_c_con_func_for_class(vm, "extend", Array_extend, array_class)
    new_c_con_func_for_class(vm, "extend_from_string", Array_extend_from_string, array_class)
    new_c_con_func_for_class(vm, "get", Array_get, array_class)
    new_c_con_func_for_class(vm, "get_slice", Array_get_slice, array_class)
    new_c_con_func_for_class(vm, "iter", Array_iter, array_class)
    new_c_con_func_for_class(vm, "len", Array_len, array_class)
    new_c_con_func_for_class(vm, "len_bytes", Array_len_bytes, array_class)
    new_c_con_func_for_class(vm, "serialize", Array_serialize, array_class)
    new_c_con_func_for_class(vm, "set", Array_set, array_class)
    new_c_con_func_for_class(vm, "to_str", Array_to_str, array_class)
