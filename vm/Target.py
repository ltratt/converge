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


from rpython.rlib.jit import *
from rpython.rtyper.lltypesystem import lltype, rffi

import sys




if sys.maxsize > 2**32:
    INTSIZE = 8
    FLOATSIZE = 8
else:
    INTSIZE = 4
    FLOATSIZE = 8

INSTR_NAMES = [None, "EXBI", "VAR_LOOKUP", "VAR_ASSIGN", None, "ADD_FAILURE_FRAME", "ADD_FAIL_UP_FRAME", "REMOVE_FAILURE_FRAME", "IS_ASSIGNED", "IS", "FAIL_NOW", "POP", "LIST", "SLOT_LOOKUP", "APPLY", "FUNC_DEFN", "RETURN", "BRANCH", "YIELD", None, "IMPORT", "DICT", "DUP", "PULL", "CHANGE_FAIL_POINT", None, "BUILTIN_LOOKUP", "ASSIGN_SLOT", "EYIELD", "ADD_EXCEPTION_FRAME", None, "INSTANCE_OF", "REMOVE_EXCEPTION_FRAME", "RAISE", "SET_ITEM", "UNPACK_ARGS", "SET", "BRANCH_IF_NOT_FAIL", "BRANCH_IF_FAIL", "CONST_GET", None, "PRE_SLOT_LOOKUP_APPLY", "UNPACK_ASSIGN", "EQ", "LE", "ADD", "SUBTRACT", "NEQ", "LE_EQ", "GR_EQ", "GT", "MODULE_LOOKUP"]

CONST_STRING = 0
CONST_INT = 1
CONST_FLOAT = 2

