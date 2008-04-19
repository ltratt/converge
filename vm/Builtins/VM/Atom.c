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

#include <signal.h>
#include <stdarg.h>
#include <string.h>

#if HAVE_ALLOCA_H
#	include <alloca.h>
#elif defined __GNUC__
#	define alloca __builtin_alloca
#elif defined _AIX
#	define alloca __alloca
#elif defined _MSC_VER
#	include <malloc.h>
#	define alloca _alloca
#else
#	include <stddef.h>
#	ifdef  __cplusplus
extern "C"
#	endif
void *alloca (size_t);
#endif

#if CON_USE_UCONTEXT_H
#include <ucontext.h>
#endif

#include "Arch.h"
#include "Core.h"
#include "Modules.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Target.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Closure/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Dict/Atom.h"
#include "Builtins/Exception/Atom.h"
#include "Builtins/Exception/Class.h"
#include "Builtins/Float/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Partial_Application/Atom.h"
#include "Builtins/Partial_Application/Class.h"
#include "Builtins/Set/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_VM_Atom_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);

Con_Obj *_Con_Builtins_VM_Atom_apply(Con_Obj *, Con_Obj *, Con_Int, Con_Obj *, bool);
#if CON_USE_UCONTEXT_H
void _Con_Builtins_VM_Atom_apply_pump_restore_c_stack(Con_Obj *, u_char *, u_char *, size_t, ucontext_t, int);
#else
void _Con_Builtins_VM_Atom_apply_pump_restore_c_stack(Con_Obj *, u_char *, u_char *, size_t, JMP_BUF, int);
#endif

Con_Obj *_Con_Builtins_VM_Atom_execute(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_VM_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *vm_atom_def = CON_BUILTIN(CON_BUILTIN_VM_ATOM_DEF_OBJECT);
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) vm_atom_def->first_atom;
	atom_def_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_VM_Atom_gc_scan_func, NULL);
	
	Con_Memory_change_chunk_type(thread, vm_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// VM creation
//

void _Con_Builtins_VM_Atom_signal_trap(int);

Con_Obj *Con_Builtins_VM_Atom_new(Con_Obj *thread, Con_Memory_Store *mem_store, int argc, char **argv, char *vm_path, char *prog_path)
{
	// It is expected that the first atom of a VM object is a Con_Builtins_VM_Atom.

	Con_Obj *vm = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_VM_Atom) + sizeof(Con_Builtins_Slots_Atom), NULL);
	Con_Builtins_VM_Atom *vm_atom = (Con_Builtins_VM_Atom *) vm->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (vm_atom + 1);
	
	vm_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	vm_atom->atom_type = CON_BUILTIN(CON_BUILTIN_VM_ATOM_DEF_OBJECT);
	vm_atom->state = CON_VM_RUNNING;
	vm_atom->mem_store = mem_store;
	vm_atom->argc = argc;
	vm_atom->argv = argv;
	vm_atom->vm_path = vm_path;
	vm_atom->prog_path = prog_path;
	vm_atom->current_exception = NULL;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, vm, CON_MEMORY_CHUNK_OBJ);
	
	CON_SET_SLOT(vm, "modules", Con_Builtins_Dict_Atom_new(thread));
	
	// XXX: this is ropey, but it's not clear where best to put it.
	
	signal(SIGINT, _Con_Builtins_VM_Atom_signal_trap);
	
	return vm;
}



//

bool _Con_Builtins_VM_Atom_signal_trap_sigint = false;

void _Con_Builtins_VM_Atom_signal_trap(int x)
{
	_Con_Builtins_VM_Atom_signal_trap_sigint = true;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//

//
// Return the VM's Con_Memory_Store.
//

Con_Memory_Store *Con_Builtins_VM_Atom_get_mem_store(Con_Obj *thread)
{
	return ((Con_Builtins_VM_Atom *) ((Con_Builtins_Thread_Atom *) thread->first_atom)->vm->first_atom)->mem_store;
}



//
//
//

void Con_Builtins_VM_Atom_read_prog_args(Con_Obj *thread, int *argc, char ***argv)
{
	Con_Builtins_VM_Atom *vm_atom = (Con_Builtins_VM_Atom *) ((Con_Builtins_Thread_Atom *) thread->first_atom)->vm->first_atom;
	
	*argc = vm_atom->argc;
	*argv = vm_atom->argv;
}



void Con_Builtins_VM_Atom_get_paths(Con_Obj *thread, char **vm_path, char **prog_path)
{
	Con_Builtins_VM_Atom *vm_atom = (Con_Builtins_VM_Atom *) ((Con_Builtins_Thread_Atom *) thread->first_atom)->vm->first_atom;

	*vm_path = vm_atom->vm_path;
	*prog_path = vm_atom->prog_path;
}



//
// Return the VM's current state.
//

Con_Atoms_VM_State Con_Builtins_VM_Atom_get_state(Con_Obj *thread)
{
	return ((Con_Builtins_VM_Atom *) ((Con_Builtins_Thread_Atom *) thread->first_atom)->vm->first_atom)->state;
}


//
// Return builtin object 'builtin_no'.
//

Con_Obj *Con_Builtins_VM_Atom_get_builtin(Con_Obj *thread, Con_Int builtin_no)
{
	return ((Con_Builtins_VM_Atom *) ((Con_Builtins_Thread_Atom *) thread->first_atom)->vm->first_atom)->builtins[builtin_no];
}



//
// Return the module the current 
//

Con_Obj *Con_Builtins_VM_Atom_get_functions_module(Con_Obj *thread)
{
	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);
	
	Con_PC pc;
	Con_Obj *func;
	CON_MUTEX_LOCK(&con_stack->mutex);
	Con_Builtins_Con_Stack_Atom_read_continuation_frame(thread, con_stack, &func, NULL, &pc);
	CON_MUTEX_UNLOCK(&con_stack->mutex);
	assert(func != NULL);
	
	return pc.module;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Function application
//

//
// Normal function application
//

//
// Perform the actual application of arguments to 'func' given a particular closure.
//
// If 'suppress_fail' is true, then if the function fails NULL is returned; otherwise the VM performs
// a failure. When C code is calling a function it is probably generally not expecting failure to
// happen; this function can be a highly useful convenience.
//

Con_Obj *_Con_Builtins_VM_Atom_apply(Con_Obj *thread, Con_Obj *func, Con_Int num_args, Con_Obj *closure, bool suppress_fail)
{
	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);

	Con_Obj *num_args_obj = CON_NEW_INT(num_args);
	CON_MUTEX_LOCK(&con_stack->mutex);
	Con_Builtins_Con_Stack_Atom_add_continuation_frame(thread, con_stack, func, num_args, closure, Con_Builtins_Func_Atom_make_con_pc_null(thread), false);
	Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, num_args_obj);

	Con_Obj *return_obj = _Con_Builtins_VM_Atom_execute(thread);

	if (return_obj == CON_BUILTIN(CON_BUILTIN_FAIL_OBJ)) {
		if (suppress_fail)
			return_obj = NULL;
		else {
			CON_RAISE_EXCEPTION("VM_Exception", CON_NEW_STRING("Function attempting to return fail, but caller can not handle failure."));
		}
	}

	CON_MUTEX_LOCK(&con_stack->mutex);
	Con_Builtins_Con_Stack_Atom_remove_continuation_frame(thread, con_stack);
	CON_MUTEX_UNLOCK(&con_stack->mutex);
		
	return return_obj;
}



//
// Apply var args to 'func'. The object resulting from calling the function is returned.
//
// If 'suppress_fail' is true, then if the function fails NULL is returned; otherwise the VM performs
// a failure. When C code is calling a function it is probably generally not expecting failure to
// happen; this function can be a highly useful convenience.
//

Con_Obj *Con_Builtins_VM_Atom_apply(Con_Obj *thread, Con_Obj *func, bool suppress_fail, ...)
{
	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);

	Con_Int num_args;
	if (CON_FIND_ATOM(func, CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT)) != NULL) {
		num_args = Con_Builtins_Partial_Application_Atom_push_args_onto_stack(thread, func, con_stack, 0);
		func = Con_Builtins_Partial_Application_Atom_get_func(thread, func);
	}
	else
		num_args = 0;
	
	if (CON_FIND_ATOM(func, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT)) == NULL)
		CON_RAISE_EXCEPTION("Apply_Exception", func);

	va_list ap;
	Con_Obj *arg_obj;
	va_start(ap, suppress_fail);
	CON_MUTEX_LOCK(&con_stack->mutex);
	while ((arg_obj = va_arg(ap, Con_Obj *)) != NULL) {
		Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, arg_obj);
		num_args += 1;
	}
	CON_MUTEX_UNLOCK(&con_stack->mutex);
	va_end(ap);
	
	Con_Int num_closure_vars = Con_Builtins_Func_Atom_get_num_closure_vars(thread, func);
	Con_Obj *container_closure = Con_Builtins_Func_Atom_get_container_closure(thread, func);
	Con_Obj *closure = NULL;
	if ((num_closure_vars > 0) || (container_closure != NULL))
		closure = Con_Builtins_Closure_Atom_new(thread, num_closure_vars, container_closure);
		
	return _Con_Builtins_VM_Atom_apply(thread, func, num_args, closure, suppress_fail);
}



