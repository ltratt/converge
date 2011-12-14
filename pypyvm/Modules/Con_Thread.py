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
    return new_c_con_module(vm, "Thread", "Thread", __file__, import_, \
      ["get_continuation_src_infos"])


def import_(vm):
    (mod,),_ = vm.decode_args("O")
    
    new_c_con_func_for_mod(vm, "get_continuation_src_infos", get_continuation_src_infos, mod)
    
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def get_continuation_src_infos(vm):
    (levs_o,),_ = vm.decode_args("I")
    assert isinstance(levs_o, Con_Int)

    cf = vm.cf_stack[-(levs_o.v + 1)]
    if isinstance(cf.pc, BC_PC):
        src_infos = cf.pc.mod.bc_off_to_src_infos(vm, cf.bc_off)
    else:
        src_infos = vm.get_builtin(BUILTIN_NULL_OBJ)

    vm.return_(src_infos)