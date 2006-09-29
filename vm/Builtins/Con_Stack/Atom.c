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

#include <stdarg.h>
#include <string.h>

#include "Core.h"
#include "Hash.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Exception/Class.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_Con_Stack_Class_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Con_Stack_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *con_stack_atom_def = CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT);
	
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) con_stack_atom_def->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (atom_def_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Con_Stack_Class_gc_scan_func, NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, con_stack_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Con_Stack creation functions
//

Con_Obj *Con_Builtins_Con_Stack_Atom_new(Con_Obj *thread)
{
	Con_Obj *new_con_stack;
	
	new_con_stack = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Con_Stack_Atom), CON_BUILTIN(CON_BUILTIN_CON_STACK_CLASS));
	Con_Builtins_Con_Stack_Atom_init_atom(thread, (Con_Builtins_Con_Stack_Atom *) new_con_stack->first_atom);
	new_con_stack->first_atom->next_atom = NULL;
	
	Con_Memory_change_chunk_type(thread, new_con_stack, CON_MEMORY_CHUNK_OBJ);
	
	return new_con_stack;
}



void Con_Builtins_Con_Stack_Atom_init_atom(Con_Obj *thread, Con_Builtins_Con_Stack_Atom *con_stack_atom)
{
	con_stack_atom->atom_type = CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT);
	con_stack_atom->stack = Con_Memory_malloc(thread, CON_DEFAULT_CON_STACK_SIZE, CON_MEMORY_CHUNK_OPAQUE);
	con_stack_atom->stack_allocated = CON_DEFAULT_CON_STACK_SIZE;
	con_stack_atom->stackp = 0;
	con_stack_atom->gfp = con_stack_atom->ffp = con_stack_atom->xfp = con_stack_atom->cfp = -1;
	con_stack_atom->continuation_id_counter = 0;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Con_Stack operations
//

// ENSURE_ROOM makes sure that the 'con_stack' variable in scope has at least i bytes of spare
// storage. If not, it is enlarged appropriately.
//
// The mutex on Con_Stack pointed to by 'con_stack' must be held when this macro is called, and one
// should be aware that it may alter the value of 'con_stack_atom->stack'.

#define ENSURE_ROOM(i) Con_Memory_make_array_room(thread, (void **) &con_stack_atom->stack, &con_stack->mutex, &con_stack_atom->stack_allocated, &con_stack_atom->stackp, i, 1)




#if CON_FULL_DEBUG
void Con_Builtins_Con_Stack_Atom_print(Con_Obj *thread, Con_Obj *con_stack)
{
	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));
	
	printf("[cfp %d ffp %d gfp %d] ", con_stack_atom->cfp, con_stack_atom->ffp, con_stack_atom->gfp);
	
	Con_Int stackp = con_stack_atom->stackp;
	while (stackp > 0) {
		switch (*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + stackp)) - 1)) {
			case CON_BUILTINS_CON_STACK_CLASS_EXCEPTION_FRAME:
				printf("E ");
				stackp -= sizeof(Con_Builtins_Con_Stack_Class_Exception_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type);
				break;
				CON_XXX;
			case CON_BUILTINS_CON_STACK_CLASS_CONTINUATION_FRAME:
				printf("C ");
				stackp -= sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type);
				break;
			case CON_BUILTINS_CON_STACK_CLASS_GENERATOR_FRAME:
				printf("Gn ");
//				printf("G prev=%d ", ((Con_Builtins_Con_Stack_Class_Generator_Frame *) (con_stack_atom->stack + stackp - sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type)))->prev_gfp);
				stackp -= sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type);
				break;
			case CON_BUILTINS_CON_STACK_CLASS_GENERATOR_EYIELD_FRAME:
				printf("Ge ");
//				printf("Ge prev=%d ", ((Con_Builtins_Con_Stack_Class_Generator_Frame *) (con_stack_atom->stack + stackp - sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type)))->prev_gfp);
				stackp -= sizeof(Con_Builtins_Con_Stack_Class_Generator_EYield_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type);
				break;
			case CON_BUILTINS_CON_STACK_CLASS_FAILURE_FRAME: {
				Con_Builtins_Con_Stack_Class_Failure_Frame *failure_frame = (Con_Builtins_Con_Stack_Class_Failure_Frame *) (con_stack_atom->stack + stackp - sizeof(Con_Builtins_Con_Stack_Class_Failure_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));
				if (failure_frame->is_fail_up)
					printf("Fu ");
				else
					printf("Ff ");
				stackp -= sizeof(Con_Builtins_Con_Stack_Class_Failure_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type);
				break;
			}
			case CON_BUILTINS_CON_STACK_CLASS_OBJECT:
				//printf("O%p ", *((Con_Obj *) (con_stack_atom->stack + stackp - sizeof(Con_Obj *) - sizeof(Con_Builtins_Con_Stack_Class_Type))));
				printf("O ");
				stackp -= sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
				break;
			case CON_BUILTINS_CON_STACK_SLOT_LOOKUP_APPLY:
				//printf("O%p ", *((Con_Obj *) (con_stack_atom->stack + stackp - sizeof(Con_Obj *) - sizeof(Con_Builtins_Con_Stack_Class_Type))));
				printf("Os ");
				stackp -= sizeof(Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply) + sizeof(Con_Builtins_Con_Stack_Class_Type);
				break;
			default:
				printf("Unknown stack type %d\n", *(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + stackp)) - 1));
				fflush(NULL);
				CON_XXX;
		}
	}
	printf("\n");
}
#endif



Con_PC Con_Builtins_Con_Stack_Atom_get_continuation_pc(Con_Obj *thread, Con_Obj *con_stack, Con_Int num_levels_back)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	Con_Builtins_Con_Stack_Class_Continuation_Frame *continuation_frame = (Con_Builtins_Con_Stack_Class_Continuation_Frame *) (con_stack_atom->stack + con_stack_atom->cfp - sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));
	
	while (num_levels_back > 0) {
		if (continuation_frame->prev_cfp == -1)
			CON_XXX;
			
		continuation_frame = (Con_Builtins_Con_Stack_Class_Continuation_Frame *) (con_stack_atom->stack + continuation_frame->prev_cfp - sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));
		
		num_levels_back -= 1;
	}
	
	return continuation_frame->pc;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Continuation frames
//

//
// Add a continuation frame to the con_stack. 'func' is the function the continuation is an instance
// of, 'num_args' is the number of arguments being passed to the function (these will be popped from
// the existing stack and placed onto the continuations stack) and 'closure' is the functions closure.
// If 'func' was applied in bytecode, 'resumption_pc' should point to just after the APPLY bytecode
// to cater for the case this continuation is a generator and is suspended and subsequently resumed.
//
// To be more accurate, the continuation frame_stack is inserted into the Con_Stack num_args *
// CON_BUILTINS_CON_STACK_CLASS_OBJECT's back into the stack. This means that the typical function
// application sequence of 1) push args 2) add frame 3) pull args 4) call 5) pop args becomes 1) push
// args 2) add frame and move args at same time 3) call 4) pop args.
//
// If 'return_as_generator' is true, a generator frame is added before the continuation frame on the
// stack. This functionality is here because generator frames are never added to the stack unless a
// continuation frame is added immediately elsewhere; by adding the generator frame here we're able
// to make a reasonable efficiency saving.
//