//
// Apply var args to 'func' whose application will use the closure 'closure'. The object resulting
// from calling the function is returned.
//
// If 'suppress_fail' is true, then if the function fails NULL is returned; otherwise the VM performs
// a failure. When C code is calling a function it is probably generally not expecting failure to
// happen; this function can be a highly useful convenience.
//
// This function is of limited use, and is probably only needed by Con_Modules_import.
//

Con_Obj *Con_Builtins_VM_Atom_apply_with_closure(Con_Obj *thread, Con_Obj *func, Con_Obj *closure, bool suppress_fail, ...)
{
	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);

	CON_MUTEX_LOCK(&con_stack->mutex);
	va_list ap;
	Con_Obj *arg_obj;
	Con_Int num_args = 0;
	va_start(ap, suppress_fail);
	while ((arg_obj = va_arg(ap, Con_Obj *)) != NULL) {
		Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, arg_obj);
		num_args += 1;
	}
	va_end(ap);
	CON_MUTEX_UNLOCK(&con_stack->mutex);
	
	return _Con_Builtins_VM_Atom_apply(thread, func, num_args, closure, suppress_fail);
}



//
// Apply var args to 'obj'.'slot_name'. The object resulting from calling the function is returned.
//
// If 'suppress_fail' is true, then if the function fails NULL is returned; otherwise the VM performs
// a failure. When C code is calling a function it is probably generally not expecting failure to
// happen; this function can be a highly useful convenience.
//
// If 'obj'.'slot_name' does not evaluate to a function this function will throw a standard "can not
// apply" function.
//
// This function is intended as both a convenience to conflate the common idiom of applying a
// function extracted from an object, and also as a performance measure since it bypasses creating
// Function_Binding objects whenever possible.
//

Con_Obj *Con_Builtins_VM_Atom_get_slot_apply(Con_Obj *thread, Con_Obj *obj, const u_char *slot_name, Con_Int slot_name_size, bool suppress_fail, ...)
{
	bool custom_get_slot;

	Con_Obj *slot_val = Con_Object_get_slot_no_binding(thread, obj, NULL, slot_name, slot_name_size, &custom_get_slot);

	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);
	Con_Int num_args;
	if (custom_get_slot) {
		if (CON_FIND_ATOM(slot_val, CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT)) != NULL) {
			num_args = Con_Builtins_Partial_Application_Atom_push_args_onto_stack(thread, slot_val, con_stack, 0);
			slot_val = Con_Builtins_Partial_Application_Atom_get_func(thread, slot_val);
		}
		else {
			num_args = 0;
		}
		
		CON_MUTEX_LOCK(&con_stack->mutex);
	}
	else {
		if (CON_FIND_ATOM(slot_val, CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT)) != NULL)
			CON_XXX;

		if (!CON_FIND_ATOM(slot_val, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT)))
			CON_XXX;

		if (Con_Builtins_Func_Atom_is_bound(thread, slot_val)) {
			num_args = 1;
			CON_MUTEX_LOCK(&con_stack->mutex);
			Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, obj);
		}
		else {
			num_args = 0;
			CON_MUTEX_LOCK(&con_stack->mutex);
		}
	}
	
	Con_Obj *arg_obj;
	va_list ap;
	va_start(ap, suppress_fail);
	while ((arg_obj = va_arg(ap, Con_Obj *)) != NULL) {
		Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, arg_obj);
		num_args += 1;
	}
	va_end(ap);
	CON_MUTEX_UNLOCK(&con_stack->mutex);

	Con_Int num_closure_vars = Con_Builtins_Func_Atom_get_num_closure_vars(thread, slot_val);
	Con_Obj *container_closure = Con_Builtins_Func_Atom_get_container_closure(thread, slot_val);
	Con_Obj *closure = NULL;
	if ((num_closure_vars > 0) || (container_closure != NULL))
		closure = Con_Builtins_Closure_Atom_new(thread, num_closure_vars, container_closure);
	
	return _Con_Builtins_VM_Atom_apply(thread, slot_val, num_args, closure, suppress_fail);
} 



//
// Function pumping application
//

//
//
//

void Con_Builtins_VM_Atom_pre_apply_pump(Con_Obj *thread, Con_Obj *func, Con_Int num_args, Con_Obj *closure, Con_PC resumption_pc)
{
	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);

	Con_Obj *num_args_obj = CON_NEW_INT(num_args);
	CON_MUTEX_LOCK(&con_stack->mutex);
	Con_Builtins_Con_Stack_Atom_add_continuation_frame(thread, con_stack, func, num_args, closure, resumption_pc, true);
	Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, num_args_obj);
	CON_MUTEX_UNLOCK(&con_stack->mutex);
}



//
// Set up 'obj.slot_name' so that if it is a generator it can be pumped for multiple values by
// calling Con_Builtins_VM_Atom_apply_pump.
//

void Con_Builtins_VM_Atom_pre_get_slot_apply_pump(Con_Obj *thread, Con_Obj *obj, const u_char *slot_name, Con_Int slot_name_size, ...)
{
	bool custom_get_slot;
	Con_Obj *slot_val = Con_Object_get_slot_no_binding(thread, obj, NULL, slot_name, slot_name_size, &custom_get_slot);

	if (!custom_get_slot) {
		if (CON_FIND_ATOM(slot_val, CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT)) != NULL)
			CON_XXX;
	}
	
	if (CON_FIND_ATOM(slot_val, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT)) == NULL)
		CON_XXX;

	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);

	Con_Obj *arg_obj;
	Con_Int num_args = 1;
	CON_MUTEX_LOCK(&con_stack->mutex);
	Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, obj);
	va_list ap;
	va_start(ap, slot_name_size);
	while ((arg_obj = va_arg(ap, Con_Obj *)) != NULL) {
		Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, arg_obj);
		num_args += 1;
	}
	CON_MUTEX_UNLOCK(&con_stack->mutex);
	va_end(ap);

	Con_Int num_closure_vars = Con_Builtins_Func_Atom_get_num_closure_vars(thread, slot_val);
	Con_Obj *container_closure = Con_Builtins_Func_Atom_get_container_closure(thread, slot_val);
	Con_Obj *closure = NULL;
	if ((num_closure_vars > 0) || (container_closure != NULL))
		closure = Con_Builtins_Closure_Atom_new(thread, num_closure_vars, container_closure);
	Con_Builtins_VM_Atom_pre_apply_pump(thread, slot_val, num_args, closure, Con_Builtins_Func_Atom_make_con_pc_null(thread));
}



//
// Restore the C stack at 'suspended_s_stack' of size 'suspended_c_stack_size' to address
// 'c_stack_bottom' and longjmp to 'suspended_env'. 'twice_more' should be set to '2' when calling
// this function.
//

