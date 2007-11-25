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


#ifndef _CON_CORE_H
#define _CON_CORE_H

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>



#ifndef CON_HAVE_U_TYPES

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;

#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Builtins
//

#define CON_NUMBER_OF_BUILTINS 40

#define CON_BUILTIN_NULL_OBJ 0
#define CON_BUILTIN_FAIL_OBJ 1

// Core atom defs

#define CON_BUILTIN_ATOM_DEF_OBJECT 2
#define CON_BUILTIN_SLOTS_ATOM_DEF_OBJECT 3
#define CON_BUILTIN_CLASS_ATOM_DEF_OBJECT 4
#define CON_BUILTIN_VM_ATOM_DEF_OBJECT 5
#define CON_BUILTIN_THREAD_ATOM_DEF_OBJECT 6
#define CON_BUILTIN_FUNC_ATOM_DEF_OBJECT 7
#define CON_BUILTIN_STRING_ATOM_DEF_OBJECT 8
#define CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT 9
#define CON_BUILTIN_LIST_ATOM_DEF_OBJECT 10
#define CON_BUILTIN_DICT_ATOM_DEF_OBJECT 11
#define CON_BUILTIN_MODULE_ATOM_DEF_OBJECT 12
#define CON_BUILTIN_INT_ATOM_DEF_OBJECT 13
#define CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT 14
#define CON_BUILTIN_CLOSURE_ATOM_DEF_OBJECT 15
#define CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT 16
#define CON_BUILTIN_EXCEPTION_ATOM_DEF_OBJECT 17
#define CON_BUILTIN_SET_ATOM_DEF_OBJECT 18

// Core classes

#define CON_BUILTIN_OBJECT_CLASS 19
#define CON_BUILTIN_CLASS_CLASS 20
#define CON_BUILTIN_VM_CLASS 21
#define CON_BUILTIN_THREAD_CLASS 22
#define CON_BUILTIN_FUNC_CLASS 23
#define CON_BUILTIN_STRING_CLASS 24
#define CON_BUILTIN_CON_STACK_CLASS 25
#define CON_BUILTIN_LIST_CLASS 26
#define CON_BUILTIN_DICT_CLASS 27
#define CON_BUILTIN_MODULE_CLASS 28
#define CON_BUILTIN_INT_CLASS 29
#define CON_BUILTIN_CLOSURE_CLASS 30
#define CON_BUILTIN_PARTIAL_APPLICATION_CLASS 31
#define CON_BUILTIN_EXCEPTION_CLASS 32
#define CON_BUILTIN_SET_CLASS 33
#define CON_BUILTIN_NUMBER_CLASS 34

#define CON_BUILTIN_C_FILE_MODULE 35
#define CON_BUILTIN_EXCEPTIONS_MODULE 36
#define CON_BUILTIN_SYS_MODULE 37

// Floats

#define CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT 38
#define CON_BUILTIN_FLOAT_CLASS 39



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Misc
//

typedef CON_INT Con_Int;
typedef CON_FLOAT Con_Float;

typedef unsigned long Con_Hash;

#if defined(CON_THREADS_SINGLE_THREADED)
	typedef Con_Int Con_Mutex;
	typedef Con_Int Con_Thread_Condition;
#elif defined(CON_THREADS_PTHREADS)
#	include <pthread.h>
	typedef pthread_mutex_t Con_Mutex;
	typedef pthread_cond_t Con_Thread_Condition;
#else
#	error "Unknown thread library"
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory
//

typedef struct Con_Memory_Store Con_Memory_Store;



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Objects
//

// Objects consist of a pointer to a mutex, and one or more atoms bonded together. Objects MUST
// occupy a contiguous area of memory. Each atom has a low-level type. An object MUST not contain
// more than one atom of the same type.
//
// An objects mutex applies equally to each of its atoms. To read or write data from an atom, the
// mutex of the object it constitutes part of MUST be held.
//
// The number of, and type of each, atom is fixed for an objects lifetime. Once an object is fully
// initialized one can thus iterate over its atoms without holding the object lock.
//
// Objects have a number of flags associated with them:
//
//   virgin          : whether this object has been altered since being created from a virgin
//                     class. In order for this to be meaningful, the first atom in the object must
//                     of the corresponding type of the class this object was created from i.e., an
//                     object which is virgin and whose first atom's type is INT_ATOM_DEF is
//                     assumed to be descended from the INT_CLASS. Violating this latter assumption
//                     will lead to unspecified behaviour; objects which can not adhere to this
//                     assumption should be marked as non-virgin upon creation.
//                     
//                     If an object is virgin, it means it implements the default behaviour. This
//                     means that code can quickly inspect an object and, if it is of a type the
//                     code understands, and if the object is virgin, short cut behaviour can be
//                     invoked (e.g., for efficiency reasons). Obviously it is expected that any
//                     such shortcut behaviour respects the default behaviour that is being
//                     short-cut.
//
//                     This means that the Converge object system has the following important
//                     properties:
//
//                       * Only classes which define a corresponding atom definition can be
//                         marked as virgin.
//                       * A virgin class creates virgin objects by default.
//                       * A non-virgin class can never create a virgin object.
//                       * If a virgin object's slots are altered it immediately and permanently
//                         loses its virgin status.
//                       * If a virgin class is altered it immediately and permanently loses its
//                         virgin status.
//                       * Once a virgin class creates a virgin object, that object is entirely
//                         independent of the class. i.e. the objects virginity remains intact even
//                         if its creating class subsequently loses that status.
//
//   custom_get_slot : whether this object defines a non-default get_slot function
//   custom_set_slot : whether this object defines a non-default get_slot function
//   custom_find_slot : whether this object defines a non-default get_slot function

typedef struct Con_Obj Con_Obj;

#define CON_ATOM_HEAD Con_Obj *atom_type; \
	struct _Con_Atom *next_atom;

typedef struct _Con_Atom {
	CON_ATOM_HEAD
} Con_Atom;

typedef struct Con_Slots Con_Slots;

struct Con_Obj {
	size_t size;
	Con_Mutex mutex;
	// creator_slots, if non-NULL, refers to another class's proto_slots_for_clones field. Thus
	// the slots pointed to by creator slots are immutable and no mutex needs to be held to access
	// them.
	Con_Slots *creator_slots;
	unsigned int
		virgin : 1, custom_get_slot : 1, custom_set_slot : 1, custom_find_slot : 1;
	Con_Atom first_atom[0];
};



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Closures and continuations
//

typedef enum {PC_TYPE_BYTECODE, PC_TYPE_C_FUNCTION, PC_TYPE_NULL} Con_PC_Type;

typedef Con_Obj * Con_C_Function(Con_Obj *);

typedef struct {
	Con_PC_Type type;
	Con_Obj *module;
	union {
		Con_Int bytecode_offset;
		Con_C_Function *c_function;
	} pc;
} Con_PC;



/////////////////////////////////////////////////////////////////////////////////////////////////////
// String encodings
//

typedef enum {CON_STR_UCS_2, CON_STR_UCS_2BE, CON_STR_UCS_2LE, CON_STR_UCS_4, CON_STR_UCS_4LE, CON_STR_UCS_4BE, CON_STR_UTF_8, CON_STR_UTF_16, CON_STR_UTF_16BE, CON_STR_UTF_16LE, CON_STR_UTF_32, CON_STR_UTF_32BE, CON_STR_UTF_32LE} Con_String_Encoding;

#endif