void Con_Builtins_Con_Stack_Atom_add_continuation_frame(Con_Obj *thread, Con_Obj *con_stack, Con_Obj *func, Con_Int num_args, Con_Obj *closure, Con_PC resumption_pc, bool return_as_generator)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

#	ifndef NDEBUG
	Con_Int stackp = con_stack_atom->stackp;
	for (int i = 0; i < num_args; i += 1) {
		assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + stackp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_OBJECT);
		stackp -= sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
	}
#	endif

	Con_PC func_pc = Con_Builtins_Func_Atom_get_pc(thread, func);

	// At this point the Con_Stack looks like:
	//   [..., <arg obj 1>, ..., <arg obj n>]

	Con_Builtins_Con_Stack_Class_Continuation_Frame *continuation_frame;
	Con_Int continuation_frame_offset;
	Con_Builtins_Con_Stack_Class_Generator_Frame *generator_frame = NULL;
	Con_Int generator_frame_offset = -1;
	if (return_as_generator) {
		ENSURE_ROOM(sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type) + sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type));
		
		generator_frame_offset = con_stack_atom->stackp - num_args * (sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type));
		continuation_frame_offset = con_stack_atom->stackp - num_args * (sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type)) + sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type);
		
		generator_frame = (Con_Builtins_Con_Stack_Class_Generator_Frame *) (con_stack_atom->stack + generator_frame_offset);
		continuation_frame = (Con_Builtins_Con_Stack_Class_Continuation_Frame *) (con_stack_atom->stack + continuation_frame_offset);
		memmove((u_char *) (continuation_frame + 1) + sizeof(Con_Builtins_Con_Stack_Class_Type), generator_frame, num_args * (sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type)));

		// At this point the Con_Stack looks like:
		//   [..., <incomplete generator frame>, <incomplete continuation frame>, <arg obj 1>,
		//    ..., <arg obj n>]
	}
	else {
		ENSURE_ROOM(sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type));

		continuation_frame_offset = con_stack_atom->stackp - num_args * (sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type));
		
		continuation_frame = (Con_Builtins_Con_Stack_Class_Continuation_Frame *) (con_stack_atom->stack + continuation_frame_offset);
		memmove((u_char *) (continuation_frame + 1) + sizeof(Con_Builtins_Con_Stack_Class_Type), continuation_frame, num_args * (sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type)));
		
		// At this point the Con_Stack looks like:
		//   [..., <incomplete continuation frame>, <arg obj 1>, ..., <arg obj n>]
	}

	// Shuffle the current objects on the stack to make room for the continuation frame.

	continuation_frame->prev_gfp = con_stack_atom->gfp;
	continuation_frame->prev_ffp = con_stack_atom->ffp;
	continuation_frame->prev_xfp = con_stack_atom->xfp;
	continuation_frame->prev_cfp = con_stack_atom->cfp;
	continuation_frame->func = func;
	continuation_frame->closure = closure;
	continuation_frame->pc = func_pc;
	continuation_frame->resumption_pc = resumption_pc;
	continuation_frame->return_as_generator = return_as_generator;

	*((Con_Builtins_Con_Stack_Class_Type *) (continuation_frame + 1)) = CON_BUILTINS_CON_STACK_CLASS_CONTINUATION_FRAME;

	if (return_as_generator) {	
		*((Con_Builtins_Con_Stack_Class_Type *) (generator_frame + 1)) = CON_BUILTINS_CON_STACK_CLASS_GENERATOR_FRAME;
		generator_frame->prev_gfp = con_stack_atom->gfp;
		generator_frame->returned = false;
		// These next two assignments signify to different pieces of code that this generator frame
		// isn't fully initialized.
		generator_frame->suspended_c_stack = NULL;
		generator_frame->suspended_con_stack_size = 0;
		
		continuation_frame->prev_gfp = generator_frame_offset + sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type);
		
		con_stack_atom->stackp += sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type) + sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type);
		
		// At this point the Con_Stack looks like:
		//   [..., <generator frame>, <continuation frame>, <arg obj 1>, ..., <arg obj n>]
	}
	else {
		con_stack_atom->stackp += sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type);

		// At this point the Con_Stack looks like:
		//   [..., <continuation frame>, <arg obj 1>, ..., <arg obj n>]
	}

	con_stack_atom->gfp = con_stack_atom->ffp = con_stack_atom->xfp = -1;
	con_stack_atom->cfp = continuation_frame_offset + sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type);
}



//
// Remove the current continuation frame from the stack.
//

void Con_Builtins_Con_Stack_Atom_remove_continuation_frame(Con_Obj *thread, Con_Obj *con_stack)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->cfp > -1);
	assert((con_stack_atom->gfp == -1) || (con_stack_atom->gfp > con_stack_atom->cfp));
	
	Con_Int old_cfp = con_stack_atom->cfp;
	
	Con_Builtins_Con_Stack_Class_Continuation_Frame *continuation_frame = (Con_Builtins_Con_Stack_Class_Continuation_Frame *) (con_stack_atom->stack + con_stack_atom->cfp - sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));
	con_stack_atom->gfp = continuation_frame->prev_gfp;
	con_stack_atom->ffp = continuation_frame->prev_ffp;
	con_stack_atom->xfp = continuation_frame->prev_xfp;
	con_stack_atom->cfp = continuation_frame->prev_cfp;
	con_stack_atom->stackp = old_cfp - sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type);
}



//
// Returns true if this continuation is being pumped for values.
//

bool Con_Builtins_Con_Stack_Atom_continuation_return_as_generator(Con_Obj *thread, Con_Obj *con_stack)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->cfp > -1);
	Con_Builtins_Con_Stack_Class_Continuation_Frame *continuation_frame = (Con_Builtins_Con_Stack_Class_Continuation_Frame *) (con_stack_atom->stack + con_stack_atom->cfp - sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));
	
	return continuation_frame->return_as_generator;
}



//
// Returns true if this continuation is being pumped for values.
//

bool Con_Builtins_Con_Stack_Atom_continuation_has_generator_frame(Con_Obj *thread, Con_Obj *con_stack)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	if (con_stack_atom->gfp == -1)
		return false;
	else
		return true;
}



//
// Read the current continuation frames function, closure and PC. If there is no continuation, 'func'
// will be set to NULL. 'closure' and 'pc' may be NULL in which case a value will not be assigned to
// them.
//

void Con_Builtins_Con_Stack_Atom_read_continuation_frame(Con_Obj *thread, Con_Obj *con_stack, Con_Obj **func, Con_Obj **closure, Con_PC *pc)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	assert(func != NULL);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));
	
	if (con_stack_atom->cfp == -1) {
		*func = NULL;
		return;
	}

	assert(con_stack_atom->cfp > -1);
	Con_Builtins_Con_Stack_Class_Continuation_Frame *continuation_frame = (Con_Builtins_Con_Stack_Class_Continuation_Frame *) (con_stack_atom->stack + con_stack_atom->cfp - sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

	*func = continuation_frame->func;
	if (closure != NULL)
		*closure = continuation_frame->closure;
	if (pc != NULL)
		*pc = continuation_frame->pc;
}