#if CON_USE_UCONTEXT_H
void _Con_Builtins_VM_Atom_apply_pump_restore_c_stack(Con_Obj *thread, u_char *c_stack_current, u_char *suspended_c_stack, size_t suspended_c_stack_size, ucontext_t suspended_env, int twice_more)
#else
void _Con_Builtins_VM_Atom_apply_pump_restore_c_stack(Con_Obj *thread, u_char *c_stack_current, u_char *suspended_c_stack, size_t suspended_c_stack_size, JMP_BUF suspended_env, int twice_more)
#endif
{
	// This function is partly inspired by Dan Piponi's suggestion:
	//
	//   http://homepage.mac.com/sigfpe/Computing/continuations.html
	//
	// and is a very portable way of restoring a suspended C stack. It uses alloca to ensure that
	// enough space is available to restore the suspended stack (if alloca isn't available, it then
	// falls back more directly on Dan's suggestion and recursively calls itself until enough stack
	// space is available). It then calls itself "twice more" to ensure that it can continue
	// executing whilst it actually restores the C stack. It is necessary to recurse two more times
	// (rather than one as in Dan's situation) because the Con_Builtins_Thread_get_stackp returns the
	// base stack address one level down or up (depending on whether the stack grows up or down
	// respectively) and may not leave enough room for this function to execute in when the
	// suspended C stack is put back in place.
	//
	// This all ensures that longjmp doesn't get confused.

	u_char *c_stackp;
	CON_ARCH_GET_STACKP(c_stackp);
#	ifdef CON_C_STACK_GROWS_DOWN
	if (c_stackp > c_stack_current) {
#		ifdef HAVE_ALLOCA
		// Use alloca in order to ensure that there's enough space on the stack to restore the
		// suspended stack.
		
		alloca(c_stackp - c_stack_current);
#		else
		_Con_Builtins_VM_Atom_apply_pump_restore_c_stack(thread, c_stack_current, suspended_c_stack, suspended_c_stack_size, suspended_env, 2);
#       endif
	}
#	else
    CON_XXX;
#	endif
	
	// At this point, we know there is definitely enough space to restore the C stack; however if
	// were to restore the C stack blindly at this point there's a good chance we'd kill the stack
	// as this function is executing, so we recurse two times to make sure there's enough stack
	// space for this function to execute in.
	
	if (twice_more > 0)
		_Con_Builtins_VM_Atom_apply_pump_restore_c_stack(thread, c_stack_current, suspended_c_stack, suspended_c_stack_size, suspended_env, twice_more - 1);
	
	memmove(c_stack_current, suspended_c_stack, suspended_c_stack_size);

#if CON_USE_UCONTEXT_H
	setcontext(&suspended_env);
#else
#	if defined(__CYGWIN__)
	// XXX
	//
	// For reasons that are far from clear to me, the following line:
	//
	//   siglongjmp(suspended_env, 1);
	//
	// causes a compiler warning (with GCC 3.4.4), and repeatedly causes the Cygwin's that I have
	// available to crash. Whether this is a code generation booboo, or whether siglongjmp doesn't
	// like the JMP_BUF to come from the argument stack, I have no idea.
	//
	// Whatever the reason, copying the JMP_BUF into temporary memory shuts up the compiler
	// and causes everything to stop crashing and start working.
	
	JMP_BUF suspended_env_copy;
	memmove(suspended_env_copy, suspended_env, sizeof(JMP_BUF));
	siglongjmp(suspended_env_copy, 1);
#	elif defined(CON_HAVE_SIGSETJMP)
	siglongjmp(suspended_env, 1);
#	else
	longjmp(suspended_env, 1);
#	endif
#endif // CON_USE_UCONTEXT_H
}



//
// Pump the most recent generator for a new value.
//
// If 'remove_finished_generator_frames' is true, this function will attempt to pro-actively remove
// generator frames that are no longer needed. This should only be set to 'true' by
//  _Con_Builtins_VM_Atom_execute.
//
// Returns NULL if the generator has no more values to pump.
//

Con_Obj *Con_Builtins_VM_Atom_apply_pump(Con_Obj *thread, bool remove_finished_generator_frames)
{
	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);

	CON_MUTEX_LOCK(&con_stack->mutex);
	
	// At this point, we're in one of two situations:
	//
	//   1) A generator frame has been added but not yet used. So the con stack looks like:
	//        [..., <generator frame>, <continuation frame>, <argument objects>]
	//
	//   2) We need to resume a previous generator. The con stack looks like:
	//        [..., <generator frame>, <random objects>]
	//
	// Fortunately there's an easy way to distinguish the two: if the current continuation on the
	// stack has a generator frame we're in situation #2, otherwise we're in situation #1.

	Con_Obj *return_obj;

#if CON_USE_UCONTEXT_H
	volatile int ucp_rtn = 0;
	ucontext_t return_env;
	getcontext(&return_env);
	if (ucp_rtn == 0) {
		ucp_rtn = 1;
#else
	JMP_BUF return_env;
#	if CON_HAVE_SIGSETJMP
	if (sigsetjmp(return_env, 0) == 0) {
#	else
	if (setjmp(return_env) == 0) {
#	endif // CON_HAVE_SIGSETJMP
#endif
		if (!Con_Builtins_Con_Stack_Atom_continuation_has_generator_frame(thread, con_stack)) {
			// This is the first pump, so setup a continuation frame.
			u_char *c_stack_start;
			CON_ARCH_GET_STACKP(c_stack_start);
			Con_Builtins_Con_Stack_Atom_update_continuation_frame(thread, con_stack, c_stack_start, &return_env);
			
			// Note that _Con_Builtins_VM_execute returns iff the function being pumped is not a
			// generator.

			CON_MUTEX_UNLOCK(&con_stack->mutex);			
			return_obj = _Con_Builtins_VM_Atom_execute(thread);

			// If the continuation returned a value directly, we're able to optimise things somewhat.

			CON_MUTEX_LOCK(&con_stack->mutex);
			Con_Builtins_Con_Stack_Atom_remove_continuation_frame(thread, con_stack);
			if (remove_finished_generator_frames) {
				// If we're able to remove unneeded generator frames, then we can safely remove the
				// current generator frame since it clearly can't ever be resumed.
				Con_Builtins_Con_Stack_Atom_remove_generator_frame(thread, con_stack);
				CON_MUTEX_UNLOCK(&con_stack->mutex);
			
				if (return_obj == CON_BUILTIN(CON_BUILTIN_FAIL_OBJ))
					return NULL;
			
				return return_obj;
			}

			Con_Builtins_Con_Stack_Atom_set_generator_returned(thread, con_stack);
		}
		else {
			// This is the second or later pump.
			
			// First of all we need to check to see if this generator returned a value (using C's
			// return statement) on its last execution; if it did, there's nothing here to resume.
			
			if (Con_Builtins_Con_Stack_Atom_has_generator_returned(thread, con_stack)) {
				Con_Builtins_Con_Stack_Atom_remove_generator_frame(thread, con_stack);
				CON_MUTEX_UNLOCK(&con_stack->mutex);
				return NULL;
			}
			
			// We need to restore the function on the C stack.
			
			u_char *old_c_stack_start, *suspended_c_stack;
			Con_Int suspended_c_stack_size;
#if CON_USE_UCONTEXT_H
			ucontext_t suspended_env;
#else
			JMP_BUF suspended_env;
#endif
			Con_Builtins_Con_Stack_Atom_prepare_generator_frame_reexecution(thread, con_stack, &old_c_stack_start, &suspended_c_stack, &suspended_c_stack_size, &suspended_env);
			CON_MUTEX_UNLOCK(&con_stack->mutex);

#ifdef CON_C_STACK_GROWS_DOWN
			_Con_Builtins_VM_Atom_apply_pump_restore_c_stack(thread, old_c_stack_start - suspended_c_stack_size, suspended_c_stack, suspended_c_stack_size, suspended_env, 2);
#else
            CON_XXX;
#endif
			// unlock lock?
			CON_XXX;
		}
		
		// Note: control will never reach this point.
	}
	else {
		con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);
		// The suspended generator has returned, so we need to relock the con stacks' mutex.
		CON_MUTEX_LOCK(&con_stack->mutex);
		
		return_obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
	}

	assert(return_obj != NULL);

	if (return_obj == CON_BUILTIN(CON_BUILTIN_FAIL_OBJ)) {
		Con_Builtins_Con_Stack_Atom_remove_generator_frame(thread, con_stack);
		CON_MUTEX_UNLOCK(&con_stack->mutex);
		return NULL;
	}
	else {
		CON_MUTEX_UNLOCK(&con_stack->mutex);
		return return_obj;
	}
}



//
// Yield 'obj' to the caller.
//
// This function should not be called directly; the CON_YIELD or CON_RETURN_FAIL macros should be
// used. Note that the CON_YIELD macro can return values directly to the functions caller and does
// not always call Con_Builtins_VM_yield.
//

void _Con_Builtins_VM_Atom_yield_ss(Con_Obj *, u_char *, u_char **, Con_Int *);

void Con_Builtins_VM_Atom_yield(Con_Obj *thread, Con_Obj *obj)
{
	assert(obj != NULL);

	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);
	
	u_char *c_stack_start;
	u_char *suspended_c_stack;
	Con_Int suspended_c_stack_size;
#if CON_USE_UCONTEXT_H
	ucontext_t return_env;
#else
	JMP_BUF return_env;
#endif
	CON_MUTEX_LOCK(&con_stack->mutex);
	Con_Builtins_Con_Stack_Atom_prepare_to_return_from_generator(thread, con_stack, &c_stack_start, &suspended_c_stack, &suspended_c_stack_size, &return_env);
    
    Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, obj);

