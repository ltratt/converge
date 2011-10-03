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

from pypy.rlib import jit
from pypy.rpython.lltypesystem import lltype, rffi

from Builtins import *
from Target import *
from VM import *







@jit.elidable_promote()
def _extract_str(bc, soff, ssize):

    off = read_word(bc, soff)
    size = read_word(bc, ssize)
    assert off > 0 and size > 0
    return rffi.charpsize2str(rffi.ptradd(bc, off), size)



def add_exec(vm, bc):
    main_mod_id = None
    for i in range(read_word(bc, BC_HD_NUM_MODULES)):
        mod_off = read_word(bc, BC_HD_MODULES + i * INTSIZE)
        mod_size = read_word(bc, mod_off + BC_MOD_SIZE)
        assert mod_off > 0 and mod_size > 0
        
        mod_bc = rffi.ptradd(bc, mod_off)
        
        name = _extract_str(mod_bc, BC_MOD_NAME, BC_MOD_NAME_SIZE)
        id_ = _extract_str(mod_bc, BC_MOD_ID, BC_MOD_ID_SIZE)
        src_path = _extract_str(mod_bc, BC_MOD_SRC_PATH, BC_MOD_SRC_PATH_SIZE)

        if main_mod_id is None:
            main_mod_id = id_

        imps = []
        j = read_word(mod_bc, BC_MOD_IMPORTS)
        for k in range(read_word(mod_bc, BC_MOD_NUM_IMPORTS)):
            assert j > 0
            imp_size = read_word(mod_bc, j)
            assert imp_size > 0
            j += INTSIZE
            imps.append(rffi.charpsize2str(rffi.ptradd(mod_bc, j), imp_size))
            j += align(imp_size)
            j += INTSIZE + align(read_word(mod_bc, j))

        num_vars = read_word(mod_bc, BC_MOD_NUM_TL_VARS_MAP)
        tlvars_map = {}
        j = read_word(mod_bc, BC_MOD_TL_VARS_MAP)
        for k in range(num_vars):
            assert j > 0
            var_num = read_word(mod_bc, j)
            j += INTSIZE
            tlvar_size = read_word(mod_bc, j)
            assert tlvar_size > 0
            j += INTSIZE
            n = rffi.charpsize2str(rffi.ptradd(mod_bc, j), tlvar_size)
            tlvars_map[n] = var_num
            j += align(tlvar_size)

        num_consts = read_word(mod_bc, BC_MOD_NUM_CONSTANTS)

        mod = new_bc_con_module(vm, mod_bc, name, id_, src_path, imps, tlvars_map, num_consts)
        init_func_off = read_word(mod_bc, BC_MOD_INSTRUCTIONS)
        pc = BC_PC(mod, init_func_off)
        mod.init_func = Con_Func(vm, new_con_string(vm, "$$init$$"), False, pc, 0, num_vars, mod, \
          None)
        
        vm.set_mod(mod)

    return main_mod_id