//
// Update information about the current continuation frames C stack. 'c_stack_start' should point to
// the first byte of stack space (be that at the top or bottom of the stack depending on the
// architecture). 'return_env' should be where this continuation will longjmp to if it is a generator
// producing multiple values.
//
// This function is intended only to be called from Con_Builtins_VM_apply_pump; return_env will thus
// longjmp back into Con_Builtins_VM_apply_pump.
//
// It may seem more logical to store this information in a generator frame. While this function
// is called when a function may potentially be pumped for multiple values, it is called before a
// the first call of the function, and thus before we know whether it is a generator or not. At the
// point this function is called, there is consequently no generator frame to put this information
// into.
//

void Con_Builtins_Con_Stack_Atom_update_continuation_frame(Con_Obj *thread, Con_Obj *con_stack, u_char *c_stack_start, sigjmp_buf return_env)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->cfp > -1);
	Con_Builtins_Con_Stack_Class_Continuation_Frame *continuation_frame = (Con_Builtins_Con_Stack_Class_Continuation_Frame *) (con_stack_atom->stack + con_stack_atom->cfp - sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));
	
	continuation_frame->c_stack_start = c_stack_start;
	memmove(continuation_frame->return_env, return_env, sizeof(sigjmp_buf));
}



//
// Update the value of the current continuation frames' PC.
//

void Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(Con_Obj *thread, Con_Obj *con_stack, Con_PC pc)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->cfp > -1);
	Con_Builtins_Con_Stack_Class_Continuation_Frame *continuation_frame = (Con_Builtins_Con_Stack_Class_Continuation_Frame *) (con_stack_atom->stack + con_stack_atom->cfp - sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));
	continuation_frame->pc = pc;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Generator frames
//

//
// There are two types of generator frames: 'normal' generator frames, which represent suspended
// function calls; 'eyield' generator frames which represent the result of executing an alternation
// construct. The later type of generator frame is significantly simpler than the former; most of
// the machinery for generator frames is necessary to support 'normal' generator frames.
//
// There is also a fundamental difference in the way that 'normal' and 'eyield' generator frames
// exist within the runtime. 'Normal' generator frames are added when needed but persist until they
// are finished with. 'Eyield' generator frames are added each time the EYield opcode is encountered,
// and are removed as soon as their value has been consumed.
//



//
// Prepare the stack to return a value from the current continuation to its generator frame.
//
// 'c_stack_start', 'suspended_c_stack', 'suspended_c_stack_size', and 'return_env', which must
// all be pointers to variables, will be set to the value of the appropriate attributes in the
// generator frame and continuation frame.
//

void Con_Builtins_Con_Stack_Atom_prepare_to_return_from_generator(Con_Obj *thread, Con_Obj *con_stack, u_char **c_stack_start, u_char **suspended_c_stack, Con_Int *suspended_c_stack_size, sigjmp_buf *return_env)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->cfp > -1);
	Con_Builtins_Con_Stack_Class_Continuation_Frame *continuation_frame = (Con_Builtins_Con_Stack_Class_Continuation_Frame *) (con_stack_atom->stack + con_stack_atom->cfp - sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

	assert(continuation_frame->prev_gfp > -1);
	Con_Builtins_Con_Stack_Class_Generator_Frame *generator_frame = (Con_Builtins_Con_Stack_Class_Generator_Frame *) (con_stack_atom->stack + continuation_frame->prev_gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

	// At this point the Con_Stack looks like:
	//   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>, <continuation frame>, ...]
	
	Con_Int suspended_con_stack_size = con_stack_atom->stackp - (con_stack_atom->cfp - sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));
	
	*c_stack_start = continuation_frame->c_stack_start;
	*suspended_c_stack = generator_frame->suspended_c_stack;
	*suspended_c_stack_size = generator_frame->suspended_c_stack_size;
	memmove(return_env, continuation_frame->return_env, sizeof(sigjmp_buf));

	if (generator_frame->suspended_con_stack_size < suspended_con_stack_size) {
		if (generator_frame->suspended_con_stack_size == 0) {
			// A suspended con stack has not previously been allocated.
			generator_frame->suspended_con_stack = Con_Memory_malloc(thread, suspended_con_stack_size, CON_MEMORY_CHUNK_CONSERVATIVE);
			generator_frame->suspended_con_stack_size = suspended_con_stack_size;
		}
		else {
			// XXX is this branch even possible?
			printf("%d %d [%d]\n", generator_frame->suspended_con_stack_size, suspended_con_stack_size, generator_frame->prev_gfp);
			CON_XXX;
		}
	}

	memmove(generator_frame->suspended_con_stack, con_stack_atom->stack + con_stack_atom->cfp - sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type), suspended_con_stack_size);
	con_stack_atom->stackp -= suspended_con_stack_size;

	// At this point the Con_Stack looks like:
	//   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>, ...]
	
	generator_frame->suspended_gfp = con_stack_atom->gfp;
	generator_frame->suspended_ffp = con_stack_atom->ffp;
	generator_frame->suspended_xfp = con_stack_atom->xfp;
	generator_frame->suspended_cfp = con_stack_atom->cfp;
	
	con_stack_atom->gfp = continuation_frame->prev_gfp;
	con_stack_atom->ffp = continuation_frame->prev_ffp;
	con_stack_atom->xfp = continuation_frame->prev_xfp;
	con_stack_atom->cfp = continuation_frame->prev_cfp;

	assert(con_stack_atom->cfp > -1);
	generator_frame->resumption_pc = continuation_frame->resumption_pc;

	// At this point the Con_Stack looks like:
	//   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>]

	Con_Int gen_objs_start = -1;
	if (generator_frame->prev_gfp > gen_objs_start)
		gen_objs_start = generator_frame->prev_gfp;
	if (con_stack_atom->cfp > gen_objs_start)
		gen_objs_start = con_stack_atom->cfp;
	if (con_stack_atom->ffp > gen_objs_start)
		gen_objs_start = con_stack_atom->ffp;

	Con_Int gen_objs_size = con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type) - gen_objs_start;
	ENSURE_ROOM(gen_objs_size + generator_frame->suspended_con_stack_size);

	generator_frame = (Con_Builtins_Con_Stack_Class_Generator_Frame *) (con_stack_atom->stack + con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));
	
	// Copy the gen objs.

	memmove(con_stack_atom->stack + con_stack_atom->stackp, con_stack_atom->stack + gen_objs_start, gen_objs_size);
	con_stack_atom->stackp += gen_objs_size;

	// At this point the Con_Stack looks like:
	//   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>, <gen obj 1>, ..., <gen obj n>]
}




//
// Update the current 'normal' generator frames knowledge of the suspended C stack.
//