#if CON_USE_UCONTEXT_H
	volatile int ucontext_rtn = 0;
	ucontext_t suspended_env;
	getcontext(&suspended_env);
	if (ucontext_rtn == 0) {
		ucontext_rtn = 1;
#else
	JMP_BUF suspended_env;
#	ifdef CON_HAVE_SIGSETJMP
	if (sigsetjmp(suspended_env, 0) == 0) {
#	else
	if (setjmp(suspended_env) == 0) {
#	endif
#endif
		_Con_Builtins_VM_Atom_yield_ss(thread, c_stack_start, &suspended_c_stack, &suspended_c_stack_size);
		Con_Builtins_Con_Stack_Atom_update_generator_frame(thread, con_stack, suspended_c_stack, suspended_c_stack_size, &suspended_env);
		CON_MUTEX_UNLOCK(&con_stack->mutex);
#if CON_USE_UCONTEXT_H
		setcontext(&return_env);
#elif CON_HAVE_SIGSETJMP
		siglongjmp(return_env, 1);
#else
		longjmp(return_env, 1);
#endif
	}
}



//
// This routine actually saves the stack; by calling this we ensure that we save the complete 
//

void _Con_Builtins_VM_Atom_yield_ss(Con_Obj *thread, u_char *c_stack_start, u_char **suspended_c_stack, Con_Int *suspended_c_stack_size)
{
	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);

	u_char *c_stack_end;
	CON_ARCH_GET_STACKP(c_stack_end);

#ifdef CON_C_STACK_GROWS_DOWN
	ptrdiff_t calculated_suspended_c_stack_size = c_stack_start - c_stack_end;
	u_char* c_stack_current = c_stack_end;
#else
    CON_XXX;
#endif

	if (*suspended_c_stack == NULL) {
		// This is the first time we've suspended the C stack, so allocate suitable memory.
		*suspended_c_stack_size = calculated_suspended_c_stack_size;
		CON_MUTEX_UNLOCK(&con_stack->mutex);
		*suspended_c_stack = Con_Memory_malloc(thread, calculated_suspended_c_stack_size, CON_MEMORY_CHUNK_CONSERVATIVE);
		CON_MUTEX_LOCK(&con_stack->mutex);
	}
	else {
		// Since we've suspended the C stack on a previous call to yield, there's no need to allocate
		// new memory, but do a quick sanity check that the old and new stack size are one and the
		// same; if they aren't, this scheme will no longer work.
		assert(calculated_suspended_c_stack_size == *suspended_c_stack_size);
	}
	// Suspend the C stack.
	memmove(*suspended_c_stack, c_stack_current, *suspended_c_stack_size);
}




////////////////////////////////////////////////////////////////////////////////////////////////////
// exbi
//

Con_Obj *Con_Builtins_VM_Atom_exbi(Con_Obj *thread, Con_Obj *class_, Con_Obj *slot_name_obj, const u_char *slot_name, Con_Int slot_name_size, Con_Obj *bind_obj)
{
	assert(((slot_name_obj != NULL) || (slot_name != NULL)) && !((slot_name_obj != NULL) && (slot_name != NULL)));

	if (slot_name == NULL) {
		assert(slot_name_size == 0);
		Con_Builtins_String_Atom *slot_name_obj_string_atom = CON_GET_ATOM(slot_name_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		slot_name = slot_name_obj_string_atom->str;
		slot_name_size = slot_name_obj_string_atom->size;
	}

	Con_Obj *field_val = Con_Builtins_Class_Atom_get_field(thread, class_, slot_name, slot_name_size);

	if ((CON_FIND_ATOM(field_val, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT)) != NULL) && (Con_Builtins_Func_Atom_is_bound(thread, field_val))) {
		return Con_Builtins_Partial_Application_Class_new(thread, field_val, bind_obj, NULL);
	}
	
	CON_XXX;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Exceptions
//

//
// Raises 'exception'.
//
// Note that this function does not return.
//

void Con_Builtins_VM_Atom_raise(Con_Obj *thread, Con_Obj *exception)
{
	// Check that exception has an appropriate atom straight away, so that if that causes a type
	// exception then the call chain is still intact.
	
	if (CON_FIND_ATOM(exception, CON_BUILTIN(CON_BUILTIN_EXCEPTION_ATOM_DEF_OBJECT)) == NULL)
		CON_RAISE_EXCEPTION("Type_Exception", CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_EXCEPTION_CLASS), "path"), exception);

	// We populate call_chain by reverse iterating over the con stack, gradually removing
	// continuations. However we don't set 'exception's call chain until we've finished populating
	// it. That means that potentially vital references (from a GC point of view) could be lost
	// inbetween them being removed from the con stack and them being recorded as being part of an
	// exceptions call chain. Thus 'call_chain' must be a conservatively garbage collected chunk
	// of memory until the point that it is set as an exceptions call chain; after that point it
	// can be marked as opaque as it's the exception's job to do the GC on it.

	Con_Int num_call_chain_entries, num_call_chain_entries_allocated = 10;
	Con_Builtins_Exception_Class_Call_Chain_Entry *call_chain = Con_Memory_malloc(thread, sizeof(Con_Builtins_Exception_Class_Call_Chain_Entry) * num_call_chain_entries_allocated, CON_MEMORY_CHUNK_CONSERVATIVE);
	
	Con_Builtins_Exception_Atom_get_call_chain(thread, exception, &call_chain, &num_call_chain_entries, &num_call_chain_entries_allocated);
	if (call_chain != NULL) {
		// Because of the above "warning", we need to temporarily remark the call chain as being
		// conservatively GC'd to ensure that new entries don't fall between the cracks.
		Con_Memory_change_chunk_type(thread, call_chain, CON_MEMORY_CHUNK_CONSERVATIVE);
	}
	else {
		// Create a blank call chain with a fixed number of entries.
		num_call_chain_entries = 0;
		num_call_chain_entries_allocated = 10;
		call_chain = Con_Memory_malloc(thread, sizeof(Con_Builtins_Exception_Class_Call_Chain_Entry) * num_call_chain_entries_allocated, CON_MEMORY_CHUNK_CONSERVATIVE);
	}

	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);

	CON_MUTEX_LOCK(&con_stack->mutex);
	while (1) {
		Con_Obj *func;
		Con_PC pc;
		Con_Builtins_Con_Stack_Atom_read_continuation_frame(thread, con_stack, &func, NULL, &pc);
		
		if (func != NULL) {
			// There is a continuation so fill out the appropriate entry in the call chain.
			
			if (num_call_chain_entries == num_call_chain_entries_allocated) {
				CON_MUTEX_UNLOCK(&con_stack->mutex);
				Con_Memory_make_array_room(thread, (void **) &call_chain, NULL, &num_call_chain_entries_allocated, &num_call_chain_entries, 1, sizeof(Con_Builtins_Exception_Class_Call_Chain_Entry));
				CON_MUTEX_LOCK(&con_stack->mutex);
			}
			call_chain[num_call_chain_entries].func = func;
			call_chain[num_call_chain_entries].pc = pc;
			num_call_chain_entries += 1;
		}
		
		// We now need to check to see whether the current continuation defines an exception frame.
		
		bool has_exception_frame;
#if CON_USE_UCONTEXT_H
		ucontext_t exception_env;
#else
		JMP_BUF exception_env;
#endif
		Con_PC except_pc;
		Con_Builtins_Con_Stack_Atom_read_exception_frame(thread, con_stack, &has_exception_frame, &exception_env, &except_pc);
		
		if (has_exception_frame) {
			// An exception frame is defined.
			
			CON_MUTEX_UNLOCK(&con_stack->mutex);
			
			Con_Builtins_Exception_Atom_set_call_chain(thread, exception, call_chain, num_call_chain_entries, num_call_chain_entries_allocated);
			
			// Now that we've handed over responsibility (from a GC point of view) for 'call_chain'
			// to 'exception' we can mark it as being on opaque chunk of memory.
			
			Con_Memory_change_chunk_type(thread, call_chain, CON_MEMORY_CHUNK_OPAQUE);
			
#			ifndef CON_THREADS_SINGLE_THREADED
			Con_Builtins_Exception_Atom_set_exception_thread(thread, exception, thread);
#			endif
			
			Con_Obj *vm = Con_Builtins_Thread_Atom_get_vm(thread);
			Con_Builtins_VM_Atom *vm_atom = (Con_Builtins_VM_Atom *) vm->first_atom;
			CON_MUTEX_LOCK(&vm->mutex);
			vm_atom->current_exception = exception;
			CON_MUTEX_UNLOCK(&vm->mutex);
			
			// Remove the exception frame from the stack...
			
			CON_MUTEX_LOCK(&con_stack->mutex);
			Con_Builtins_Con_Stack_Atom_remove_exception_frame(thread, con_stack);
			CON_MUTEX_UNLOCK(&con_stack->mutex);
			
			if (except_pc.type != PC_TYPE_NULL)
				Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, except_pc);
			
			// ...and then jump to its handler.
			
#if CON_USE_UCONTEXT_H
			setcontext(&exception_env);
#else
			longjmp(exception_env, 1);
#endif
		}
		else if (func == NULL) {
			// We've reached the top of the stack but no exception handler has been found. This
			// shouldn't happen in normal circumstances as all threads should define an appropriate
			// exception handler. However if something goes wrong in the exception handler then the
			// only conclusion is that something seriously bad has happened and the best course of
			// action is to halt the VM.
			
			printf("%s\n", Con_Builtins_String_Atom_to_c_string(thread, CON_GET_SLOT_APPLY(exception, "to_str")));
			CON_FATAL_ERROR("Exception percolated to top of call stack but no exception handler found.");
		}
		
		// Since this continuation didn't have an exception frame, pop it off the con stack.
		
		Con_Builtins_Con_Stack_Atom_remove_continuation_frame(thread, con_stack);
	}
}



