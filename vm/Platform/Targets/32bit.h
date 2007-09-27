// Copyright (c) 2003-2006 King's College London, created by Laurence Tratt
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.


#define CON_BYTECODE_HEADER 0 * sizeof(Con_Int)
#define CON_BYTECODE_VERSION 2 * sizeof(Con_Int)
#define CON_BYTECODE_NUMBER_OF_MODULES 3 * sizeof(Con_Int)
#define CON_BYTECODE_MODULES 4 * sizeof(Con_Int)

#define CON_BYTECODE_MODULE_HEADER 0 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_VERSION 2 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_NAME 3 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_NAME_SIZE 4 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_IDENTIFIER 5 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_IDENTIFIER_SIZE 6 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_INSTRUCTIONS 7 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_INSTRUCTIONS_SIZE 8 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_IMPORTS 9 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_IMPORTS_SIZE 10 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_NUM_IMPORTS 11 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_SRC_POSITIONS 12 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_SRC_POSITIONS_SIZE 13 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_NEWLINES 14 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_NUM_NEWLINES 15 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_TOP_LEVEL_VARS_MAP 16 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_TOP_LEVEL_VARS_MAP_SIZE 17 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_NUM_TOP_LEVEL_VARS 18 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_NUM_CONSTANTS 19 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_CONSTANTS_CREATE_OFFSETS 20 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_MOD_LOOKUPS 21 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_MOD_LOOKUPS_SIZE 22 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_NUM_MOD_LOOKUPS 23 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_IMPORT_DEFNS 24 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_NUM_IMPORT_DEFNS 25 * sizeof(Con_Int)
#define CON_BYTECODE_MODULE_SIZE 26 * sizeof(Con_Int)

#define CON_BYTECODE_IMPORT_IDENTIFIER_SIZE 0 * sizeof(Con_Int)
#define CON_BYTECODE_IMPORT_IDENTIFIER 1 * sizeof(Con_Int)

#define CON_BYTECODE_TOP_LEVEL_VAR_NUM 0 * sizeof(Con_Int)
#define CON_BYTECODE_TOP_LEVEL_VAR_NAME_SIZE 1 * sizeof(Con_Int)
#define CON_BYTECODE_TOP_LEVEL_VAR_NAME 2 * sizeof(Con_Int)

#define CON_BYTECODE_IMPORT_TYPE_BUILTIN 0
#define CON_BYTECODE_IMPORT_TYPE_USER_MOD 1

// 32-bit little endian instructions

#define CON_INSTR_EXBI 1					// bits 0-7 1, bits 8-31 size of field name, bits 32-.. field name
#define CON_INSTR_VAR_LOOKUP 2  			// bits 0-7 2, bits 8-19 closures offset, bits 20-31 var number
#define CON_INSTR_VAR_ASSIGN 3  			// bits 0-7 3, bits 8-19 closures offset, bits 20-31 var number
#define CON_INSTR_INT 4 					// bits 0-7 4, bits 8-30 integer value, bit 31 sign (0 = positive, 1 = negative)
#define CON_INSTR_ADD_FAILURE_FRAME 5		// bits 0-7 5, bits 8-30 pc offset, bit 31 offset sign
#define CON_INSTR_ADD_FAIL_UP_FRAME 6		// bits 0-7 6
#define CON_INSTR_REMOVE_FAILURE_FRAME 7	// bits 0-7 7
#define CON_INSTR_IS_ASSIGNED 8 			// bits 0-7 8, bits 8-19 closures offset, bits 20-31 var number
											// bits 32-39 := 0, 40-62 := pc offset, bit 63 := offset sign