void Con_Builtins_Con_Stack_Atom_update_generator_frame(Con_Obj *thread, Con_Obj *con_stack, u_char *suspended_c_stack, Con_Int suspended_c_stack_size, sigjmp_buf suspended_env)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->gfp > -1);
	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->gfp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_GENERATOR_FRAME);
	Con_Builtins_Con_Stack_Class_Generator_Frame *generator_frame = (Con_Builtins_Con_Stack_Class_Generator_Frame *) (con_stack_atom->stack + con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

	generator_frame->suspended_c_stack = suspended_c_stack;
	generator_frame->suspended_c_stack_size = suspended_c_stack_size;
	memmove(generator_frame->suspended_env, suspended_env, sizeof(sigjmp_buf));
}



//
// Restore the suspended Con_Stack so that the most recent 'normal' generator can be pumped for more
// values.
//
// 'c_stack_start', 'suspended_c_stack', 'suspended_c_stack_size', and 'suspended_env' must all be
// pointers to variables that will be set to the value of the appropriate attributes in the
// generator frame and continuation frame.
//

void Con_Builtins_Con_Stack_Atom_prepare_generator_frame_reexecution(Con_Obj *thread, Con_Obj *con_stack, u_char **c_stack_start, u_char **suspended_c_stack, Con_Int *suspended_c_stack_size, sigjmp_buf *suspended_env)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->gfp > -1);
	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->gfp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_GENERATOR_FRAME);
	Con_Builtins_Con_Stack_Class_Generator_Frame *generator_frame = (Con_Builtins_Con_Stack_Class_Generator_Frame *) (con_stack_atom->stack + con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

	// At this point, the stack looks like:
	//   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>, <gen obj 1>, ..., <gen obj n>]
	
	con_stack_atom->stackp = con_stack_atom->gfp;

	// At this point, the stack looks like:
	//   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>]

	// Copy the continuation frame.
	
	memmove(con_stack_atom->stack + con_stack_atom->stackp, generator_frame->suspended_con_stack, generator_frame->suspended_con_stack_size);
	//con_stack_atom->cfp = con_stack_atom->stackp + sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type);
	//assert(con_stack_atom->cfp == generator_frame->suspended_cfp);
	con_stack_atom->gfp = generator_frame->suspended_gfp;
	con_stack_atom->ffp = generator_frame->suspended_ffp;
	con_stack_atom->xfp = generator_frame->suspended_xfp;
	con_stack_atom->cfp = generator_frame->suspended_cfp;
	con_stack_atom->stackp += generator_frame->suspended_con_stack_size;

	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->cfp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_CONTINUATION_FRAME);
	Con_Builtins_Con_Stack_Class_Continuation_Frame *continuation_frame = (Con_Builtins_Con_Stack_Class_Continuation_Frame *) (con_stack_atom->stack + con_stack_atom->cfp - sizeof(Con_Builtins_Con_Stack_Class_Continuation_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

	// At this point, the stack looks like:
	//   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>, <continuation frame>, ...]

	*suspended_c_stack = generator_frame->suspended_c_stack;
	*suspended_c_stack_size = generator_frame->suspended_c_stack_size;
	memmove(suspended_env, generator_frame->suspended_env, sizeof(sigjmp_buf));
	*c_stack_start = continuation_frame->c_stack_start;
}



//
// Add an 'eyield' generator frame which if resumed will start from 'resumption_pc'.
//

void Con_Builtins_Con_stack_Atom_add_generator_eyield_frame(Con_Obj *thread, Con_Obj *con_stack, Con_PC resumption_pc)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	// At this point the Con_Stack looks like:
	//   [..., <gen obj 1>, ..., <gen obj n>]

	ENSURE_ROOM(sizeof(Con_Builtins_Con_Stack_Class_Generator_EYield_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type));
		
	Con_Builtins_Con_Stack_Class_Generator_EYield_Frame *generator_frame = (Con_Builtins_Con_Stack_Class_Generator_EYield_Frame *) (con_stack_atom->stack + con_stack_atom->stackp);
	generator_frame->prev_gfp = con_stack_atom->gfp;
	generator_frame->resumption_pc = resumption_pc;
	
	*((Con_Builtins_Con_Stack_Class_Type *) (generator_frame + 1)) = CON_BUILTINS_CON_STACK_CLASS_GENERATOR_EYIELD_FRAME;
	con_stack_atom->stackp += sizeof(Con_Builtins_Con_Stack_Class_Generator_EYield_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type);
	con_stack_atom->gfp = con_stack_atom->stackp;

	// At this point the Con_Stack looks like:
	//   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>]

	Con_Int gen_objs_start = -1;
	if (generator_frame->prev_gfp > gen_objs_start)
		gen_objs_start = generator_frame->prev_gfp;
	if (con_stack_atom->cfp > gen_objs_start)
		gen_objs_start = con_stack_atom->cfp;
	if (con_stack_atom->ffp > gen_objs_start)
		gen_objs_start = con_stack_atom->ffp;
	
	Con_Int gen_objs_size = con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_EYield_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type) - gen_objs_start;
	ENSURE_ROOM(gen_objs_size);
	
	// Copy the gen objs.

	memmove(con_stack_atom->stack + con_stack_atom->stackp, con_stack_atom->stack + gen_objs_start, gen_objs_size);
	con_stack_atom->stackp += gen_objs_size;

	// At this point the Con_Stack looks like:
	//   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>, <gen obj 1>, ..., <gen obj n>]
}



//
// Read information about the current generator frame ('normal' or 'eyield').
// 'is_eyield_generator_frame' will be set to true if the current generator is an 'eyield' gnerator
// frame. 'resumption_pc' will be set to the PC that the generator expects to resume from.
//

void Con_Builtins_Con_Stack_Atom_read_generator_frame(Con_Obj *thread, Con_Obj *con_stack, bool *is_eyield_generator_frame, Con_PC *resumption_pc)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->gfp > -1);
	Con_Builtins_Con_Stack_Class_Type type = *(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->gfp)) - 1);
	assert((type == CON_BUILTINS_CON_STACK_CLASS_GENERATOR_FRAME) || (type == CON_BUILTINS_CON_STACK_CLASS_GENERATOR_EYIELD_FRAME));
	
	if (type == CON_BUILTINS_CON_STACK_CLASS_GENERATOR_FRAME) {
		Con_Builtins_Con_Stack_Class_Generator_Frame *generator_frame = (Con_Builtins_Con_Stack_Class_Generator_Frame *) (con_stack_atom->stack + con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

		*is_eyield_generator_frame = false;
		*resumption_pc = generator_frame->resumption_pc;
	}
	else {
		// EYield generator frame
		Con_Builtins_Con_Stack_Class_Generator_EYield_Frame *generator_frame = (Con_Builtins_Con_Stack_Class_Generator_EYield_Frame *) (con_stack_atom->stack + con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_EYield_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));
		
		*is_eyield_generator_frame = true;
		*resumption_pc = generator_frame->resumption_pc;
	}
}



//
// Mark the current generator frames' continuation as having returned i.e. it can not be pumped for
// any further values.
//

