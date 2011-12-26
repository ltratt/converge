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


from Builtins import *




def init(vm):
    return new_c_con_module(vm, "VM", "VM", __file__, \
      import_, \
      ["add_modules", "del_mod", "find_module", "import_module", "iter_mods"])


@con_object_proc
def import_(vm):
    (mod,),_ = vm.decode_args("O")

    new_c_con_func_for_mod(vm, "add_modules", add_modules, mod)
    new_c_con_func_for_mod(vm, "del_mod", del_mod, mod)
    new_c_con_func_for_mod(vm, "find_module", find_module, mod)
    new_c_con_func_for_mod(vm, "import_module", import_module, mod)
    new_c_con_func_for_mod(vm, "iter_mods", iter_mods, mod)
    
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def add_modules(vm):
    (mods_o,),_ = vm.decode_args("O")

    vm.pre_get_slot_apply_pump(mods_o, "iter")
    while 1:
        e_o = vm.apply_pump()
        if not e_o:
            break
        e_o = type_check_module(vm, e_o)
        vm.mods[e_o.id_] = e_o
    
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def del_mod(vm):
    (mod_id_o,),_ = vm.decode_args("S")
    assert isinstance(mod_id_o, Con_String)

    if mod_id_o.v not in vm.mods:
        vm.raise_helper("Key_Exception", [mod_id_o])

    del vm.mods[mod_id_o.v]

    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def find_module(vm):
    (mod_id_o,),_ = vm.decode_args("S")
    assert isinstance(mod_id_o, Con_String)
    
    m_o = vm.find_mod(mod_id_o.v)
    if m_o is None:
        m_o = vm.get_builtin(BUILTIN_FAIL_OBJ)
    
    return m_o


@con_object_proc
def import_module(vm):
    (mod_o,),_ = vm.decode_args("M")
    assert isinstance(mod_o, Con_Module)

    mod_o.import_(vm)
    return mod_o


@con_object_gen
def iter_mods(vm):
    _,_ = vm.decode_args("")
    
    for mod in vm.mods.values():
        yield mod
