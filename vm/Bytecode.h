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


#ifndef _CON_BYTECODE_H
#define _CON_BYTECODE_H

#include "Core.h"

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
#define CON_BYTECODE_MODULE_SIZE 21 * sizeof(Con_Int)

#define CON_BYTECODE_IMPORT_IDENTIFIER_SIZE 0 * sizeof(Con_Int)
#define CON_BYTECODE_IMPORT_IDENTIFIER 1 * sizeof(Con_Int)

#define CON_BYTECODE_TOP_LEVEL_VAR_NUM 0 * sizeof(Con_Int)
#define CON_BYTECODE_TOP_LEVEL_VAR_NAME_SIZE 1 * sizeof(Con_Int)
#define CON_BYTECODE_TOP_LEVEL_VAR_NAME 2 * sizeof(Con_Int)

#define CON_BYTECODE_IMPORT_TYPE_BUILTIN 0
#define CON_BYTECODE_IMPORT_TYPE_USER_MOD 1

Con_Obj *Con_Bytecode_add_executable(Con_Obj *, u_char *);
Con_Obj *Con_Bytecode_add_module(Con_Obj *, u_char *);

#endif