void Con_Builtins_Con_Stack_Atom_set_generator_returned(Con_Obj *thread, Con_Obj *con_stack)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->gfp > -1);
	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->gfp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_GENERATOR_FRAME);
	Con_Builtins_Con_Stack_Class_Generator_Frame *generator_frame = (Con_Builtins_Con_Stack_Class_Generator_Frame *) (con_stack_atom->stack + con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));
	
	generator_frame->returned = true;
}



//
// Returns true if the current generator frames' continuation is marked as having returned i.e. it
// can not be pumped for any further values.
//

bool Con_Builtins_Con_Stack_Atom_has_generator_returned(Con_Obj *thread, Con_Obj *con_stack)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->gfp > -1);
	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->gfp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_GENERATOR_FRAME);
	Con_Builtins_Con_Stack_Class_Generator_Frame *generator_frame = (Con_Builtins_Con_Stack_Class_Generator_Frame *) (con_stack_atom->stack + con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));
	
	return generator_frame->returned;
}



//
// Remove the most recent generator frame (normal or eyield) from the stack.
//

void Con_Builtins_Con_Stack_Atom_remove_generator_frame(Con_Obj *thread, Con_Obj *con_stack)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->gfp > -1);
	
	Con_Builtins_Con_Stack_Class_Type type = *(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->gfp)) - 1);
	assert((type == CON_BUILTINS_CON_STACK_CLASS_GENERATOR_FRAME) || (type == CON_BUILTINS_CON_STACK_CLASS_GENERATOR_EYIELD_FRAME));
	
	if (type == CON_BUILTINS_CON_STACK_CLASS_GENERATOR_FRAME) {
		Con_Builtins_Con_Stack_Class_Generator_Frame *generator_frame = (Con_Builtins_Con_Stack_Class_Generator_Frame *) (con_stack_atom->stack + con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

		if (generator_frame->suspended_c_stack != NULL) {
			Con_Builtins_Con_Stack_Class_Continuation_Frame *continuation_frame = (Con_Builtins_Con_Stack_Class_Continuation_Frame *) generator_frame->suspended_con_stack;
			con_stack_atom->cfp = continuation_frame->prev_cfp;
			con_stack_atom->ffp = continuation_frame->prev_ffp;
			con_stack_atom->xfp = continuation_frame->prev_xfp;
		}

		con_stack_atom->stackp = con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type);
		con_stack_atom->gfp = generator_frame->prev_gfp;
	}
	else {
		// EYield generator frame
		Con_Builtins_Con_Stack_Class_Generator_EYield_Frame *generator_frame = (Con_Builtins_Con_Stack_Class_Generator_EYield_Frame *) (con_stack_atom->stack + con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_EYield_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

		con_stack_atom->stackp = con_stack_atom->gfp - sizeof(Con_Builtins_Con_Stack_Class_Generator_EYield_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type);

		con_stack_atom->gfp = generator_frame->prev_gfp;
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Failure frames
//

//
// Add a failure frame, which will fail to 'fail_to_pc'.
//

void Con_Builtins_Con_Stack_Atom_add_failure_frame(Con_Obj *thread, Con_Obj *con_stack, Con_PC fail_to_pc)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	ENSURE_ROOM(sizeof(Con_Builtins_Con_Stack_Class_Failure_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type));

	Con_Builtins_Con_Stack_Class_Failure_Frame *failure_frame = (Con_Builtins_Con_Stack_Class_Failure_Frame *) (con_stack_atom->stack + con_stack_atom->stackp);
	failure_frame->is_fail_up = false;
	failure_frame->prev_gfp = con_stack_atom->gfp;
	failure_frame->prev_ffp = con_stack_atom->ffp;
	failure_frame->fail_to_pc = fail_to_pc;

	con_stack_atom->stackp += sizeof(Con_Builtins_Con_Stack_Class_Failure_Frame);
	*((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->stackp)) = CON_BUILTINS_CON_STACK_CLASS_FAILURE_FRAME;
	con_stack_atom->stackp += sizeof(Con_Builtins_Con_Stack_Class_Type);
	con_stack_atom->gfp = -1;
	con_stack_atom->ffp = con_stack_atom->stackp;
}



//
// Add a failure "fail up" frame.
//
//

void Con_Builtins_Con_Stack_Atom_add_failure_fail_up_frame(Con_Obj *thread, Con_Obj *con_stack)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	ENSURE_ROOM(sizeof(Con_Builtins_Con_Stack_Class_Failure_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type));

	Con_Builtins_Con_Stack_Class_Failure_Frame *failure_frame = (Con_Builtins_Con_Stack_Class_Failure_Frame *) (con_stack_atom->stack + con_stack_atom->stackp);
	failure_frame->is_fail_up = true;
	failure_frame->prev_gfp = con_stack_atom->gfp;
	failure_frame->prev_ffp = con_stack_atom->ffp;

	con_stack_atom->stackp += sizeof(Con_Builtins_Con_Stack_Class_Failure_Frame);
	*((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->stackp)) = CON_BUILTINS_CON_STACK_CLASS_FAILURE_FRAME;
	con_stack_atom->stackp += sizeof(Con_Builtins_Con_Stack_Class_Type);
	con_stack_atom->gfp = -1;
	con_stack_atom->ffp = con_stack_atom->stackp;
}



//
// Read information about the current fail up frame. 'is_fail_up' will be set to true if the current
// failure frame is a fail up failure frame. 'has_resumable_generator' will be set to true if there
// is a generator which it may be possible to pump for further values. If the current failure frame
// is not a fail up failure frame, 'fail_to_pc' will be set to the PC which should be jumped to in
// the event of failure.
//
// Note that if any of 'is_fail_up', 'has_resumable_generator', or 'fail_to_pc' is set to NULL then
// that value will not be written to.
//

