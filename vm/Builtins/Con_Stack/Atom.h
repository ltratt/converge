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


#ifndef _CON_ATOMS_BUILTINS_CON_STACK_ATOM_H
#define _CON_ATOMS_BUILTINS_CON_STACK_ATOM_H

#include "Core.h"
#include <setjmp.h>

typedef enum {CON_BUILTINS_CON_STACK_CLASS_CONTINUATION_FRAME, CON_BUILTINS_CON_STACK_CLASS_GENERATOR_FRAME, CON_BUILTINS_CON_STACK_CLASS_GENERATOR_EYIELD_FRAME, CON_BUILTINS_CON_STACK_CLASS_FAILURE_FRAME, CON_BUILTINS_CON_STACK_CLASS_EXCEPTION_FRAME, CON_BUILTINS_CON_STACK_SLOT_LOOKUP_APPLY, CON_BUILTINS_CON_STACK_CLASS_OBJECT} Con_Builtins_Con_Stack_Class_Type;

typedef struct {
	Con_Obj *func, *closure;
	Con_PC pc;
	Con_Int prev_gfp, prev_ffp, prev_xfp, prev_cfp;
	sigjmp_buf return_env;
	Con_PC resumption_pc;
	u_char *c_stack_start; // Real address.
	bool return_as_generator;
} Con_Builtins_Con_Stack_Class_Continuation_Frame;

typedef struct {
	Con_Int prev_gfp;
	
	Con_PC resumption_pc;
	u_char *suspended_c_stack; // Address of copy.
	Con_Int suspended_c_stack_size; // Size of copy.
	Con_Int con_stack_start; // Real start offset.
	u_char *suspended_con_stack; // Address of copy.
	Con_Int suspended_con_stack_size; // Size of copy.
	sigjmp_buf suspended_env;
	Con_Int suspended_gfp, suspended_ffp, suspended_xfp, suspended_cfp, suspended_stackp;
	
	bool returned;
} Con_Builtins_Con_Stack_Class_Generator_Frame;

typedef struct {
	Con_Int prev_gfp;
	Con_PC resumption_pc;
} Con_Builtins_Con_Stack_Class_Generator_EYield_Frame;

typedef struct {
	bool is_fail_up;
	Con_PC fail_to_pc;
	Con_Int prev_gfp, prev_ffp;
} Con_Builtins_Con_Stack_Class_Failure_Frame;

typedef struct {
	Con_Int prev_ffp, prev_gfp, prev_xfp;
	sigjmp_buf exception_env;
} Con_Builtins_Con_Stack_Class_Exception_Frame;

typedef struct {
	Con_Obj *self; // If NULL, signifies func should be treated "as is".
	Con_Obj *func;
} Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply;

typedef struct {
	CON_ATOM_HEAD
	u_char *stack;
	// stackp is the current extent of the stack; at all times stackp < stack_allocated
	Con_Int stack_allocated, stackp;
	// gfp : generator frame pointer
	// ffp : failure frame pointer
	// xfp : exception frame pointer
	// cfp : continuation frame pointer
	Con_Int gfp, ffp, xfp, cfp;
	
	Con_Int continuation_id_counter;
} Con_Builtins_Con_Stack_Atom;


void Con_Builtins_Con_Stack_Atom_bootstrap(Con_Obj *);

Con_Obj *Con_Builtins_Con_Stack_Atom_new(Con_Obj *);
void Con_Builtins_Con_Stack_Atom_init_atom(Con_Obj *, Con_Builtins_Con_Stack_Atom *);

#if CON_FULL_DEBUG
void Con_Builtins_Con_Stack_Atom_print(Con_Obj *, Con_Obj *);
#endif

Con_PC Con_Builtins_Con_Stack_Atom_get_continuation_pc(Con_Obj *, Con_Obj *, Con_Int);