//
// Returns the last raised exception.
//
// This is only intended for use internally by the VM, and by the CON_CATCH macro.
//

Con_Obj *Con_Builtins_VM_Atom_get_current_exception(Con_Obj *thread)
{
	Con_Obj *vm = Con_Builtins_Thread_Atom_get_vm(thread);
	Con_Builtins_VM_Atom *vm_atom = (Con_Builtins_VM_Atom *) vm->first_atom;
	
	CON_MUTEX_LOCK(&vm->mutex);
	Con_Obj *exception = vm_atom->current_exception;
	assert(exception != NULL);
	CON_MUTEX_UNLOCK(&vm->mutex);
	
	return exception;
}



//
// Resets the last raised exception. This signifies that the exception has been dealt with in
// some fashion.
//
// This is only intended for use internally by the VM, and by the CON_CATCH macro.
//

void Con_Builtins_VM_Atom_reset_current_exception(Con_Obj *thread)
{
	Con_Obj *vm = Con_Builtins_Thread_Atom_get_vm(thread);
	Con_Builtins_VM_Atom *vm_atom = (Con_Builtins_VM_Atom *) vm->first_atom;
	
	CON_MUTEX_LOCK(&vm->mutex);
	assert(vm_atom->current_exception != NULL);
	vm_atom->current_exception = NULL;
	CON_MUTEX_UNLOCK(&vm->mutex);
}



//
// Ensures that the VM is not currently dealing with an exception. This should be called before
// throwing an exception to detect errors in the error handlers.
//
// This is only intended for use internally by the VM, and by the CON_CATCH macro.
//

