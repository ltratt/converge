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


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Debugging
//

#define CON_XXX do { fflush(NULL); printf("XXX exit line %d %s: \n", __LINE__, __FILE__); abort(); } while (0)
#define CON_FATAL_ERROR(x) do { printf("Fatal error line %d file %s:  %s\n", __LINE__, __FILE__, x); abort(); } while (0)
#define CON_FATAL_ERROR_OVERFLOW CON_FATAL_ERROR("Buffer overflow.");



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Threading
//

#ifdef CON_THREADS_SINGLE_THREADED
	// Make these no-ops that are hopefully simple enough for any half-decent compiler to optimise
	// away.

#	define CON_MUTEX_LOCK(mutex) while (0) {if (mutex) {}}
#	define CON_MUTEX_ADD_LOCK(...) while (0) {}
#	define CON_MUTEX_UNLOCK(mutex) while (0) {if (mutex) {}}
#	define CON_ASSERT_MUTEX_LOCKED(mutex) while (0) {if (mutex) {}}

#	define CON_MUTEXES_LOCK(...)
#	define CON_MUTEXES_UNLOCK(...)

#	define CON_THREAD_START_BLOCKING_CALL()
#	define CON_THREAD_END_BLOCKING_CALL()

#else

#	define CON_MUTEX_LOCK(mutex) Con_Builtins_Thread_Atom_mutex_lock(thread, mutex)
#	define CON_MUTEX_ADD_LOCK(...) Con_Builtins_Thread_Atom_mutex_add_lock(thread, ## __VA_ARGS__, NULL)
#	define CON_MUTEX_UNLOCK(mutex) Con_Builtins_Thread_Atom_mutex_unlock(thread, mutex)
#	define CON_ASSERT_MUTEX_LOCKED(mutex) assert(!Con_Builtins_Thread_Atom_mutex_trylock(thread, mutex));

#	ifdef __GNUC__
#		define CON_MUTEXES_LOCK(...) Con_Builtins_Thread_Atom_mutexes_lock(thread, ## __VA_ARGS__, NULL)
#		define CON_MUTEXES_UNLOCK(...) Con_Builtins_Thread_Atom_mutexes_unlock(thread, ## __VA_ARGS__, NULL)
#	else
#		error "Unsupported compiler"
#	endif

#	define CON_THREAD_START_BLOCKING_CALL() Con_Builtins_Thread_Atom_start_blocking_call(thread)
#	define CON_THREAD_END_BLOCKING_CALL() Con_Builtins_Thread_Atom_end_blocking_call(thread)
#endif



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Exceptions
//

#define CON_TRY { \
	JMP_BUF _con_except_env; \
	if (setjmp(_con_except_env) == 0) { \
		{ \
			Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread); \
			Con_PC except_pc; \
			except_pc.type = PC_TYPE_NULL; \
			CON_MUTEX_LOCK(&con_stack->mutex); \
			Con_Builtins_Con_Stack_Atom_add_exception_frame(thread, con_stack, _con_except_env, except_pc); \
			CON_MUTEX_UNLOCK(&con_stack->mutex); \
		}

#define CON_CATCH(v) {\
			Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread); \
			CON_MUTEX_LOCK(&con_stack->mutex); \
			Con_Builtins_Con_Stack_Atom_remove_exception_frame(thread, con_stack); \
			CON_MUTEX_UNLOCK(&con_stack->mutex); \
		} \
	} else { \
		(v) = Con_Builtins_VM_Atom_get_current_exception(thread); \
		Con_Builtins_VM_Atom_reset_current_exception(thread); \

#define CON_TRY_END } }



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Atoms
//

#define CON_GET_ATOM(obj, type) ((obj)->first_atom->atom_type == (type) ? obj->first_atom : Con_Object_get_atom(thread, obj, type))
#define CON_FIND_ATOM(obj, type) ((obj)->first_atom->atom_type == (type) ? obj->first_atom : Con_Object_find_atom(thread, obj, type))



/////////////////////////////////////////////////////////////////////////////////////////////////////
// General
//

#define CON_BUILTIN(builtin_no) (((Con_Builtins_VM_Atom *) ((Con_Builtins_Thread_Atom *) \
	thread->first_atom)->vm->first_atom)->builtins[builtin_no])

#define CON_GET_SLOT(obj, slot_name) Con_Object_get_slot(thread, obj, NULL, (u_char *) slot_name, sizeof(slot_name) - 1)
#define CON_GET_SLOT_STR(obj, slot_name) Con_Object_get_slot(thread, obj, (u_char *) slot_name, NULL, 0)
#define CON_SET_SLOT(obj, slot_name, val) Con_Object_set_slot(thread, obj, NULL, (u_char *) slot_name, sizeof(slot_name) - 1, val)