#define CON_INSTR_IS 9  					// bits 0-7 9
#define CON_INSTR_FAIL_NOW 10				// bits 0-7 10
#define CON_INSTR_POP 11					// bits 0-7 11
#define CON_INSTR_LIST 12					// bits 0-7 12, bits 8-31 number of list elements
#define CON_INSTR_SLOT_LOOKUP 13			// bits 0-7 13, bits 8-31 size of slot name, bits 32-.. slot name
#define CON_INSTR_APPLY 14  				// bits 0-7 14, bits 8-31 number of args
#define CON_INSTR_FUNC_DEF 15				// bits 0-7 15, bits 8-9 is_bound
#define CON_INSTR_RETURN 16 				// bits 0-7 16
#define CON_INSTR_BRANCH 17 				// bits 0-7 17, bits 8-30 pc offset, bit 31 offset sign (0 = positive, 1 = negative)
#define CON_INSTR_YIELD 18  				// bits 0-7 18
#define CON_INSTR_IMPORT 20 				// bits 0-7 20, bits 8-31 module number
#define CON_INSTR_DICT 21					// bits 0-7 21, bits 8-31 number of dictionary elements
#define CON_INSTR_DUP 22					// bits 0-7 22
#define CON_INSTR_PULL 23					// bits 0-7 23, bits 8-31 := number of entries back in the stack to pull the value from
#define CON_INSTR_CHANGE_FAIL_POINT 24  	// bits 0-7 24, bits 8-30 pc offset, bit 31 offset sign
#define CON_INSTR_STRING 25 				// bits 0-7 25, bits 8-31 size of string, bits 32-.. string
#define CON_INSTR_BUILTIN_LOOKUP 26 		// bits 0-7 26, bits 8-15 builtin number
#define CON_INSTR_ASSIGN_SLOT 27			// bits 0-7 13, bits 8-31 size of slot name, bits 32-.. slot name
#define CON_INSTR_EYIELD 28 				// bits 0-7 28
#define CON_INSTR_ADD_EXCEPTION_FRAME 29	// bits 0-7 5, bits 8-30 pc offset, bit 31 offset sign
#define CON_INSTR_INSTANCE_OF 31			// bits 0-7 31
#define CON_INSTR_REMOVE_EXCEPTION_FRAME 32 // bits 0-7 32
#define CON_INSTR_RAISE 33  				// bits 0-7 33
#define CON_INSTR_SET_ITEM 34				// bits 0-7 34
#define CON_INSTR_UNPACK_ARGS 35			// bits 0-7 := 35, bits 8-15 := num normal args, bit 16 := has var args
											// each variable is then a subsequent word:
											//   word 1: 0-11 := var number, bit 12 := is mandatory arg
#define CON_INSTR_SET 36					// bits 0-7 36
#define CON_INSTR_BRANCH_IF_NOT_FAIL 37 	// bits 0-7 37, bits 8-30 pc offset, bit 31 offset sign
#define CON_INSTR_BRANCH_IF_FAIL 38 		// bits 0-7 38, bits 8-30 pc offset, bit 31 offset sign
#define CON_INSTR_CONSTANT_GET 39	 		// bits 0-7 39, bits 8-30 constant num
#define CON_INSTR_CONSTANT_SET 40	 		// bits 0-7 40, bits 8-30 constant num
#define CON_INSTR_PRE_SLOT_LOOKUP_APPLY 41  // bits 0-7 41, bits 8-31 size of slot name, bits 32-.. slot name
#define CON_INSTR_UNPACK_ASSIGN 42			// bits 0-7 42, bits 8-31 number of elements to unpack
#define CON_INSTR_EQ 43						// bits 0-7 43
#define CON_INSTR_LE 44						// bits 0-7 44
#define CON_INSTR_ADD 45					// bits 0-7 45
#define CON_INSTR_SUBTRACT 46				// bits 0-7 46
#define CON_INSTR_NEQ 47					// bits 0-7 47
#define CON_INSTR_LE_EQ 48					// bits 0-7 48
#define CON_INSTR_GR_EQ 49					// bits 0-7 49
#define CON_INSTR_GT 50						// bits 0-7 50
#define CON_INSTR_MODULE_LOOKUP 51			// bits 0-7 51, bits 8-31 := size of definition name, bits 32-.. := definition name


#define CON_INSTR_DECODE_EXBI_SIZE(instruction) ((instruction & 0xFFFFFF00) >> 8)
#define CON_INSTR_DECODE_EXBI_START(instruction) sizeof(uint32_t)

#define CON_INSTR_DECODE_VAR_ASSIGN_CLOSURES_OFFSET(instruction) ((instruction & 0x000FFF00) >> 8)
#define CON_INSTR_DECODE_VAR_ASSIGN_VAR_NUM(instruction) ((instruction & 0xFFF00000) >> 20)

#define CON_INSTR_DECODE_VAR_LOOKUP_CLOSURES_OFFSET(instruction) ((instruction & 0x000FFF00) >> 8)
#define CON_INSTR_DECODE_VAR_LOOKUP_VAR_NUM(instruction) ((instruction & 0xFFF00000) >> 20)

#define CON_INSTR_DECODE_INT_VAL(instruction) ((instruction & 0x7FFFFF00) >> 8)
#define CON_INSTR_DECODE_INT_SIGN(instruction) ((instruction & 0x80000000) >> 8)

#define CON_INSTR_DECODE_LIST_NUM_ENTRIES(instruction) ((instruction & 0xFFFFFF00) >> 8)

#define CON_INSTR_DECODE_SLOT_LOOKUP_SIZE(instruction) ((instruction & 0xFFFFFF00) >> 8)
#define CON_INSTR_DECODE_SLOT_LOOKUP_START(instruction) sizeof(uint32_t)

#define CON_INSTR_DECODE_APPLY_NUM_ARGS(instruction) ((instruction & 0xFFFFFF00) >> 8)