void Con_Builtins_VM_Atom_ensure_no_current_exception(Con_Obj *thread)
{
	Con_Obj *vm = Con_Builtins_Thread_Atom_get_vm(thread);
	Con_Builtins_VM_Atom *vm_atom = (Con_Builtins_VM_Atom *) vm->first_atom;
	
	CON_MUTEX_LOCK(&vm->mutex);
	if (vm_atom->current_exception != NULL)
		CON_FATAL_ERROR("Exception while handling exception");
	CON_MUTEX_UNLOCK(&vm->mutex);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// VM execution
//

Con_Obj *_Con_Builtins_VM_Atom_execute(Con_Obj *thread)
{
	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);
	
	CON_ASSERT_MUTEX_LOCKED(&con_stack->mutex);

	Con_PC pc;
	Con_Obj *func, *closure;
	Con_Builtins_Con_Stack_Atom_read_continuation_frame(thread, con_stack, &func, &closure, &pc);

	if (pc.type == PC_TYPE_C_FUNCTION) {
		CON_MUTEX_UNLOCK(&con_stack->mutex);
		if (_Con_Builtins_VM_Atom_signal_trap_sigint == true) {
			_Con_Builtins_VM_Atom_signal_trap_sigint = false;
			CON_RAISE_EXCEPTION("Signal_Exception", CON_NEW_STRING("SIGINT."));
		}
		Con_Memory_gc_poll(thread);
		return pc.pc.c_function(thread);
	}
	else if (pc.type == PC_TYPE_BYTECODE) {
		int poll = 0;
		while (1) {
			if (_Con_Builtins_VM_Atom_signal_trap_sigint == true) {
				CON_MUTEX_UNLOCK(&con_stack->mutex);
				_Con_Builtins_VM_Atom_signal_trap_sigint = false;
				CON_RAISE_EXCEPTION("Signal_Exception", CON_NEW_STRING("SIGINT."));
			}

			if (poll == 100) {
				CON_MUTEX_UNLOCK(&con_stack->mutex);
				Con_Memory_gc_poll(thread);
				poll = 0;
				CON_MUTEX_LOCK(&con_stack->mutex);
			}
			else
				poll += 1;
			
			Con_Int instruction = Con_Builtins_Module_Atom_get_instruction(thread, pc.module, pc.pc.bytecode_offset);
#			if CON_FULL_DEBUG
			printf("[%p] %d : %s [0x%x] ", thread, pc.pc.bytecode_offset, Con_Instructions_names[instruction & 0x000000FF], (instruction & 0xFFFFFF00) >> 8);
			Con_Builtins_Con_Stack_Atom_print(thread, con_stack);
			fflush(NULL);
#			endif

			switch (instruction & 0x000000FF) {
				case CON_INSTR_EXBI: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Int field_name_size = CON_INSTR_DECODE_EXBI_SIZE(instruction);
					u_char field_name[field_name_size];
					Con_Builtins_Module_Atom_read_bytes(thread, pc.module, pc.pc.bytecode_offset + CON_INSTR_DECODE_EXBI_START(instruction), field_name, field_name_size);

					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Obj *obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					Con_Obj *bind_obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Obj *val = Con_Builtins_VM_Atom_exbi(thread, obj, NULL, field_name, field_name_size, bind_obj);
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, val);
					pc.pc.bytecode_offset += Con_Arch_align(thread, CON_INSTR_DECODE_EXBI_START(instruction) + field_name_size);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_VAR_LOOKUP: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Obj *obj = Con_Builtins_Closure_Atom_get_var(thread, closure, CON_INSTR_DECODE_VAR_LOOKUP_CLOSURES_OFFSET(instruction), CON_INSTR_DECODE_VAR_LOOKUP_VAR_NUM(instruction));
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, obj);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_VAR_ASSIGN: {
					Con_Obj *obj = Con_Builtins_Con_Stack_Atom_peek_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Builtins_Closure_Atom_set_var(thread, closure, CON_INSTR_DECODE_VAR_ASSIGN_CLOSURES_OFFSET(instruction), CON_INSTR_DECODE_VAR_ASSIGN_VAR_NUM(instruction), obj);
					CON_MUTEX_LOCK(&con_stack->mutex);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_INT: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Int int_val;
					//printf("%p %p\n", instruction >> 8, CON_INSTR_DECODE_INT_SIGN(instruction));
					if (CON_INSTR_DECODE_INT_SIGN(instruction))
						int_val = -((Con_Int) CON_INSTR_DECODE_INT_VAL(instruction));
					else
						int_val = CON_INSTR_DECODE_INT_VAL(instruction);
					Con_Obj *new_int_obj = CON_NEW_INT(int_val);
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, new_int_obj);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_FLOAT: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Float float_val;
					Con_Builtins_Module_Atom_read_bytes(thread, pc.module, pc.pc.bytecode_offset + sizeof(Con_Int), &float_val, sizeof(Con_Float));
					Con_Obj *float_obj = Con_Builtins_Float_Atom_new(thread, float_val);
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, float_obj);
					pc.pc.bytecode_offset += sizeof(Con_Int) + sizeof(Con_Float);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_ADD_FAILURE_FRAME: {
					Con_PC new_pc = pc;
					if (CON_INSTR_DECODE_ADD_FAILURE_FRAME_SIGN(instruction))
						new_pc.pc.bytecode_offset -= CON_INSTR_DECODE_ADD_FAILURE_FRAME_OFFSET(instruction);
					else
						new_pc.pc.bytecode_offset += CON_INSTR_DECODE_ADD_FAILURE_FRAME_OFFSET(instruction);
					Con_Builtins_Con_Stack_Atom_add_failure_frame(thread, con_stack, new_pc);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_ADD_FAIL_UP_FRAME: {
					Con_Builtins_Con_Stack_Atom_add_failure_fail_up_frame(thread, con_stack);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_REMOVE_FAILURE_FRAME: {
					Con_Builtins_Con_Stack_Atom_remove_failure_frame(thread, con_stack);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_IS_ASSIGNED: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					if (Con_Builtins_Closure_Atom_is_assigned(thread, closure, CON_INSTR_DECODE_IS_ASSIGNED_CLOSURES_OFFSET(instruction), CON_INSTR_DECODE_IS_ASSIGNED_VAR_NUM(instruction))) {
						Con_Int branch_info = Con_Builtins_Module_Atom_get_instruction(thread, pc.module, pc.pc.bytecode_offset + sizeof(Con_Int));
						if (CON_INSTR_DECODE_IS_ASSIGNED_BRANCH_SIGN(branch_info))
							pc.pc.bytecode_offset -= CON_INSTR_DECODE_IS_ASSIGNED_BRANCH_OFFSET(branch_info);
						else
							pc.pc.bytecode_offset += CON_INSTR_DECODE_IS_ASSIGNED_BRANCH_OFFSET(branch_info);
					}
					else
						pc.pc.bytecode_offset += sizeof(Con_Int) + sizeof(Con_Int);
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_IS: {
					Con_Obj *x = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					Con_Obj *y = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					Con_Obj *rtn = CON_GET_SLOT_APPLY_NO_FAIL(x, "is", y);
					if (rtn == NULL)
						goto fail_now;
					
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, rtn);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_FAIL_NOW:
					goto fail_now;
				case CON_INSTR_POP: {
					Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_IMPORT: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Obj *new_module = Con_Modules_import_mod_from_bytecode(thread, CON_GET_SLOT(func, "container"), CON_INSTR_DECODE_IMPORT_MOD_NUM(instruction));
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, new_module);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_LIST: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Obj *new_list = Con_Builtins_List_Atom_new_from_con_stack(thread, con_stack, CON_INSTR_DECODE_LIST_NUM_ENTRIES(instruction));
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, new_list);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_SLOT_LOOKUP: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Int slot_name_size = CON_INSTR_DECODE_SLOT_LOOKUP_SIZE(instruction);
					u_char slot_name[slot_name_size];
					Con_Builtins_Module_Atom_read_bytes(thread, pc.module, pc.pc.bytecode_offset + CON_INSTR_DECODE_SLOT_LOOKUP_START(instruction), slot_name, slot_name_size);
					
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Obj *obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Obj *val = Con_Object_get_slot(thread, obj, NULL, slot_name, slot_name_size);
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, val);
					pc.pc.bytecode_offset += Con_Arch_align(thread, CON_INSTR_DECODE_SLOT_LOOKUP_START(instruction) + slot_name_size);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_APPLY: {
					Con_Int num_args = CON_INSTR_DECODE_APPLY_NUM_ARGS(instruction);
					Con_Obj *apply_func;
					if (Con_Builtins_Con_Stack_Atom_pop_n_object_or_slot_lookup_apply(thread, con_stack, num_args, &apply_func)) {
						num_args += 1;
					}
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					
/*					if (CON_FIND_ATOM(apply_func, CON_BUILTIN(CON_BUILTIN_CLASS_CLASS)) != NULL) {
						apply_func = CON_GET_SLOT(apply_func, "new");
					}*/
					
					if (CON_FIND_ATOM(apply_func, CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT)) != NULL) {
						num_args += Con_Builtins_Partial_Application_Atom_push_args_onto_stack(thread, apply_func, con_stack, num_args);
						apply_func = Con_Builtins_Partial_Application_Atom_get_func(thread, apply_func);
					}
					
					if (CON_FIND_ATOM(apply_func, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT)) == NULL) {
						CON_RAISE_EXCEPTION("Apply_Exception", apply_func);
					}

					Con_Int num_closure_vars = Con_Builtins_Func_Atom_get_num_closure_vars(thread, apply_func);
					Con_Obj *container_closure = Con_Builtins_Func_Atom_get_container_closure(thread, apply_func);
					Con_Obj *closure = NULL;
					if ((num_closure_vars > 0) || (container_closure != NULL))
						closure = Con_Builtins_Closure_Atom_new(thread, num_closure_vars, container_closure);
					Con_Obj *rtn_obj;
					
					// Unless we're in a "fail up" failure frame, there's no chance of a generator
					// being resumed after its initial execution so we avoid setting up a relatively
					// expensive pumping application in such cases.
					//
					// Since most applications aren't going to be a "fail up" failure frame, this can
					// be a fairly significant win.

					bool is_fail_up_failure_frame;
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_read_failure_frame(thread, con_stack, &is_fail_up_failure_frame, NULL, NULL);
					CON_MUTEX_UNLOCK(&con_stack->mutex);

					if (is_fail_up_failure_frame) {
						Con_PC resumption_pc = pc;
						resumption_pc.pc.bytecode_offset += sizeof(Con_Int);
						Con_Builtins_VM_Atom_pre_apply_pump(thread, apply_func, num_args, closure, resumption_pc);
						rtn_obj = Con_Builtins_VM_Atom_apply_pump(thread, true);
					}
					else {
						// If we're doing non-pumping application, suppress failure; we deal with
						// that by "goto fail_now;". This also means that we can uniformly handle
						// pumping and non-pumping application in this VM instruction.
					
						rtn_obj = _Con_Builtins_VM_Atom_apply(thread, apply_func, num_args, closure, true);
					}
					
					if (rtn_obj == NULL) {
						CON_MUTEX_LOCK(&con_stack->mutex);
						goto fail_now;
					}
						
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, rtn_obj);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_FUNC_DEFN: {
					Con_PC new_pc = pc;
					new_pc.pc.bytecode_offset += sizeof(Con_Int) + sizeof(Con_Int);
					Con_Obj *num_params_obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					Con_Obj *num_local_vars_obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					Con_Obj *name = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					
					Con_Int num_params = Con_Numbers_Number_to_Con_Int(thread, num_params_obj);
					Con_Int num_local_vars = Con_Numbers_Number_to_Con_Int(thread, num_local_vars_obj);
					Con_Obj *new_func = Con_Builtins_Func_Atom_new(thread, CON_INSTR_DECODE_FUNC_DEF_IS_BOUND(instruction), new_pc, num_params, num_local_vars, closure, name, CON_GET_SLOT(func, "container"));
					
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, new_func);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_RETURN: {
					Con_Obj *rtn_obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					return rtn_obj;
				}
				case CON_INSTR_BRANCH:
					if (CON_INSTR_DECODE_BRANCH_SIGN(instruction))
						pc.pc.bytecode_offset -= CON_INSTR_DECODE_BRANCH_OFFSET(instruction);
					else
						pc.pc.bytecode_offset += CON_INSTR_DECODE_BRANCH_OFFSET(instruction);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				case CON_INSTR_YIELD: {
					Con_Obj *yield_obj = Con_Builtins_Con_Stack_Atom_peek_object(thread, con_stack);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					CON_YIELD(yield_obj);
					break;
				}
				case CON_INSTR_DICT: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Int num_entries = CON_INSTR_DECODE_DICT_NUM_ENTRIES(instruction);
					Con_Obj *dict = Con_Builtins_Dict_Atom_new(thread);
					for (int i = num_entries - 1; i >= 0; i -= 1) {
						CON_MUTEX_LOCK(&con_stack->mutex);
						Con_Obj *val = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
						Con_Obj *key = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
						CON_MUTEX_UNLOCK(&con_stack->mutex);
						CON_GET_SLOT_APPLY(dict, "set", key, val);
					}
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, dict);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_DUP: {
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, Con_Builtins_Con_Stack_Atom_peek_object(thread, con_stack));
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_PULL: {
					Con_Obj *obj = Con_Builtins_Con_Stack_Atom_pop_n_object(thread, con_stack, CON_INSTR_DECODE_PULL_N(instruction));
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, obj);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_STRING: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Int size = CON_INSTR_DECODE_STRING_SIZE(instruction);
					Con_Obj *str = Con_Builtins_Module_Atom_get_string(thread, pc.module, pc.pc.bytecode_offset + CON_INSTR_DECODE_STRING_START(instruction), size);
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, str);
					pc.pc.bytecode_offset += Con_Arch_align(thread, CON_INSTR_DECODE_STRING_START(instruction) + size);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_BUILTIN_LOOKUP:
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, CON_BUILTIN(CON_INSTR_DECODE_IMPORT_BUILTIN_LOOKUP(instruction)));
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				case CON_INSTR_ASSIGN_SLOT: {
					Con_Obj *assignee_obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					Con_Obj *val = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					
					Con_Int slot_name_size = CON_INSTR_DECODE_SLOT_LOOKUP_SIZE(instruction);
					u_char slot_name[slot_name_size];
					Con_Builtins_Module_Atom_read_bytes(thread, pc.module, pc.pc.bytecode_offset + CON_INSTR_DECODE_ASSIGN_SLOT_START(instruction), slot_name, CON_INSTR_DECODE_SLOT_LOOKUP_SIZE(instruction));
					Con_Object_set_slot(thread, assignee_obj, NULL, slot_name, slot_name_size, val);
					
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, val);
					pc.pc.bytecode_offset += Con_Arch_align(thread, CON_INSTR_DECODE_ASSIGN_SLOT_START(instruction) + slot_name_size);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_EYIELD: {
					Con_Obj *eyield_obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
				
					// We know that we must be inside a non-"fail up" failure frame,
					// since the EYIELD op is only used by the alternation construct.
					
					Con_PC resumption_pc;
					Con_Builtins_Con_Stack_Atom_read_failure_frame(thread, con_stack, NULL, NULL, &resumption_pc);
					Con_Builtins_Con_Stack_Atom_remove_failure_frame(thread, con_stack);
					Con_Builtins_Con_stack_Atom_add_generator_eyield_frame(thread, con_stack, resumption_pc);
					
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, eyield_obj);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_ADD_EXCEPTION_FRAME: {
					Con_PC new_pc = pc;
					if (CON_INSTR_DECODE_ADD_EXCEPTION_FRAME_SIGN(instruction))
						new_pc.pc.bytecode_offset -= CON_INSTR_DECODE_ADD_EXCEPTION_FRAME_OFFSET(instruction);
					else
						new_pc.pc.bytecode_offset += CON_INSTR_DECODE_ADD_EXCEPTION_FRAME_OFFSET(instruction);
#if CON_USE_UCONTEXT_H
					ucontext_t except_env;
					volatile int ucontext_rtn = 0;
					getcontext(&except_env);
					if (ucontext_rtn == 0) {
						ucontext_rtn = 1;
#else
					JMP_BUF except_env;
#	ifdef CON_HAVE_SIGSETJMP
					if (sigsetjmp(except_env, 0) == 0) {
#	else
					if (setjmp(except_env) == 0) {
#	endif
#endif

#if CON_USE_UCONTEXT_H
						Con_Builtins_Con_Stack_Atom_add_exception_frame(thread, con_stack, &except_env, new_pc);
#else
						Con_Builtins_Con_Stack_Atom_add_exception_frame(thread, con_stack, except_env, new_pc);
#endif
						pc.pc.bytecode_offset += sizeof(Con_Int);
						Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					}
					else {
						Con_Obj *e = Con_Builtins_VM_Atom_get_current_exception(thread);
						Con_Builtins_VM_Atom_reset_current_exception(thread);
						Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, e);
						Con_Builtins_Con_Stack_Atom_read_continuation_frame(thread, con_stack, &func, NULL, &pc);
					}
					break;
				}
				case CON_INSTR_REMOVE_EXCEPTION_FRAME: {
					Con_Builtins_Con_Stack_Atom_remove_exception_frame(thread, con_stack);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_RAISE: {
					Con_Obj *exception = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Builtins_VM_Atom_raise(thread, exception);
					break; // We won't get this far, but just emphasise that there's no fall through.
				}
				case CON_INSTR_UNPACK_ARGS: {
					Con_Obj *num_args_obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Int num_args = Con_Numbers_Number_to_Con_Int(thread, num_args_obj);

					if ((num_args > CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction)) && !CON_INSTR_DECODE_UNPACK_ARGS_HAS_VAR_ARGS(instruction)) {
						Con_Obj *msg = CON_NEW_STRING("Too many parameters (");
						msg = CON_ADD(msg, CON_GET_SLOT_APPLY(CON_NEW_INT(num_args), "to_str"));
						msg = CON_ADD(msg, CON_NEW_STRING(" passed, but a maximum of "));
						msg = CON_ADD(msg, CON_GET_SLOT_APPLY(CON_NEW_INT(CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction)), "to_str"));
						msg = CON_ADD(msg, CON_NEW_STRING(" allowed)."));
						CON_RAISE_EXCEPTION("Parameters_Exception", msg);
					}

					// We have to unpack the arguments in reverse order.
					
					Con_Int arg_offset = pc.pc.bytecode_offset + sizeof(Con_Int) + CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction) * sizeof(Con_Int);
					for (Con_Int i = CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction) - 1; i >= 0; i -= 1) {
						arg_offset -= sizeof(Con_Int);
						Con_Int arg_info = Con_Builtins_Module_Atom_get_instruction(thread, pc.module, arg_offset);
						if (i >= num_args) {
							if (!CON_INSTR_DECODE_UNPACK_ARGS_IS_MANDATORY(arg_info)) {
								Con_Obj *msg = CON_NEW_STRING("No value passed for parameter num ");
								msg = CON_ADD(msg, CON_GET_SLOT_APPLY(CON_NEW_INT(i), "to_str"));
								msg = CON_ADD(msg, CON_NEW_STRING("."));
								CON_RAISE_EXCEPTION("Parameters_Exception", msg);
							}
						}
						else {
							CON_MUTEX_LOCK(&con_stack->mutex);
							Con_Obj *obj;
							if (num_args > CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction))
								obj = Con_Builtins_Con_Stack_Atom_pop_n_object(thread, con_stack, num_args - CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction));
							else
								obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
							CON_MUTEX_UNLOCK(&con_stack->mutex);
							Con_Builtins_Closure_Atom_set_var(thread, closure, 0, CON_INSTR_DECODE_UNPACK_ARGS_ARG_NUM(arg_info), obj);
						}
					}
					
					if (CON_INSTR_DECODE_UNPACK_ARGS_HAS_VAR_ARGS(instruction)) {
						arg_offset = pc.pc.bytecode_offset + sizeof(Con_Int) + CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction) * sizeof(Con_Int);
						Con_Int arg_info = Con_Builtins_Module_Atom_get_instruction(thread, pc.module, arg_offset);
						Con_Obj *new_list;
						if (num_args <= CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction))
							new_list = Con_Builtins_List_Atom_new_sized(thread, 0);
						else {
							new_list = Con_Builtins_List_Atom_new_sized(thread, num_args - CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction));
							for (Con_Int j = CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction); j < num_args; j += 1) {
								CON_MUTEX_LOCK(&con_stack->mutex);
								Con_Obj *obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
								CON_MUTEX_UNLOCK(&con_stack->mutex);
								CON_GET_SLOT_APPLY(new_list, "insert", CON_NEW_INT(0), obj);
							}
						}
						Con_Builtins_Closure_Atom_set_var(thread, closure, 0, CON_INSTR_DECODE_UNPACK_ARGS_ARG_NUM(arg_info), new_list);
					}
					
					CON_MUTEX_LOCK(&con_stack->mutex);
					if (CON_INSTR_DECODE_UNPACK_ARGS_HAS_VAR_ARGS(instruction))
						pc.pc.bytecode_offset += sizeof(Con_Int) + (CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction) + 1) * sizeof(Con_Int);
					else
						pc.pc.bytecode_offset += sizeof(Con_Int) + CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction) * sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_SET: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Obj *new_set = Con_Builtins_Set_Atom_new_from_con_stack(thread, con_stack, CON_INSTR_DECODE_LIST_NUM_ENTRIES(instruction));
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, new_set);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_BRANCH_IF_NOT_FAIL: {
					Con_Obj *obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					if (obj != CON_BUILTIN(CON_BUILTIN_FAIL_OBJ)) {
						if (CON_INSTR_DECODE_BRANCH_IF_FAIL_SIGN(instruction))
							pc.pc.bytecode_offset -= CON_INSTR_DECODE_BRANCH_IF_FAIL_OFFSET(instruction);
						else
							pc.pc.bytecode_offset += CON_INSTR_DECODE_BRANCH_IF_FAIL_OFFSET(instruction);
					}
					else
						pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_BRANCH_IF_FAIL: {
					Con_Obj *obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					if (obj == CON_BUILTIN(CON_BUILTIN_FAIL_OBJ)) {
						if (CON_INSTR_DECODE_BRANCH_IF_FAIL_SIGN(instruction))
							pc.pc.bytecode_offset -= CON_INSTR_DECODE_BRANCH_IF_FAIL_OFFSET(instruction);
						else
							pc.pc.bytecode_offset += CON_INSTR_DECODE_BRANCH_IF_FAIL_OFFSET(instruction);
					}
					else
						pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_CONSTANT_GET: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Obj *val = Con_Builtins_Module_Atom_get_constant(thread, pc.module, CON_INSTR_DECODE_CONSTANT_GET_NUM(instruction));
					if (val == NULL) {
						Con_PC new_pc = pc;
						pc.pc.bytecode_offset = Con_Builtins_Module_Atom_get_constant_create_offset(thread, pc.module, CON_INSTR_DECODE_CONSTANT_GET_NUM(instruction));
						CON_MUTEX_LOCK(&con_stack->mutex);
						Con_Builtins_Con_Stack_Atom_add_failure_frame(thread, con_stack, new_pc);
						Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					}
					else {
						CON_MUTEX_LOCK(&con_stack->mutex);
						Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, val);
						pc.pc.bytecode_offset += sizeof(Con_Int);
						Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					}
					break;
				}
				case CON_INSTR_CONSTANT_SET: {
					Con_Obj *obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Builtins_Module_Atom_set_constant(thread, pc.module, CON_INSTR_DECODE_CONSTANT_SET_NUM(instruction), obj);
					CON_MUTEX_LOCK(&con_stack->mutex);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_PRE_SLOT_LOOKUP_APPLY: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Int slot_name_size = CON_INSTR_DECODE_PRE_SLOT_LOOKUP_SIZE(instruction);
					u_char slot_name[slot_name_size];
					Con_Builtins_Module_Atom_read_bytes(thread, pc.module, pc.pc.bytecode_offset + CON_INSTR_DECODE_PRE_SLOT_LOOKUP_START(instruction), slot_name, slot_name_size);
					
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Obj *obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					bool custom_get_slot;
					Con_Obj *val = Con_Object_get_slot_no_binding(thread, obj, NULL, slot_name, slot_name_size, &custom_get_slot);
					bool skip_binding = true;
					if (!custom_get_slot && (CON_FIND_ATOM(val, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT)) != NULL) && (Con_Builtins_Func_Atom_is_bound(thread, val)))
						skip_binding = false;
						
					CON_MUTEX_LOCK(&con_stack->mutex);
					if (skip_binding)
						Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, val);
					else
						Con_Builtins_Con_Stack_Atom_push_slot_lookup_apply(thread, con_stack, obj, val);
					pc.pc.bytecode_offset += Con_Arch_align(thread, CON_INSTR_DECODE_PRE_SLOT_LOOKUP_START(instruction) + slot_name_size);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_UNPACK_ASSIGN: {
					// This instruction is intended for an assignment of the form:
					//   x, y := [4, 5]
					// It first checks that the list on the RHS is of the correct length; it then
					// pushes each element from the list onto the stack in reverse order ready for
					// assignment.
				
					Con_Obj *obj = Con_Builtins_Con_Stack_Atom_peek_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					if (!Con_Object_eq(thread, CON_GET_SLOT_APPLY(obj, "len"), CON_NEW_INT(CON_INSTR_DECODE_UNPACK_ASSIGN_NUM_ELEMENTS(instruction)))) {
						Con_Obj *len = CON_GET_SLOT_APPLY(obj, "len");
						CON_RAISE_EXCEPTION("Unpack_Exception", CON_NEW_INT(CON_INSTR_DECODE_UNPACK_ASSIGN_NUM_ELEMENTS(instruction)), len);
					}
					
					for (Con_Int i = (Con_Int) CON_INSTR_DECODE_UNPACK_ASSIGN_NUM_ELEMENTS(instruction) - 1; i >= 0; i -= 1) {
						Con_Obj *elem = CON_GET_SLOT_APPLY(obj, "get", CON_NEW_INT(i));
						CON_MUTEX_LOCK(&con_stack->mutex);
						Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, elem);
						CON_MUTEX_UNLOCK(&con_stack->mutex);
					}
					CON_MUTEX_LOCK(&con_stack->mutex);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_EQ:
				case CON_INSTR_NEQ:
				case CON_INSTR_GT: 
				case CON_INSTR_LE: 
				case CON_INSTR_LE_EQ:
				case CON_INSTR_GR_EQ: {
					Con_Obj *rhs = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					Con_Obj *lhs = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					switch (instruction) {
						case CON_INSTR_EQ:
							if (!Con_Object_eq(thread, lhs, rhs)) {
								CON_MUTEX_LOCK(&con_stack->mutex);
								goto fail_now;
							}
							break;
						case CON_INSTR_NEQ:
							if (!Con_Object_neq(thread, lhs, rhs)) {
								CON_MUTEX_LOCK(&con_stack->mutex);
								goto fail_now;
							}
							break;
						case CON_INSTR_GT:
							if (!Con_Object_gt(thread, lhs, rhs)) {
								CON_MUTEX_LOCK(&con_stack->mutex);
								goto fail_now;
							}
							break;
						case CON_INSTR_LE:
							if (!Con_Object_le(thread, lhs, rhs)) {
								CON_MUTEX_LOCK(&con_stack->mutex);
								goto fail_now;
							}
							break;
						case CON_INSTR_LE_EQ:
							if (!Con_Object_le_eq(thread, lhs, rhs)) {
								CON_MUTEX_LOCK(&con_stack->mutex);
								goto fail_now;
							}
							break;
						case CON_INSTR_GR_EQ:
							if (!Con_Object_gr_eq(thread, lhs, rhs)) {
								CON_MUTEX_LOCK(&con_stack->mutex);
								goto fail_now;
							}
							break;
						default:
							CON_XXX;
					}
						
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, rhs);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_ADD:
				case CON_INSTR_SUBTRACT: {
					Con_Obj *rhs = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					Con_Obj *lhs = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Obj *rtn;
					switch (instruction) {
						case CON_INSTR_ADD:
							if ((rtn = Con_Object_add(thread, lhs, rhs)) == NULL) {
								CON_MUTEX_LOCK(&con_stack->mutex);
								goto fail_now;
							}
							break;
						case CON_INSTR_SUBTRACT:
							if ((rtn = Con_Object_subtract(thread, lhs, rhs)) == NULL) {
								CON_MUTEX_LOCK(&con_stack->mutex);
								goto fail_now;
							}
							break;
						default:
							CON_XXX;
					}
						
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, rtn);
					pc.pc.bytecode_offset += sizeof(Con_Int);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				case CON_INSTR_MODULE_LOOKUP: {
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Int definition_name_size = CON_INSTR_DECODE_MODULE_LOOKUP_SIZE(instruction);
					u_char definition_name[definition_name_size];
					Con_Builtins_Module_Atom_read_bytes(thread, pc.module, pc.pc.bytecode_offset + CON_INSTR_DECODE_MODULE_LOOKUP_START(instruction), definition_name, definition_name_size);
					
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Obj *module = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
					CON_MUTEX_UNLOCK(&con_stack->mutex);
					Con_Obj *val = Con_Builtins_Module_Atom_get_defn(thread, module, definition_name, definition_name_size);
					CON_MUTEX_LOCK(&con_stack->mutex);
					Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, val);
					pc.pc.bytecode_offset += Con_Arch_align(thread, CON_INSTR_DECODE_MODULE_LOOKUP_START(instruction) + definition_name_size);
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, pc);
					break;
				}
				default:
					CON_FATAL_ERROR("Unknown instruction");
			}
			// The continue statement forces execution of the next VM instruction, and means that
			// the fail_now code isn't inadvertantly executed.
			continue;
			