void Con_Builtins_Con_Stack_Atom_read_failure_frame(Con_Obj *thread, Con_Obj *con_stack, bool *is_fail_up, bool *has_resumable_generator, Con_PC *fail_to_pc)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	if (con_stack_atom->ffp == -1)
		CON_RAISE_EXCEPTION("Parameters_Exception", CON_NEW_STRING("Not enough parameters."));
	assert(con_stack_atom->ffp > -1);
	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->ffp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_FAILURE_FRAME);
	Con_Builtins_Con_Stack_Class_Failure_Frame *failure_frame = (Con_Builtins_Con_Stack_Class_Failure_Frame *) (con_stack_atom->stack + con_stack_atom->ffp - sizeof(Con_Builtins_Con_Stack_Class_Failure_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

	if (is_fail_up != NULL)
		*is_fail_up = failure_frame->is_fail_up;
	if (failure_frame->is_fail_up) {
		if (has_resumable_generator != NULL) {
			if (con_stack_atom->gfp > -1)
				*has_resumable_generator = true;
			else
				*has_resumable_generator = false;
		}
	}
	else {
		if (fail_to_pc != NULL)
			*fail_to_pc = failure_frame->fail_to_pc;
	}
}



//
// Remove the most recent failure frame.
//

void Con_Builtins_Con_Stack_Atom_remove_failure_frame(Con_Obj *thread, Con_Obj *con_stack)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->ffp > -1);
	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->ffp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_FAILURE_FRAME);
	Con_Builtins_Con_Stack_Class_Failure_Frame *failure_frame = (Con_Builtins_Con_Stack_Class_Failure_Frame *) (con_stack_atom->stack + con_stack_atom->ffp - sizeof(Con_Builtins_Con_Stack_Class_Failure_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

	con_stack_atom->stackp = con_stack_atom->ffp - sizeof(Con_Builtins_Con_Stack_Class_Failure_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type);
	con_stack_atom->ffp = failure_frame->prev_ffp;
	con_stack_atom->gfp = failure_frame->prev_gfp;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Exception frames
//

//
// Add a failure frame, which will fail to 'fail_to_pc'.
//

void Con_Builtins_Con_Stack_Atom_add_exception_frame(Con_Obj *thread, Con_Obj *con_stack, sigjmp_buf exception_env)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	ENSURE_ROOM(sizeof(Con_Builtins_Con_Stack_Class_Exception_Frame) + sizeof(Con_Builtins_Con_Stack_Class_Type));

	Con_Builtins_Con_Stack_Class_Exception_Frame *exception_frame = (Con_Builtins_Con_Stack_Class_Exception_Frame *) (con_stack_atom->stack + con_stack_atom->stackp);
	memmove(exception_frame->exception_env, exception_env, sizeof(sigjmp_buf));
	exception_frame->prev_ffp = con_stack_atom->ffp;
	exception_frame->prev_gfp = con_stack_atom->gfp;
	exception_frame->prev_xfp = con_stack_atom->xfp;

	con_stack_atom->stackp += sizeof(Con_Builtins_Con_Stack_Class_Exception_Frame);
	*((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->stackp)) = CON_BUILTINS_CON_STACK_CLASS_EXCEPTION_FRAME;
	con_stack_atom->stackp += sizeof(Con_Builtins_Con_Stack_Class_Type);
	con_stack_atom->xfp = con_stack_atom->stackp;
}



//
// Determine whether there an exception frame is in scope; if so, read information about it.
//
// 'has_exception_frame' should be a pointer to a boolean which will be set to true if there is an
// exception frame in scope. If it is set to true, the contents of the longjmp environment neeeded
// to jump to the handler exception handler will be copied into the memory location pointed to by
// 'exception_env'.
//

void Con_Builtins_Con_Stack_Atom_read_exception_frame(Con_Obj *thread, Con_Obj *con_stack, bool *has_exception_frame, sigjmp_buf *exception_env)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	if (con_stack_atom->xfp == -1) {
		*has_exception_frame = false;
		return;
	}

	assert(con_stack_atom->xfp > -1);
	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->xfp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_EXCEPTION_FRAME);
	Con_Builtins_Con_Stack_Class_Exception_Frame *exception_frame = (Con_Builtins_Con_Stack_Class_Exception_Frame *) (con_stack_atom->stack + con_stack_atom->xfp - sizeof(Con_Builtins_Con_Stack_Class_Exception_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

	*has_exception_frame = true;
	memmove(exception_env, exception_frame->exception_env, sizeof(sigjmp_buf));
} 



//
// Remove the most recent failure frame.
//

void Con_Builtins_Con_Stack_Atom_remove_exception_frame(Con_Obj *thread, Con_Obj *con_stack)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(con_stack_atom->xfp > -1);
	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->xfp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_EXCEPTION_FRAME);
	Con_Builtins_Con_Stack_Class_Exception_Frame *exception_frame = (Con_Builtins_Con_Stack_Class_Exception_Frame *) (con_stack_atom->stack + con_stack_atom->xfp - sizeof(Con_Builtins_Con_Stack_Class_Exception_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type));

	con_stack_atom->stackp = con_stack_atom->xfp - sizeof(Con_Builtins_Con_Stack_Class_Exception_Frame) - sizeof(Con_Builtins_Con_Stack_Class_Type);
	con_stack_atom->ffp = exception_frame->prev_ffp;
	con_stack_atom->gfp = exception_frame->prev_gfp;
	con_stack_atom->xfp = exception_frame->prev_xfp;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Pushing / pulling objects
//

//
// Insert 'obj' onto the stack 'num_objs_to_skip' objects back into the stack.
//

void Con_Builtins_Con_Stack_Atom_push_n_object(Con_Obj *thread, Con_Obj *con_stack, Con_Obj *obj, Con_Int num_objs_to_skip)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	ENSURE_ROOM(sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type));

#	ifndef NDEBUG
	Con_Int stackp = con_stack_atom->stackp;
	for (int i = 0; i < num_objs_to_skip; i += 1) {
		assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + stackp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_OBJECT);
		stackp -= sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
	}
	assert(stackp >= 0);
#	endif

	Con_Int obj_start = con_stack_atom->stackp - (sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type)) * num_objs_to_skip;

	memmove(con_stack_atom->stack + obj_start + sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type), con_stack_atom->stack + obj_start, (sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type)) * num_objs_to_skip);

	*(Con_Obj **) (con_stack_atom->stack + obj_start) = obj;
	*((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + obj_start + sizeof(Con_Obj *))) = CON_BUILTINS_CON_STACK_CLASS_OBJECT;
	con_stack_atom->stackp += sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
}



//
// Push 'obj' onto the end of the stack.
//

void Con_Builtins_Con_Stack_Atom_push_object(Con_Obj *thread, Con_Obj *con_stack, Con_Obj *obj)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	ENSURE_ROOM(sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type));

	*(Con_Obj **) (con_stack_atom->stack + con_stack_atom->stackp) = obj;
	*((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->stackp + sizeof(Con_Obj *))) = CON_BUILTINS_CON_STACK_CLASS_OBJECT;
	con_stack_atom->stackp += sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
}



//
// Returns and removes the element 'num_objs_to_skip' objects back in the stack, which must itself
// be an object.
//

Con_Obj *Con_Builtins_Con_Stack_Atom_pop_n_object(Con_Obj *thread, Con_Obj *con_stack, Con_Int num_objs_to_skip)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	Con_Int obj_stackp = con_stack_atom->stackp;
	for (int i = 0; i < num_objs_to_skip; i += 1) {
		Con_Builtins_Con_Stack_Class_Type type = *(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + obj_stackp)) - 1);

		assert((type == CON_BUILTINS_CON_STACK_CLASS_OBJECT) || (type == CON_BUILTINS_CON_STACK_SLOT_LOOKUP_APPLY));
		if (type == CON_BUILTINS_CON_STACK_CLASS_OBJECT)
			obj_stackp -= sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
		else
			obj_stackp -= sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply);
	}
	assert(obj_stackp >= 0);

	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + obj_stackp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_OBJECT);
	
	Con_Obj *obj = *(Con_Obj **) (con_stack_atom->stack + obj_stackp - sizeof(Con_Builtins_Con_Stack_Class_Type) - sizeof(Con_Obj *));
	
	memmove(con_stack_atom->stack + obj_stackp - sizeof(Con_Builtins_Con_Stack_Class_Type) - sizeof(Con_Obj *), con_stack_atom->stack + obj_stackp, con_stack_atom->stackp - obj_stackp);
	con_stack_atom->stackp -= sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
	
	return obj;
}