#define CON_GET_MOD_DEFN(obj, definition_name) Con_Builtins_Module_Atom_get_defn(thread, obj, (u_char *) definition_name, sizeof(definition_name) - 1)
#define CON_SET_MOD_DEFN(obj, defn_name, val) Con_Builtins_Module_Atom_set_defn(thread, obj, (u_char *) defn_name, sizeof(defn_name) - 1, val)

#define CON_SET_FIELD(_class, field_name, val) Con_Builtins_Class_Atom_set_field(thread, _class, (u_char *) field_name, sizeof(field_name) - 1, val)

#ifdef __GNUC__
#	define CON_APPLY(func, ...) Con_Builtins_VM_Atom_apply(thread, func, false, ## __VA_ARGS__, NULL)
#	define CON_APPLY_NO_FAIL(func, ...) Con_Builtins_VM_Atom_apply(thread, func, true, ## __VA_ARGS__, NULL)

#	define CON_GET_SLOT_APPLY(obj, slot_name, ...) \
		Con_Builtins_VM_Atom_get_slot_apply(thread, obj, (u_char *) slot_name, sizeof(slot_name) - 1, false, ## __VA_ARGS__, NULL)
#	define CON_GET_SLOT_APPLY_NO_FAIL(obj, slot_name, ...) \
		Con_Builtins_VM_Atom_get_slot_apply(thread, obj, (u_char *) slot_name, sizeof(slot_name) - 1, true, ## __VA_ARGS__, NULL)

#	define CON_PRE_GET_SLOT_APPLY_PUMP(obj, slot_name, ...) \
		Con_Builtins_VM_Atom_pre_get_slot_apply_pump(thread, obj, (u_char *) slot_name, sizeof(slot_name) - 1, ## __VA_ARGS__, NULL)
#	define CON_APPLY_PUMP() Con_Builtins_VM_Atom_apply_pump(thread, false)

#	define CON_UNPACK_ARGS(args, ...) \
		Con_Builtins_Con_Stack_Atom_unpack_args(thread, Con_Builtins_Thread_Atom_get_con_stack(thread), args, ## __VA_ARGS__, NULL)

#	define CON_RAISE_EXCEPTION(name, ...) { \
		Con_Builtins_VM_Atom_ensure_no_current_exception(thread); \
		Con_Builtins_VM_Atom_raise(thread, \
			Con_Builtins_VM_Atom_get_slot_apply(thread, Con_Builtins_Module_Atom_get_defn(thread, CON_BUILTIN(CON_BUILTIN_EXCEPTIONS_MODULE), (u_char *) name, sizeof(name) - 1), \
			(u_char *) "new", sizeof("new") - 1, false, ## __VA_ARGS__, NULL)); \
		abort(); \
	}

#	define CON_PRINTLN(...) CON_APPLY(CON_GET_MOD_DEFN(CON_BUILTIN(CON_BUILTIN_SYS_MODULE), "println"), ## __VA_ARGS__, NULL)
#else
#	error "Unsupported compiler"
#endif

// CON_YIELD is a little odd. If the current continuation is being pumped for values, it uses yield
// to return values. If it is only being requested to produce a single value, it returns the object
// directly up the call chain.

#define CON_YIELD(obj) do { \
		Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread); \
		CON_MUTEX_LOCK(&con_stack->mutex); \
		bool return_as_generator = Con_Builtins_Con_Stack_Atom_continuation_return_as_generator(thread, con_stack); \
		CON_MUTEX_UNLOCK(&con_stack->mutex); \
		if (return_as_generator) \
			Con_Builtins_VM_Atom_yield(thread, obj); \
		else \
			return obj; \
	} while (0)

#define CON_EXBI(_class, slot_name, self) \
	Con_Builtins_VM_Atom_exbi(thread, _class, NULL, (u_char *) slot_name, sizeof(slot_name) - 1, self)


#define CON_NEW_BOUND_C_FUNC(f, n, module, container) Con_Builtins_Func_Atom_new(thread, true, Con_Builtins_Func_Atom_make_con_pc_c_function(thread, module, f), -1, 0, NULL, CON_NEW_STRING(n), container)
#define CON_NEW_UNBOUND_C_FUNC(f, n, module) Con_Builtins_Func_Atom_new(thread, false, Con_Builtins_Func_Atom_make_con_pc_c_function(thread, module, f), -1, 0, NULL, CON_NEW_STRING(n), module)

#define CON_NEW_STRING(s) Con_Builtins_String_Atom_new_no_copy(thread, (u_char *) s, sizeof(s) - 1, CON_STR_UTF_8)
#define CON_NEW_INT(i) Con_Builtins_Int_Atom_new(thread, i)

#define CON_ADD(x, y) Con_Object_add(thread, x, y)
#define CON_SUBTRACT(x, y) Con_Object_subtract(thread, x, y)

#define CON_C_STRING_EQ(c_string, string_obj) Con_Builtins_String_Atom_c_string_eq(thread, (u_char *) c_string, sizeof(c_string) - 1, string_obj)