fail_now:
			while (1) {
				bool is_fail_up, has_resumable_generator;
				Con_PC fail_to_pc;
				Con_Builtins_Con_Stack_Atom_read_failure_frame(thread, con_stack, &is_fail_up, &has_resumable_generator, &fail_to_pc);
				if (is_fail_up) {
					if (has_resumable_generator) {
						bool is_eyield_generator_frame;
						Con_PC resumption_pc;
						Con_Builtins_Con_Stack_Atom_read_generator_frame(thread, con_stack, &is_eyield_generator_frame, &resumption_pc);
						if (is_eyield_generator_frame) {
							Con_Builtins_Con_Stack_Atom_remove_generator_frame(thread, con_stack);
							pc = resumption_pc;
							Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, resumption_pc);
							break;
						}
						else {
							CON_MUTEX_UNLOCK(&con_stack->mutex);
							Con_Obj *obj = Con_Builtins_VM_Atom_apply_pump(thread, true);
							CON_MUTEX_LOCK(&con_stack->mutex);
							if (obj == NULL) {
								continue;
							}
							else {
								Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, obj);
								pc = resumption_pc;
								Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, resumption_pc);
								break;
							}
						}
					}
					else {
						Con_Builtins_Con_Stack_Atom_remove_failure_frame(thread, con_stack);
						pc = fail_to_pc;
						Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, fail_to_pc);
						continue;
					}
				}
				else {
					Con_Builtins_Con_Stack_Atom_remove_failure_frame(thread, con_stack);
					pc = fail_to_pc;
					Con_Builtins_Con_Stack_Atom_update_continuation_frame_pc(thread, con_stack, fail_to_pc);
					break;
				}
			}
		}
	}
	else
		CON_XXX;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_VM_Atom_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_VM_Atom *vm_atom = (Con_Builtins_VM_Atom *) atom;

	if (vm_atom->current_exception != NULL)
		Con_Memory_gc_push(thread, vm_atom->current_exception);

	for (int i = 0; i < CON_NUMBER_OF_BUILTINS; i += 1) {
		// XXX should be Con_Memory_gc_push but these might not yet be filled in
		Con_Memory_gc_push_ptr(thread, vm_atom->builtins[i]);
	}
}