//
// Returns the last element of a stack, which must be an object, and then removes it from the stack.
//

Con_Obj *Con_Builtins_Con_Stack_Atom_pop_object(Con_Obj *thread, Con_Obj *con_stack)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->stackp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_OBJECT);
	
	Con_Obj *obj = *(Con_Obj **) (con_stack_atom->stack + con_stack_atom->stackp - sizeof(Con_Builtins_Con_Stack_Class_Type) - sizeof(Con_Obj *));
	
	con_stack_atom->stackp -= sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
	
	return obj;
}



//
// Returns the last element of a stack, which must be an object, but does not pop it off the stack.
//

Con_Obj *Con_Builtins_Con_Stack_Atom_peek_object(Con_Obj *thread, Con_Obj *con_stack)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->stackp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_OBJECT);
	
	Con_Obj *obj = *(Con_Obj **) (con_stack_atom->stack + con_stack_atom->stackp - sizeof(Con_Builtins_Con_Stack_Class_Type) - sizeof(Con_Obj *));
	
	return obj;
}



//
// Push 'self'.'func' onto the stack.
//

void Con_Builtins_Con_Stack_Atom_push_slot_lookup_apply(Con_Obj *thread, Con_Obj *con_stack, Con_Obj *self, Con_Obj *func)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	ENSURE_ROOM(sizeof(Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply) + sizeof(Con_Builtins_Con_Stack_Class_Type));

	Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply *slot_lookup_apply = (Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply *) (con_stack_atom->stack + con_stack_atom->stackp);
	slot_lookup_apply->self = self;
	slot_lookup_apply->func = func;

	*((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->stackp + sizeof(Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply))) = CON_BUILTINS_CON_STACK_SLOT_LOOKUP_APPLY;
	con_stack_atom->stackp += sizeof(Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply) + sizeof(Con_Builtins_Con_Stack_Class_Type);
}



//
// Pop either a function or a slot_lookup_apply from the stack; return false for the former and true
// for the latter. In the latter case, the self obj is automatically placed at the correct point in
// the stack and the number of arguments that 'func' is considered to be passed should be increased
// by one.
//

bool Con_Builtins_Con_Stack_Atom_pop_n_object_or_slot_lookup_apply(Con_Obj *thread, Con_Obj *con_stack, Con_Int num_objs_to_skip, Con_Obj **func)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

#	ifndef NDEBUG
	Con_Int stackp = con_stack_atom->stackp;
	for (int i = 0; i < num_objs_to_skip; i += 1) {
		assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + stackp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_OBJECT);
		stackp -= sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
	}
	assert(stackp >= 0);
#	endif

	Con_Int obj_stackp = con_stack_atom->stackp - (sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type)) * num_objs_to_skip;

	Con_Builtins_Con_Stack_Class_Type type = *(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + obj_stackp)) - 1);

	assert((type == CON_BUILTINS_CON_STACK_CLASS_OBJECT) || (type == CON_BUILTINS_CON_STACK_SLOT_LOOKUP_APPLY));
	
	if (type == CON_BUILTINS_CON_STACK_CLASS_OBJECT) {
		*func = *(Con_Obj **) (con_stack_atom->stack + obj_stackp - sizeof(Con_Builtins_Con_Stack_Class_Type) - sizeof(Con_Obj *));
		
		memmove(con_stack_atom->stack + obj_stackp - sizeof(Con_Builtins_Con_Stack_Class_Type) - sizeof(Con_Obj *), con_stack_atom->stack + obj_stackp, con_stack_atom->stackp - obj_stackp);
		con_stack_atom->stackp -= sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
		
		return false;
	}
	else {
		Con_Int slot_lookup_stackp = obj_stackp - sizeof(Con_Builtins_Con_Stack_Class_Type) - sizeof(Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply);
	
		Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply *slot_lookup_apply = (Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply *) (con_stack_atom->stack + slot_lookup_stackp);
		Con_Obj *self = slot_lookup_apply->self;
		*func = slot_lookup_apply->func;
		
		*((Con_Obj **) (con_stack_atom->stack + slot_lookup_stackp)) = self;
		*((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + slot_lookup_stackp + sizeof(Con_Obj *))) = CON_BUILTINS_CON_STACK_CLASS_OBJECT;
		
		Con_Int shift_stackp = obj_stackp - (sizeof(Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply) - sizeof(Con_Obj *));
		memmove(con_stack_atom->stack + shift_stackp, con_stack_atom->stack + obj_stackp, con_stack_atom->stackp - shift_stackp);
		con_stack_atom->stackp -= sizeof(Con_Builtins_Con_Stack_Class_Slot_Lookup_Apply) - sizeof(Con_Obj *);
		
		return true;
	}
}



//
// Swap the last two elements on the stack, both of which must be objects.
//

void Con_Builtins_Con_Stack_Atom_swap(Con_Obj *thread, Con_Obj *con_stack)
{
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->stackp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_OBJECT);
	
	Con_Obj **obj_firstp = (Con_Obj **) (con_stack_atom->stack + con_stack_atom->stackp - sizeof(Con_Builtins_Con_Stack_Class_Type) - sizeof(Con_Obj *));

	assert(*((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + con_stack_atom->stackp - sizeof(Con_Builtins_Con_Stack_Class_Type) - sizeof(Con_Obj *) - sizeof(Con_Builtins_Con_Stack_Class_Type))) == CON_BUILTINS_CON_STACK_CLASS_OBJECT);

	Con_Obj **obj_secondp = (Con_Obj **) (con_stack_atom->stack + con_stack_atom->stackp - sizeof(Con_Builtins_Con_Stack_Class_Type) - sizeof(Con_Obj *) - sizeof(Con_Builtins_Con_Stack_Class_Type) - sizeof(Con_Obj *));
	
	Con_Obj *tmp = *obj_firstp;
	*obj_firstp = *obj_secondp;
	*obj_secondp = tmp;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Function arguments
//

//
// Unpack a funtions arguments.
//
// This should generally be called via the CON_UNPACK_ARGS macro.
//

void Con_Builtins_Con_Stack_Atom_unpack_args(Con_Obj *thread, Con_Obj *con_stack, const char *args, ...)
{
	Con_Builtins_Con_Stack_Atom *con_stack_atom = CON_GET_ATOM(con_stack, CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&con_stack->mutex);
	Con_Int num_args = Con_Numbers_Number_to_Con_Int(thread, Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack));
	
#	ifndef NDEBUG
	Con_Int debug_stackp = con_stack_atom->stackp;
	for (int i = 0; i < num_args; i += 1) {
		assert(*(((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + debug_stackp)) - 1) == CON_BUILTINS_CON_STACK_CLASS_OBJECT);
		debug_stackp -= sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
	}
