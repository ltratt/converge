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


#include "Config.h"

#include "Target.h"




const char *Con_Instructions_names[52] = {"**unused**", "ASSIGN", "VAR_LOOKUP", "VAR_ASSIGN", "INT", "ADD_FAILURE_FRAME", "ADD_FAIL_UP_FRAME", "REMOVE_FAILURE_FRAME", "IS_ASSIGNED", "IS", "FAIL_NOW", "POP", "LIST", "SLOT_LOOKUP", "APPLY", "FUNC_DEF", "RETURN", "BRANCH", "YIELD", "**unused**", "IMPORT", "DICT", "DUP", "PULL", "CHANGE_FAIL_POINT", "STRING", "BUILTIN_LOOKUP", "ASSIGN_SLOT", "EYIELD", "ADD_EXCEPTION_FRAME", "**unused**", "INSTANCE_OF", "REMOVE_EXCEPTION_FRAME", "RAISE", "SET_ITEM", "UNPACK_ARGS", "SET", "BRANCH_IF_NOT_FAIL", "BRANCH_IF_FAIL", "CONSTANT_GET", "CONSTANT_SET", "PRE_SLOT_LOOKUP_APPLY", "UNPACK_ASSIGN", "EQ", "LE", "ADD", "SUBTRACT", "NEQ", "LE_EQ", "GR_EQ", "GT", "MODULE_LOOKUP"};