void Con_Builtins_Con_Stack_Atom_add_continuation_frame(Con_Obj *, Con_Obj *, Con_Obj *, Con_Int, Con_Obj *, Con_PC, bool);
void Con_Builtins_Con_Stack_Atom_remove_continuation_frame(Con_Obj *, Con_Obj *);
bool Con_Builtins_Con_Stack_Atom_continuation_return_as_generator(Con_Obj *, Con_Obj *);
bool Con_Builtins_Con_Stack_Atom_continuation_has_generator_frame(Con_Obj *, Con_Obj *);
void Con_Builtins_Con_Stack_Atom_read_continuation_frame(Con_Obj *, Con_Obj *, Con_Obj **, Con_Obj **, Con_PC *);
void Con_Builtins_Con_Stack_Atom_update_continuation_frame(Con_Obj *, Con_Obj *, u_char *, sigjmp_buf);
void Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(Con_Obj *, Con_Obj *, Con_PC);

void Con_Builtins_Con_Stack_Atom_prepare_to_return_from_generator(Con_Obj *, Con_Obj *, u_char **, u_char **, Con_Int *, sigjmp_buf *);
void Con_Builtins_Con_Stack_Atom_update_generator_frame(Con_Obj *, Con_Obj *, u_char *, Con_Int, sigjmp_buf);
void Con_Builtins_Con_Stack_Atom_prepare_generator_frame_reexecution(Con_Obj *, Con_Obj *, u_char **, u_char **, Con_Int *, sigjmp_buf *);
void Con_Builtins_Con_stack_Atom_add_generator_eyield_frame(Con_Obj *, Con_Obj *, Con_PC);
void Con_Builtins_Con_Stack_Atom_read_generator_frame(Con_Obj *, Con_Obj *, bool *, Con_PC *);
void Con_Builtins_Con_Stack_Atom_set_generator_returned(Con_Obj *, Con_Obj *);
bool Con_Builtins_Con_Stack_Atom_has_generator_returned(Con_Obj *, Con_Obj *);
void Con_Builtins_Con_Stack_Atom_remove_generator_frame(Con_Obj *, Con_Obj *);

void Con_Builtins_Con_Stack_Atom_add_failure_frame(Con_Obj *, Con_Obj *, Con_PC);
void Con_Builtins_Con_Stack_Atom_add_failure_fail_up_frame(Con_Obj *, Con_Obj *);
void Con_Builtins_Con_Stack_Atom_read_failure_frame(Con_Obj *, Con_Obj *, bool *, bool *, Con_PC *);
void Con_Builtins_Con_Stack_Atom_remove_failure_frame(Con_Obj *, Con_Obj *);

void Con_Builtins_Con_Stack_Atom_add_exception_frame(Con_Obj *, Con_Obj *, sigjmp_buf);
void Con_Builtins_Con_Stack_Atom_read_exception_frame(Con_Obj *, Con_Obj *, bool *, sigjmp_buf *);
void Con_Builtins_Con_Stack_Atom_remove_exception_frame(Con_Obj *, Con_Obj *);

void Con_Builtins_Con_Stack_Atom_push_n_object(Con_Obj *, Con_Obj *, Con_Obj *, Con_Int);
void Con_Builtins_Con_Stack_Atom_push_object(Con_Obj *, Con_Obj *, Con_Obj *);
Con_Obj *Con_Builtins_Con_Stack_Atom_pop_n_object(Con_Obj *, Con_Obj *, Con_Int);
Con_Obj *Con_Builtins_Con_Stack_Atom_pop_object(Con_Obj *, Con_Obj *);
Con_Obj *Con_Builtins_Con_Stack_Atom_peek_object(Con_Obj *, Con_Obj *);
void Con_Builtins_Con_Stack_Atom_push_slot_lookup_apply(Con_Obj *, Con_Obj *, Con_Obj *, Con_Obj *);
bool Con_Builtins_Con_Stack_Atom_pop_n_object_or_slot_lookup_apply(Con_Obj *, Con_Obj *, Con_Int, Con_Obj **);
void Con_Builtins_Con_Stack_Atom_swap(Con_Obj *, Con_Obj *);

void Con_Builtins_Con_Stack_Atom_unpack_args(Con_Obj *, Con_Obj *, const char *, ...);

#endif