#define CON_INSTR_DECODE_ADD_FAILURE_FRAME_OFFSET(instruction) ((instruction & 0x7FFFFF00) >> 8)
#define CON_INSTR_DECODE_ADD_FAILURE_FRAME_SIGN(instruction) ((instruction & 0x80000000) >> 8)

#define CON_INSTR_DECODE_IS_ASSIGNED_CLOSURES_OFFSET(instruction) ((instruction & 0x000FFF00) >> 8)
#define CON_INSTR_DECODE_IS_ASSIGNED_VAR_NUM(instruction) ((instruction & 0xFFF00000) >> 20)
#define CON_INSTR_DECODE_IS_ASSIGNED_BRANCH_OFFSET(instruction) ((instruction & 0x7FFFFF00) >> 8)
#define CON_INSTR_DECODE_IS_ASSIGNED_BRANCH_SIGN(instruction) ((instruction & 0x80000000) >> 8)

#define CON_INSTR_DECODE_IMPORT_MOD_NUM(instruction) ((instruction & 0xFFFFFF00) >> 8)

#define CON_INSTR_DECODE_IMPORT_BUILTIN_LOOKUP(instruction) ((instruction & 0xFFFFFF00) >> 8)

#define CON_INSTR_DECODE_SLOT_ASSIGN_SIZE(instruction) ((instruction & 0xFFFFFF00) >> 8)

#define CON_INSTR_DECODE_DICT_NUM_ENTRIES(instruction) ((instruction & 0xFFFFFF00) >> 8)

#define CON_INSTR_DECODE_PULL_N(instruction) ((instruction & 0xFFFFFF00) >> 8)

#define CON_INSTR_DECODE_BRANCH_OFFSET(instruction) ((instruction & 0x7FFFFF00) >> 8)
#define CON_INSTR_DECODE_BRANCH_SIGN(instruction) ((instruction & 0x80000000) >> 8)

#define CON_INSTR_DECODE_FUNC_DEF_IS_BOUND(instruction) ((instruction & 0x00000100) >> 8)

#define CON_INSTR_DECODE_STRING_SIZE(instruction) ((instruction & 0xFFFFFF00) >> 8)
#define CON_INSTR_DECODE_STRING_START(instruction) sizeof(uint32_t)

#define CON_INSTR_DECODE_ASSIGN_SLOT_SIZE(instruction) ((instruction & 0xFFFFFF00) >> 8)
#define CON_INSTR_DECODE_ASSIGN_SLOT_START(instruction) sizeof(uint32_t)

#define CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction) ((instruction & 0x0000FF00) >> 8)
#define CON_INSTR_DECODE_UNPACK_ARGS_HAS_VAR_ARGS(instruction) ((instruction & 0x00010000) >> 16)
#define CON_INSTR_DECODE_UNPACK_ARGS_ARG_NUM(instruction) (instruction & 0x000000FF)
#define CON_INSTR_DECODE_UNPACK_ARGS_IS_MANDATORY(instruction) ((instruction & 0x00000100) >> 8)

#define CON_INSTR_DECODE_ADD_BRANCH_IF_NOT_FAIL_OFFSET(instruction) ((instruction & 0x7FFFFF00) >> 8)
#define CON_INSTR_DECODE_ADD_BRANCH_IF_NOT_FAIL_SIGN(instruction) ((instruction & 0x80000000) >> 8)

#define CON_INSTR_DECODE_BRANCH_IF_FAIL_OFFSET(instruction) ((instruction & 0x7FFFFF00) >> 8)
#define CON_INSTR_DECODE_BRANCH_IF_FAIL_SIGN(instruction) ((instruction & 0x80000000) >> 8)

#define CON_INSTR_DECODE_CONSTANT_GET_NUM(instruction) ((instruction & 0xFFFFFF00) >> 8)

#define CON_INSTR_DECODE_CONSTANT_SET_NUM(instruction) ((instruction & 0xFFFFFF00) >> 8)

#define CON_INSTR_DECODE_PRE_SLOT_LOOKUP_SIZE(instruction) ((instruction & 0xFFFFFF00) >> 8)
#define CON_INSTR_DECODE_PRE_SLOT_LOOKUP_START(instruction) sizeof(uint32_t)

#define CON_INSTR_DECODE_UNPACK_ASSIGN_NUM_ELEMENTS(instruction) ((instruction & 0xFFFFFF00) >> 8)

#define CON_INSTR_DECODE_SET_CLASS_FIELD_SIZE(instruction) ((instruction & 0xFFFFFF00) >> 8)

#define CON_INSTR_DECODE_MODULE_LOOKUP_SIZE(instruction) ((instruction & 0xFFFFFF00) >> 8)
#define CON_INSTR_DECODE_MODULE_LOOKUP_START(instruction) sizeof(uint32_t)