#	endif

	Con_Int stackp = con_stack_atom->stackp - (sizeof(Con_Builtins_Con_Stack_Class_Type) + sizeof(Con_Obj *)) * num_args;

	va_list ap;
	va_start(ap, args);
	Con_Int args_processed = 0;
	bool processing_optional_args = false;
	while (*args) {
		if (*args == 'v') {
			assert(*(args + 1) == '\0');
			CON_MUTEX_UNLOCK(&con_stack->mutex);
			Con_Obj *var_args = Con_Builtins_List_Atom_new_sized(thread, num_args - args_processed);
			for (int i = args_processed; i < num_args; i += 1) {
				CON_MUTEX_LOCK(&con_stack->mutex);
				Con_Obj *obj = *((Con_Obj **) (con_stack_atom->stack + stackp));
				stackp += sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
				CON_MUTEX_UNLOCK(&con_stack->mutex);
				CON_GET_SLOT_APPLY(var_args, "append", obj);
				args_processed += 1;
			}
			*va_arg(ap, Con_Obj **) = var_args;
			CON_MUTEX_LOCK(&con_stack->mutex);
			break;
		}
		else if (*args == ';') {
			processing_optional_args = true;
			args += 1;
			continue;
		}
		
		if (args_processed == num_args) {
			if (processing_optional_args) {
				while (*args) {
					*va_arg(ap, Con_Obj **) = NULL;
					args += 1;
				}
				break;
			}
			else
				CON_RAISE_EXCEPTION("Parameters_Exception", CON_NEW_STRING("Not enough parameters."));
		}
		
		assert(*((Con_Builtins_Con_Stack_Class_Type *) (con_stack_atom->stack + stackp + sizeof(Con_Obj *))) == CON_BUILTINS_CON_STACK_CLASS_OBJECT);
		Con_Obj *obj = *((Con_Obj **) (con_stack_atom->stack + stackp));
		stackp += sizeof(Con_Obj *) + sizeof(Con_Builtins_Con_Stack_Class_Type);
		
		if (*args == '.') {
			va_arg(ap, Con_Obj **);
			args_processed += 1;
			args += 1;
			continue;
		}
		
		if (*args == 'u' || *args == 'U') {
			if (!((*args == 'u') && ((obj == NULL) || (obj == CON_BUILTIN(CON_BUILTIN_NULL_OBJ))))) {
				Con_Obj *atom_type = va_arg(ap, Con_Obj *);
				if (CON_FIND_ATOM(obj, atom_type) == NULL)
					CON_XXX;
			}
			else
				va_arg(ap, Con_Obj *);
			*va_arg(ap, Con_Obj **) = obj;
			args_processed += 1;
			args += 1;
			continue;
		}
		
		*va_arg(ap, Con_Obj **) = obj;

#		define CONFORMS_TO(atom, class) if (CON_FIND_ATOM(obj, CON_BUILTIN(atom)) == NULL) {\
			Con_Obj *msg = CON_ADD(CON_NEW_STRING("arg "), CON_GET_SLOT_APPLY(CON_NEW_INT(args_processed + 1), "to_str")); \
			CON_RAISE_EXCEPTION("Type_Exception", CON_BUILTIN(class), obj, msg); \
		}

		char arg = *args;
		if (arg >= 'a' && arg <= 'z') {
			arg += 'A' - 'a';
			if ((obj == NULL) || (obj == CON_BUILTIN(CON_BUILTIN_NULL_OBJ))) {
				args_processed += 1;
				args += 1;
				continue;
			}
		}

		switch (arg) {
			case 'O':
				// By definition every object is considered conformant to args type 'o'.
				args_processed += 1;
				args += 1;
				break;
			case 'N':
				CONFORMS_TO(CON_BUILTIN_INT_ATOM_DEF_OBJECT, CON_BUILTIN_INT_CLASS)
				args_processed += 1;
				args += 1;
				break;
			case 'I':
				CONFORMS_TO(CON_BUILTIN_INT_ATOM_DEF_OBJECT, CON_BUILTIN_INT_CLASS)
				args_processed += 1;
				args += 1;
				break;
			case 'S':
				CONFORMS_TO(CON_BUILTIN_STRING_ATOM_DEF_OBJECT, CON_BUILTIN_STRING_CLASS)
				args_processed += 1;
				args += 1;
				break;
			case 'L': 
				CONFORMS_TO(CON_BUILTIN_LIST_ATOM_DEF_OBJECT, CON_BUILTIN_LIST_CLASS)
				args_processed += 1;
				args += 1;
				break;
			case 'D':
				CONFORMS_TO(CON_BUILTIN_DICT_ATOM_DEF_OBJECT, CON_BUILTIN_DICT_CLASS)
				args_processed += 1;
				args += 1;
				break;
			case 'M':
				CONFORMS_TO(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT, CON_BUILTIN_MODULE_CLASS)
				args_processed += 1;
				args += 1;
				break;
			case 'T':
				CONFORMS_TO(CON_BUILTIN_THREAD_ATOM_DEF_OBJECT, CON_BUILTIN_THREAD_CLASS)
				args_processed += 1;
				args += 1;
				break;
			case 'F':
				CONFORMS_TO(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT, CON_BUILTIN_FUNC_CLASS)
				args_processed += 1;
				args += 1;
				break;
			case 'W':
				CONFORMS_TO(CON_BUILTIN_SET_ATOM_DEF_OBJECT, CON_BUILTIN_CLASS_CLASS)
				args_processed += 1;
				args += 1;
				break;
			case 'E':
				CONFORMS_TO(CON_BUILTIN_EXCEPTION_ATOM_DEF_OBJECT, CON_BUILTIN_EXCEPTION_CLASS)
				args_processed += 1;
				args += 1;
				break;
			case 'C':
				CONFORMS_TO(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT, CON_BUILTIN_CLASS_CLASS)
				args_processed += 1;
				args += 1;
				break;
			default:
				printf("%c\n", *args);
				CON_XXX;
		}
	}
	va_end(ap);
	
	if (args_processed < num_args) {
		CON_MUTEX_UNLOCK(&con_stack->mutex);
		printf("%d %d %p\n", args_processed, num_args, CON_GET_SLOT(CON_BUILTIN(CON_BUILTIN_EXCEPTIONS_MODULE), "Parameters_Exception"));
		CON_RAISE_EXCEPTION("Parameters_Exception", CON_NEW_STRING("Too many parameters."));
	}
	
	con_stack_atom->stackp -= (sizeof(Con_Builtins_Con_Stack_Class_Type) + sizeof(Con_Obj *)) * num_args;
	CON_MUTEX_UNLOCK(&con_stack->mutex);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Con_Stack_Class_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Con_Stack_Atom *con_stack_atom = (Con_Builtins_Con_Stack_Atom *) atom;

	Con_Memory_gc_push(thread, con_stack_atom->stack);
	// Conservative scanning is pure laziness, but is easy.
	Con_Memory_gc_scan_conservative(thread, con_stack_atom->stack, con_stack_atom->stackp);
}