if INTSIZE == 8:
    BC_HD_HEADER = 0 * 8
    BC_HD_VERSION = 1 * 8
    BC_HD_NUM_MODULES = 2 * 8
    BC_HD_MODULES = 3 * 8

    BC_MOD_HEADER = 0 * 8
    BC_MOD_VERSION = 1 * 8
    BC_MOD_NAME = 2 * 8
    BC_MOD_NAME_SIZE = 3 * 8
    BC_MOD_ID = 4 * 8
    BC_MOD_ID_SIZE = 5 * 8
    BC_MOD_SRC_PATH = 6 * 8
    BC_MOD_SRC_PATH_SIZE = 7 * 8
    BC_MOD_INSTRUCTIONS = 8 * 8
    BC_MOD_INSTRUCTIONS_SIZE = 9 * 8
    BC_MOD_IMPORTS = 10 * 8
    BC_MOD_IMPORTS_SIZE = 11 * 8
    BC_MOD_NUM_IMPORTS = 12 * 8
    BC_MOD_SRC_POSITIONS = 13 * 8
    BC_MOD_SRC_POSITIONS_SIZE = 14 * 8
    BC_MOD_NEWLINES = 15 * 8
    BC_MOD_NUM_NEWLINES = 16 * 8
    BC_MOD_TL_VARS_MAP = 17 * 8
    BC_MOD_TL_VARS_MAP_SIZE = 18 * 8
    BC_MOD_NUM_TL_VARS_MAP = 19 * 8
    BC_MOD_NUM_CONSTANTS = 20 * 8
    BC_MOD_CONSTANTS_OFFSETS = 21 * 8
    BC_MOD_CONSTANTS = 22 * 8
    BC_MOD_CONSTANTS_SIZE = 23 * 8
    BC_MOD_MOD_LOOKUPS = 24 * 8
    BC_MOD_MOD_LOOKUPS_SIZE = 25 * 8
    BC_MOD_NUM_MOD_LOOKUPS = 26 * 8
    BC_MOD_IMPORT_DEFNS = 27 * 8
    BC_MOD_NUM_IMPORT_DEFNS = 28 * 8
    BC_MOD_SIZE = 29 * 8
    
    # Libraries

    BC_LIB_HD_HEADER = 0 * 8
    BC_LIB_HD_FORMAT_VERSION = 1 * 8
    BC_LIB_HD_NUM_MODULES = 2 * 8
    BC_LIB_HD_MODULES = 3 * 8
    
    CON_INSTR_EXBI = 1                    # bits 0-7 1, bits 8-31 size of field name, bits 32-.. field name
    CON_INSTR_VAR_LOOKUP = 2              # bits 0-7 2, bits 8-19 closures offset, bits 20-31 var number
    CON_INSTR_VAR_ASSIGN = 3              # bits 0-7 3, bits 8-19 closures offset, bits 20-31 var number
    CON_INSTR_ADD_FAILURE_FRAME = 5       # bits 0-7 5, bits 8-30 pc offset, bit 31 offset sign
    CON_INSTR_ADD_FAIL_UP_FRAME = 6       # bits 0-7 6
    CON_INSTR_REMOVE_FAILURE_FRAME = 7    # bits 0-7 7
    CON_INSTR_IS_ASSIGNED = 8             # Stored as two words
                                          #   Word 1: bits 0-7 8, bits 8-19 closures offset, bits 20-31 var number
                                          #   Word 2: bits bits 8-30 pc offset, bit 31 offset sign
    CON_INSTR_IS = 9                      # bits 0-7 9
    CON_INSTR_FAIL_NOW = 10               # bits 0-7 10
    CON_INSTR_POP = 11                    # bits 0-7 11
    CON_INSTR_LIST = 12                   # bits 0-7 12, bits 8-31 number of list elements
    CON_INSTR_SLOT_LOOKUP = 13            # bits 0-7 13, bits 8-31 size of slot name, bits 32-.. slot name
    CON_INSTR_APPLY = 14                  # bits 0-7 14, bits 8-31 number of args
    CON_INSTR_FUNC_DEFN = 15              # bits 0-7 15, bits 8-9 is_bound, bits 9-32 max_stack_size, bit 33 has_loop
    CON_INSTR_RETURN = 16                 # bits 0-7 16
    CON_INSTR_BRANCH = 17                 # bits 0-7 17, bits 8-30 pc offset, bit 31 offset sign (0 = positive, 1 = negative)
    CON_INSTR_YIELD = 18                  # bits 0-7 18
    CON_INSTR_IMPORT = 20                 # bits 0-7 20, bits 8-31 module number
    CON_INSTR_DICT = 21                   # bits 0-7 21, bits 8-31 number of dictionary elements
    CON_INSTR_DUP = 22                    # bits 0-7 22
    CON_INSTR_PULL = 23                   # bits 0-7 23, bits 8-31 := number of entries back in the stack to pull the value from
    CON_INSTR_BUILTIN_LOOKUP = 26         # bits 0-7 26, bits 8-15 builtin number
    CON_INSTR_ASSIGN_SLOT = 27            # bits 0-7 13, bits 8-31 size of slot name, bits 32-.. slot name
    CON_INSTR_EYIELD = 28                 # bits 0-7 28
    CON_INSTR_ADD_EXCEPTION_FRAME = 29    # bits 0-7 5, bits 8-30 pc offset, bit 31 offset sign
    CON_INSTR_INSTANCE_OF = 31            # bits 0-7 31
    CON_INSTR_REMOVE_EXCEPTION_FRAME = 32 # bits 0-7 32
    CON_INSTR_RAISE = 33                  # bits 0-7 33
    CON_INSTR_SET_ITEM = 34               # bits 0-7 34
    CON_INSTR_UNPACK_ARGS = 35            # bits 0-7 := 35, bits 8-15 := num normal args, bit 16 := has var args
                                          #  each variable is then a subsequent word:
                                          #  word 1: 0-11 := var number, bit 12 := is mandatory arg
    CON_INSTR_SET = 36                    # bits 0-7 36, bits 8-31 number of set elements
    CON_INSTR_BRANCH_IF_NOT_FAIL = 37     # bits 0-7 37, bits 8-30 pc offset, bit 31 offset sign
    CON_INSTR_BRANCH_IF_FAIL = 38         # bits 0-7 38, bits 8-30 pc offset, bit 31 offset sign
    CON_INSTR_CONST_GET = 39              # bits 0-7 39, bits 8-30 constant num
    CON_INSTR_PRE_SLOT_LOOKUP_APPLY = 41  # bits 0-7 41, bits 8-31 size of slot name, bits 32-.. slot name
    CON_INSTR_UNPACK_ASSIGN = 42          # bits 0-7 42, bits 8-31 number of elements to unpack
    CON_INSTR_EQ = 43                     # bits 0-7 43
    CON_INSTR_LE = 44                     # bits 0-7 44
    CON_INSTR_ADD = 45                    # bits 0-7 45
    CON_INSTR_SUBTRACT = 46               # bits 0-7 46
    CON_INSTR_NEQ = 47                    # bits 0-7 47
    CON_INSTR_LE_EQ = 48                  # bits 0-7 48
    CON_INSTR_GR_EQ = 49                  # bits 0-7 49
    CON_INSTR_GT = 50                     # bits 0-7 50
    CON_INSTR_MODULE_LOOKUP = 51          # bits 0-7 51, bits 8-31 := size of definition name, bits 32-.. := definition name

    @elidable_promote()
    def extract_str(bc, off, size):
        assert off > 0 and size >= 0
        return rffi.charpsize2str(rffi.ptradd(bc, off), size)

    @elidable_promote()
    def read_word(bc, i):
        return rffi.cast(lltype.Signed, rffi.cast(rffi.LONGP, bc)[i / 8])

    @elidable_promote()
    def read_uint32_word(bc, i):
        return rffi.cast(lltype.Signed, rffi.cast(rffi.UINTP, bc)[i / 4])

    @elidable_promote()
    def read_float(bc, i):
        return rffi.cast(lltype.Float, rffi.cast(rffi.DOUBLEP, bc)[i / 8])

    @elidable_promote()
    def align(i):
        return (i + 7) & ~7

    @elidable_promote()
    def unpack_exbi(instr):
        return (4, (instr & 0xFFFFFF00) >> 8)

    @elidable_promote()
    def get_instr(instr):
        return instr & 0xFF

    @elidable_promote()
    def unpack_var_lookup(instr):
        return ((instr & 0x000FFF00) >> 8, (instr & 0xFFF00000) >> 20)

    @elidable_promote()
    def unpack_var_assign(instr):
        return ((instr & 0x000FFF00) >> 8, (instr & 0xFFF00000) >> 20)

    @elidable_promote()
    def unpack_int(instr):
        x = 63
        if (instr & (1 << x)) >> 8:
            return -((instr & ((1 << x) - 256)) >> 8)
        else:
            return (instr & ((1 << x) - 256)) >> 8

    @elidable_promote()
    def unpack_add_failure_frame(instr):
        if (instr & 0x80000000) >> 8:
            return -((instr & 0x7FFFFF00) >> 8)
        else:
            return (instr & 0x7FFFFF00) >> 8

    @elidable_promote()
    def unpack_is_assigned(instr2):
        if (instr2 & 0x80000000) >> 8:
            return -((instr2 & 0x7FFFFF00) >> 8)
        else:
            return (instr2 & 0x7FFFFF00) >> 8

    @elidable_promote()
    def unpack_func_defn(instr):
        return ((instr & 0x00000100) >> 8, (instr & 0x7ffffe00) >> 9, (instr & 0x80000000))

    @elidable_promote()
    def unpack_list(instr):
        return (instr & 0xFFFFFF00) >> 8

    @elidable_promote()
    def unpack_slot_lookup(instr):
        return (4, (instr & 0xFFFFFF00) >> 8)

    @elidable_promote()
    def unpack_apply(instr):
        return (instr & 0xFFFFFF00) >> 8
    
    @elidable_promote()
    def unpack_branch(instr):
        if (instr & 0x80000000) >> 8:
            return -((instr & 0x7FFFFF00) >> 8)
        else:
            return (instr & 0x7FFFFF00) >> 8

    @elidable_promote()
    def unpack_pull(instr):
        return (instr & 0xFFFFFF00) >> 8

    @elidable_promote()
    def unpack_import(instr):
        return (instr & 0xFFFFFF00) >> 8

    @elidable_promote()
    def unpack_dict(instr):
        return (instr & 0xFFFFFF00) >> 8

    @elidable_promote()
    def unpack_string(instr):
        return (4, (instr & 0xFFFFFF00) >> 8)

    @elidable_promote()
    def unpack_builtin_lookup(instr):
        return (instr & 0xFFFFFF00) >> 8

    @elidable_promote()
    def unpack_assign_slot(instr):
        return (4, (instr & 0xFFFFFF00) >> 8)

    @elidable_promote()
    def unpack_add_exception_frame(instr):
        if (instr & 0x80000000) >> 8:
            return -((instr & 0x7FFFFF00) >> 8)
        else:
            return (instr & 0x7FFFFF00) >> 8

    @elidable_promote()
    def unpack_unpack_args(instr):
        return ((instr & 0x0000FF00) >> 8, (instr & 0x00010000) >> 16)

    @elidable_promote()
    def unpack_set(instr):
        return (instr & 0xFFFFFF00) >> 8

    @elidable_promote()
    def unpack_unpack_args_is_mandatory(arg_info):
        return (arg_info & 0x00000100) >> 8

    @elidable_promote()
    def unpack_unpack_args_arg_num(arg_info):
        return arg_info & 0x000000FF

    @elidable_promote()
    def unpack_constant_get(instr):
        return (instr & 0xFFFFFF00) >> 8

    @elidable_promote()
    def unpack_unpack_assign(instr):
        return (instr & 0xFFFFFF00) >> 8

    @elidable_promote()
    def unpack_branch_if_not_fail(instr):
        if (instr & 0x80000000) >> 8:
            return -((instr & 0x7FFFFF00) >> 8)
        else:
            return (instr & 0x7FFFFF00) >> 8

    @elidable_promote()
    def unpack_mod_lookup(instr):
        return (4, (instr & 0xFFFFFF00) >> 8)
else:
    assert INTSIZE == 4
    BC_HD_HEADER = 0 * 4
    BC_HD_VERSION = 2 * 4
    BC_HD_NUM_MODULES = 3 * 4
    BC_HD_MODULES = 4 * 4

    BC_MOD_HEADER = 0 * 4
    BC_MOD_VERSION = 2 * 4
    BC_MOD_NAME = 3 * 4
    BC_MOD_NAME_SIZE = 4 * 4
    BC_MOD_ID = 5 * 4
    BC_MOD_ID_SIZE = 6 * 4
    BC_MOD_SRC_PATH = 7 * 4
    BC_MOD_SRC_PATH_SIZE = 8 * 4
    BC_MOD_INSTRUCTIONS = 9 * 4
    BC_MOD_INSTRUCTIONS_SIZE = 10 * 4
    BC_MOD_IMPORTS = 11 * 4
    BC_MOD_IMPORTS_SIZE = 12 * 4
    BC_MOD_NUM_IMPORTS = 13 * 4
    BC_MOD_SRC_POSITIONS = 14 * 4
    BC_MOD_SRC_POSITIONS_SIZE = 15 * 4
    BC_MOD_NEWLINES = 16 * 4
    BC_MOD_NUM_NEWLINES = 17 * 4
    BC_MOD_TL_VARS_MAP = 18 * 4
    BC_MOD_TL_VARS_MAP_SIZE = 19 * 4
    BC_MOD_NUM_TL_VARS_MAP = 20 * 4
    BC_MOD_NUM_CONSTANTS = 21 * 4
    BC_MOD_CONSTANTS_OFFSETS = 22 * 4
    BC_MOD_CONSTANTS = 23 * 4
    BC_MOD_CONSTANTS_SIZE = 24 * 4
    BC_MOD_MOD_LOOKUPS = 25 * 4
    BC_MOD_MOD_LOOKUPS_SIZE = 26 * 4
    BC_MOD_NUM_MOD_LOOKUPS = 27 * 4
    BC_MOD_IMPORT_DEFNS = 28 * 4
    BC_MOD_NUM_IMPORT_DEFNS = 29 * 4
    BC_MOD_SIZE = 30 * 4
    
    # Libraries

    BC_LIB_HD_HEADER = 0 * 4
    BC_LIB_HD_FORMAT_VERSION = 2 * 4
    BC_LIB_HD_NUM_MODULES = 3 * 4
    BC_LIB_HD_MODULES = 4 * 4

    CON_INSTR_EXBI = 1                    # bits 0-7 1, bits 8-31 size of field name, bits 32-.. field name
    CON_INSTR_VAR_LOOKUP = 2              # bits 0-7 2, bits 8-19 closures offset, bits 20-31 var number
    CON_INSTR_VAR_ASSIGN = 3              # bits 0-7 3, bits 8-19 closures offset, bits 20-31 var number
    CON_INSTR_ADD_FAILURE_FRAME = 5       # bits 0-7 5, bits 8-30 pc offset, bit 31 offset sign
    CON_INSTR_ADD_FAIL_UP_FRAME = 6       # bits 0-7 6
    CON_INSTR_REMOVE_FAILURE_FRAME = 7    # bits 0-7 7
    CON_INSTR_IS_ASSIGNED = 8             # Stored as two words
                                          #   Word 1: bits 0-7 8, bits 8-19 closures offset, bits 20-31 var number
                                          #   Word 2: bits bits 8-30 pc offset, bit 31 offset sign
    CON_INSTR_IS = 9                      # bits 0-7 9
    CON_INSTR_FAIL_NOW = 10               # bits 0-7 10
    CON_INSTR_POP = 11                    # bits 0-7 11
    CON_INSTR_LIST = 12                   # bits 0-7 12, bits 8-31 number of list elements
    CON_INSTR_SLOT_LOOKUP = 13            # bits 0-7 13, bits 8-31 size of slot name, bits 32-.. slot name
    CON_INSTR_APPLY = 14                  # bits 0-7 14, bits 8-31 number of args
    CON_INSTR_FUNC_DEFN = 15              # bits 0-7 15, bits 8-9 is_bound
    CON_INSTR_RETURN = 16                 # bits 0-7 16
    CON_INSTR_BRANCH = 17                 # bits 0-7 17, bits 8-30 pc offset, bit 31 offset sign (0 = positive, 1 = negative)
    CON_INSTR_YIELD = 18                  # bits 0-7 18
    CON_INSTR_IMPORT = 20                 # bits 0-7 20, bits 8-31 module number
    CON_INSTR_DICT = 21                   # bits 0-7 21, bits 8-31 number of dictionary elements
    CON_INSTR_DUP = 22                    # bits 0-7 22
    CON_INSTR_PULL = 23                   # bits 0-7 23, bits 8-31 := number of entries back in the stack to pull the value from
    CON_INSTR_BUILTIN_LOOKUP = 26         # bits 0-7 26, bits 8-15 builtin number
    CON_INSTR_ASSIGN_SLOT = 27            # bits 0-7 13, bits 8-31 size of slot name, bits 32-.. slot name
    CON_INSTR_EYIELD = 28                 # bits 0-7 28
    CON_INSTR_ADD_EXCEPTION_FRAME = 29    # bits 0-7 5, bits 8-30 pc offset, bit 31 offset sign
    CON_INSTR_INSTANCE_OF = 31            # bits 0-7 31
    CON_INSTR_REMOVE_EXCEPTION_FRAME = 32 # bits 0-7 32
    CON_INSTR_RAISE = 33                  # bits 0-7 33
    CON_INSTR_SET_ITEM = 34               # bits 0-7 34
    CON_INSTR_UNPACK_ARGS = 35            # bits 0-7 := 35, bits 8-15 := num normal args, bit 16 := has var args
                                          #  each variable is then a subsequent word:
                                          #  word 1: 0-11 := var number, bit 12 := is mandatory arg
    CON_INSTR_SET = 36                    # bits 0-7 36, bits 8-31 number of set elements
    CON_INSTR_BRANCH_IF_NOT_FAIL = 37     # bits 0-7 37, bits 8-30 pc offset, bit 31 offset sign
    CON_INSTR_BRANCH_IF_FAIL = 38         # bits 0-7 38, bits 8-30 pc offset, bit 31 offset sign
    CON_INSTR_CONST_GET = 39              # bits 0-7 39, bits 8-30 constant num
    CON_INSTR_PRE_SLOT_LOOKUP_APPLY = 41  # bits 0-7 41, bits 8-31 size of slot name, bits 32-.. slot name
    CON_INSTR_UNPACK_ASSIGN = 42          # bits 0-7 42, bits 8-31 number of elements to unpack
    CON_INSTR_EQ = 43                     # bits 0-7 43
    CON_INSTR_LE = 44                     # bits 0-7 44
    CON_INSTR_ADD = 45                    # bits 0-7 45
    CON_INSTR_SUBTRACT = 46               # bits 0-7 46
    CON_INSTR_NEQ = 47                    # bits 0-7 47
    CON_INSTR_LE_EQ = 48                  # bits 0-7 48
    CON_INSTR_GR_EQ = 49                  # bits 0-7 49
    CON_INSTR_GT = 50                     # bits 0-7 50
    CON_INSTR_MODULE_LOOKUP = 51          # bits 0-7 51, bits 8-31 := size of definition name, bits 32-.. := definition name

    @elidable_promote()
    def extract_str(bc, off, size):
        assert off > 0 and size >= 0
        return rffi.charpsize2str(rffi.ptradd(bc, off), size)

    @elidable_promote()
    def read_word(bc, i):
        return rffi.cast(lltype.Signed, rffi.cast(rffi.INTP, bc)[i / 4])

    @elidable_promote()
    def read_uint32_word(bc, i):
        return rffi.cast(lltype.Signed, rffi.cast(rffi.UINTP, bc)[i / 4])

    @elidable_promote()
    def read_float(bc, i):
        return rffi.cast(lltype.Float, rffi.cast(rffi.DOUBLEP, bc)[i / 8])

    @elidable_promote()
    def align(i):
        return (i + 3) & ~3

    @elidable_promote()
    def unpack_exbi(instr):
        x = 0xFFFFFF
        return (4, (instr & (x << 8)) >> 8)

    @elidable_promote()
    def get_instr(instr):
        return instr & 0xFF

    @elidable_promote()
    def unpack_var_lookup(instr):
        x = 0xFFF
        return ((instr & (x << 8)) >> 8, (instr & (x << 20)) >> 20)

    @elidable_promote()
    def unpack_var_assign(instr):
        x = 0xFFF
        return ((instr & (x << 8)) >> 8, (instr & (x << 20)) >> 20)

    @elidable_promote()
    def unpack_int(instr):
        x = 31
        if (instr & (1 << x)) >> 8:
            return -((instr & ((1 << x) - 256)) >> 8)
        else:
            return (instr & ((1 << x) - 256)) >> 8

    @elidable_promote()
    def unpack_add_failure_frame(instr):
        x = 0x8
        y = 0x7FFFFF
        if (instr & (x << 28)) >> 8:
            return -((instr & (y << 8)) >> 8)
        else:
            return (instr & (y << 8)) >> 8

    @elidable_promote()
    def unpack_is_assigned(instr2):
        x = 0x8
        y = 0x7FFFFF
        if (instr2 & (x << 28)) >> 8:
            return -((instr2 & (y << 8)) >> 8)
        else:
            return (instr2 & (y << 8)) >> 8

    @elidable_promote()
    def unpack_func_defn(instr):
        x = 0x7ffffe
        return ((instr & 0x00000100) >> 8, (instr & (x << 8)) >> 9, (instr & (1 << 31)))

    @elidable_promote()
    def unpack_list(instr):
        x = 0xFFFFFF
        return (instr & (x << 8)) >> 8

    @elidable_promote()
    def unpack_slot_lookup(instr):
        x = 0xFFFFFF
        return (4, (instr & (x << 8)) >> 8)

    @elidable_promote()
    def unpack_apply(instr):
        x = 0xFFFFFF
        return (instr & (x << 8)) >> 8
    
    @elidable_promote()
    def unpack_branch(instr):
        x = 0x8
        y = 0x7FFFFF
        if (instr & (x << 28)) >> 8:
            return -((instr & (y << 8)) >> 8)
        else:
            return (instr & (y << 8)) >> 8

    @elidable_promote()
    def unpack_pull(instr):
        x = 0xFFFFFF
        return (instr & (x << 8)) >> 8

    @elidable_promote()
    def unpack_import(instr):
        x = 0xFFFFFF
        return (instr & (x << 8)) >> 8

    @elidable_promote()
    def unpack_dict(instr):
        x = 0xFFFFFF
        return (instr & (x << 8)) >> 8

    @elidable_promote()
    def unpack_string(instr):
        x = 0xFFFFFF
        return (4, (instr & (x << 8)) >> 8)

    @elidable_promote()
    def unpack_builtin_lookup(instr):
        x = 0xFFFFFF
        return (instr & (x << 8)) >> 8

    @elidable_promote()
    def unpack_assign_slot(instr):
        x = 0xFFFFFF
        return (4, (instr & (x << 8)) >> 8)

    @elidable_promote()
    def unpack_add_exception_frame(instr):
        x = 0x8
        y = 0x7FFFFF
        if (instr & (x << 28)) >> 8:
            return -((instr & (y << 8)) >> 8)
        else:
            return (instr & (y << 8)) >> 8

    @elidable_promote()
    def unpack_unpack_args(instr):
        return ((instr & 0x0000FF00) >> 8, (instr & 0x00010000) >> 16)

    @elidable_promote()
    def unpack_set(instr):
        x = 0xFFFFFF
        return (instr & (x << 8)) >> 8

    @elidable_promote()
    def unpack_unpack_args_is_mandatory(arg_info):
        return (arg_info & 0x00000100) >> 8

    @elidable_promote()
    def unpack_unpack_args_arg_num(arg_info):
        return arg_info & 0x000000FF

    @elidable_promote()
    def unpack_constant_get(instr):
        x = 0xFFFFFF
        return (instr & (x << 8)) >> 8

    @elidable_promote()
    def unpack_unpack_assign(instr):
        x = 0xFFFFFF
        return (instr & (x << 8)) >> 8

    @elidable_promote()
    def unpack_branch_if_not_fail(instr):
        x = 0x8
        y = 0x7FFFFF
        if (instr & (x << 28)) >> 8:
            return -((instr & (y << 8)) >> 8)
        else:
            return (instr & (y << 8)) >> 8

    @elidable_promote()
    def unpack_mod_lookup(instr):
        x = 0xFFFFFF
        return (4, (instr & (x << 8)) >> 8)
