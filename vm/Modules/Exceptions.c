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

#include "Core.h"
#include "Modules.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Exception/Atom.h"
#include "Builtins/Exception/Class.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *Con_Module_Exceptions_init(Con_Obj *, Con_Obj *);
Con_Obj *Con_Module_Exceptions_import(Con_Obj *, Con_Obj *);

Con_Obj *_Con_Module_Exception_System_Exit_Exception_init_func(Con_Obj *);
Con_Obj *_Con_Module_Exception_VM_Exception_init_func(Con_Obj *);

Con_Obj *_Con_Module_Exception_Apply_Exception_init_func(Con_Obj *);
Con_Obj *_Con_Module_Exception_Bounds_Exception_init_func(Con_Obj *);
Con_Obj *_Con_Module_Exception_Mod_Defn_Exception_init_func(Con_Obj *);
Con_Obj *_Con_Module_Exception_Field_Exception_init_func(Con_Obj *);
Con_Obj *_Con_Module_Exception_Import_Exception_init_func(Con_Obj *);
Con_Obj *_Con_Module_Exception_Indices_Exception_init_func(Con_Obj *);
Con_Obj *_Con_Module_Exception_Key_Exception_init_func(Con_Obj *);
Con_Obj *_Con_Module_Exception_Number_Exception_init_func(Con_Obj *);
Con_Obj *_Con_Module_Exception_Slot_Exception_init_func(Con_Obj *);
Con_Obj *_Con_Module_Exception_Type_Exception_init_func(Con_Obj *);
Con_Obj *_Con_Module_Exception_Unpack_Exception_init_func(Con_Obj *);

Con_Obj *_Con_Module_Exception_backtrace_func(Con_Obj *);



Con_Obj *Con_Module_Exceptions_init(Con_Obj *thread, Con_Obj *identifier)
{
	const char* defn_names[] = {"Exception", "Internal_Exception", "User_Exception", "System_Exit_Exception", "VM_Exception", "Apply_Exception", "Assert_Exception", "Bounds_Exception", "Field_Exception", "Import_Exception", "Indices_Exception", "Key_Exception", "Mod_Defn_Exception", "NDIf_Exception", "Number_Exception", "Parameters_Exception", "Signal_Exception", "Slot_Exception", "Type_Exception", "Unpack_Exception", "Unassigned_Var_Exception", "IO_Exception", "File_Exception", "backtrace", NULL};

	return Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("Exceptions"), defn_names, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



Con_Obj *Con_Module_Exceptions_import(Con_Obj *thread, Con_Obj *exceptions_mod)
{
	// First we setup the main Exception class, and its "category" subclasses.

	CON_SET_MOD_DEFN(exceptions_mod, "Exception", CON_BUILTIN(CON_BUILTIN_EXCEPTION_CLASS));

	// class Internal_Exception

	Con_Obj *internal_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Internal_Exception"), Con_Builtins_List_Atom_new_va(thread, CON_BUILTIN(CON_BUILTIN_EXCEPTION_CLASS), NULL), exceptions_mod);
	CON_SET_MOD_DEFN(exceptions_mod, "Internal_Exception", internal_exception);

	// class User_Exception

	Con_Obj *user_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("User_Exception"), Con_Builtins_List_Atom_new_va(thread, CON_BUILTIN(CON_BUILTIN_EXCEPTION_CLASS), NULL), exceptions_mod);
	CON_SET_MOD_DEFN(exceptions_mod, "User_Exception", user_exception);

	// Setup internal exceptions.

	// class System_Exit_Exception

	Con_Obj *system_exit_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("System_Exit_Exception"), Con_Builtins_List_Atom_new_va(thread, internal_exception, NULL), exceptions_mod);
	CON_SET_FIELD(system_exit_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_System_Exit_Exception_init_func, "init", exceptions_mod, system_exit_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "System_Exit_Exception", system_exit_exception);

	// class VM_Exception

	Con_Obj *vm_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("VM_Exception"), Con_Builtins_List_Atom_new_va(thread, internal_exception, NULL), exceptions_mod);
	CON_SET_FIELD(vm_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_VM_Exception_init_func, "init", exceptions_mod, vm_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "VM_Exception", vm_exception);

	// Setup the standard user exceptions.

	// class Apply_Exception

	Con_Obj *apply_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Apply_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_FIELD(apply_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_Apply_Exception_init_func, "init", exceptions_mod, apply_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "Apply_Exception", apply_exception);

	// class Assert_Exception
	
	Con_Obj *assert_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Assert_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_MOD_DEFN(exceptions_mod, "Assert_Exception", assert_exception);

	// class Bounds_Exception

	Con_Obj *bounds_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Bounds_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_FIELD(bounds_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_Bounds_Exception_init_func, "init", exceptions_mod, bounds_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "Bounds_Exception", bounds_exception);

	// class Field_Exception

	Con_Obj *field_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Field_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_FIELD(field_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_Field_Exception_init_func, "init", exceptions_mod, field_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "Field_Exception", field_exception);

	// class Import_Exception

	Con_Obj *import_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Import_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_FIELD(import_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_Import_Exception_init_func, "init", exceptions_mod, import_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "Import_Exception", import_exception);

	// class Indices_Exception

	Con_Obj *indices_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Indices_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_FIELD(indices_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_Indices_Exception_init_func, "init", exceptions_mod, indices_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "Indices_Exception", indices_exception);

	// class Key_Exception

	Con_Obj *key_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Key_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_FIELD(key_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_Key_Exception_init_func, "init", exceptions_mod, key_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "Key_Exception", key_exception);

	// class Mod_Defn_Exception

	Con_Obj *defn_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Mod_Defn_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_FIELD(defn_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_Mod_Defn_Exception_init_func, "init", exceptions_mod, defn_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "Mod_Defn_Exception", defn_exception);

	// class NDIf_Exception
	
	Con_Obj *ndif_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("NDIf_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_MOD_DEFN(exceptions_mod, "NDIf_Exception", ndif_exception);

	// class Number_Exception

	Con_Obj *number_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Number_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_FIELD(number_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_Number_Exception_init_func, "init", exceptions_mod, number_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "Number_Exception", number_exception);

	// class Parameters_Exception

	Con_Obj *parameters_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Parameters_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_MOD_DEFN(exceptions_mod, "Parameters_Exception", parameters_exception);

	// class Signal_Exception
	
	Con_Obj *signal_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Signal_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_MOD_DEFN(exceptions_mod, "Signal_Exception", signal_exception);

	// class Slot_Exception

	Con_Obj *slot_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Slot_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_FIELD(slot_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_Slot_Exception_init_func, "init", exceptions_mod, slot_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "Slot_Exception", slot_exception);

	// class Type_Exception
	
	Con_Obj *type_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Type_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_FIELD(type_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_Type_Exception_init_func, "init", exceptions_mod, type_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "Type_Exception", type_exception);

	// class Unpack_Exception
	
	Con_Obj *unpack_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Unpack_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_FIELD(unpack_exception, "init", CON_NEW_BOUND_C_FUNC(_Con_Module_Exception_Unpack_Exception_init_func, "init", exceptions_mod, unpack_exception));
	CON_SET_MOD_DEFN(exceptions_mod, "Unpack_Exception", unpack_exception);

	// class Unassigned_Var_Exception
	
	Con_Obj *unassigned_var_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Unassigned_Var_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_MOD_DEFN(exceptions_mod, "Unassigned_Var_Exception", unassigned_var_exception);
	
	// IO exceptions
	
	// class IO_Exception
	
	Con_Obj *io_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("IO_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), exceptions_mod);
	CON_SET_MOD_DEFN(exceptions_mod, "IO_Exception", io_exception);

	// class File_Exception
	
	Con_Obj *file_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("File_Exception"), Con_Builtins_List_Atom_new_va(thread, io_exception, NULL), exceptions_mod);
	CON_SET_MOD_DEFN(exceptions_mod, "File_Exception", file_exception);
	
	// func backtrace
	
	CON_SET_MOD_DEFN(exceptions_mod, "backtrace", CON_NEW_UNBOUND_C_FUNC(_Con_Module_Exception_backtrace_func, "backtrace", exceptions_mod));
	
	return exceptions_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in Exception module
//

//
// Internal exceptions
//

//
// 'System_Exit_Exception.init(func)'.
//

Con_Obj *_Con_Module_Exception_System_Exit_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *code_obj, *self;
	CON_UNPACK_ARGS("OO", &self, &code_obj);

	CON_SET_SLOT(self, "msg", CON_NEW_STRING("<System attempting to exit>."));
	CON_SET_SLOT(self, "code", code_obj);

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'VM_Exception.init(func)'.
//

Con_Obj *_Con_Module_Exception_VM_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *msg, *self;
	CON_UNPACK_ARGS("OO", &self, &msg);

	CON_SET_SLOT(self, "msg", msg);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// User exceptions
//

//
// 'Apply_Exception.init(func)'.
//

Con_Obj *_Con_Module_Exception_Apply_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *self, *obj;
	CON_UNPACK_ARGS("OO", &self, &obj);

	Con_Obj *msg = CON_NEW_STRING("Do not know how to apply instance of '");
	msg = CON_ADD(msg, CON_GET_SLOT(CON_GET_SLOT(obj, "instance_of"), "name"));
	msg = CON_ADD(msg, CON_NEW_STRING("'."));
	
	CON_SET_SLOT(self, "msg", msg);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'Bounds_Exception.init(i, upper_bound)'.
//

Con_Obj *_Con_Module_Exception_Bounds_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *index, *self, *upper_bound;
	CON_UNPACK_ARGS("OOO", &self, &index, &upper_bound);

	Con_Obj *msg = CON_GET_SLOT_APPLY(index, "to_str");
	if (CON_GET_SLOT_APPLY_NO_FAIL(index, "<", CON_NEW_INT(0)) != NULL) {
		msg = CON_ADD(msg, CON_NEW_STRING(" below lower bound 0"));
	}
	else {
		msg = CON_ADD(msg, CON_NEW_STRING(" exceeds upper bound "));
		msg = CON_ADD(msg, CON_GET_SLOT_APPLY(upper_bound, "to_str"));
	}
	
	CON_SET_SLOT(self, "msg", msg);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'Field_Exception.init(name, o)'.
//

Con_Obj *_Con_Module_Exception_Field_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *self, *field_name, *class_;
	CON_UNPACK_ARGS("OOO", &self, &field_name, &class_);

	Con_Obj *msg = CON_NEW_STRING("No such field '");
	msg = CON_ADD(msg, field_name);
	msg = CON_ADD(msg, CON_NEW_STRING("' in class '"));
	msg = CON_ADD(msg, CON_GET_SLOT(class_, "name"));
	msg = CON_ADD(msg, CON_NEW_STRING("'."));
	
	CON_SET_SLOT(self, "msg", msg);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'Import_Exception.init(identifier)'.
//

Con_Obj *_Con_Module_Exception_Import_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *identifier, *self;
	CON_UNPACK_ARGS("OO", &self, &identifier);

	Con_Obj *msg = CON_NEW_STRING("Unable to import '");
	msg = CON_ADD(msg, identifier);
	msg = CON_ADD(msg, CON_NEW_STRING("'."));
	
	CON_SET_SLOT(self, "msg", msg);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'Indices_Exception.init(lower, upper)'.
//

Con_Obj *_Con_Module_Exception_Indices_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *lower_bound, *self, *upper_bound;
	CON_UNPACK_ARGS("OOO", &self, &lower_bound, &upper_bound);

	Con_Obj *msg = CON_NEW_STRING("Lower bound ");
	msg = CON_ADD(msg, CON_GET_SLOT_APPLY(lower_bound, "to_str"));
	msg = CON_ADD(msg, CON_NEW_STRING(" exceeds upper bound "));
	msg = CON_ADD(msg, CON_GET_SLOT_APPLY(upper_bound, "to_str"));
	msg = CON_ADD(msg, CON_NEW_STRING("."));
	
	CON_SET_SLOT(self, "msg", msg);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'Key_Exception.init(key)'.
//

Con_Obj *_Con_Module_Exception_Key_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *key, *self;
	CON_UNPACK_ARGS("OO", &self, &key);

	Con_Obj *msg = CON_NEW_STRING("Key '");
	msg = CON_ADD(msg, CON_GET_SLOT_APPLY(key, "to_str"));
	msg = CON_ADD(msg, CON_NEW_STRING("' not found."));
	
	CON_SET_SLOT(self, "msg", msg);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'Mod_Defn_Exception.init(key)'.
//

Con_Obj *_Con_Module_Exception_Mod_Defn_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *key, *mod, *self;
	CON_UNPACK_ARGS("OOO", &self, &key, &mod);

	Con_Obj *msg = CON_NEW_STRING("Definition '");
	msg = CON_ADD(msg, key);
	msg = CON_ADD(msg, CON_NEW_STRING("' not found in '"));
	msg = CON_ADD(msg, CON_GET_SLOT(mod, "name"));
	msg = CON_ADD(msg, CON_NEW_STRING("'."));
	
	CON_SET_SLOT(self, "msg", msg);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'Number_Exception.init(num)'.
//

Con_Obj *_Con_Module_Exception_Number_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *num, *self;
	CON_UNPACK_ARGS("OO", &self, &num);

	Con_Obj *msg = CON_NEW_STRING("Number '");
	msg = CON_ADD(msg, CON_GET_SLOT_APPLY(num, "to_str"));
	msg = CON_ADD(msg, CON_NEW_STRING("' not valid."));
	
	CON_SET_SLOT(self, "msg", msg);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'Slot_Exception.init(name, o)'.
//

Con_Obj *_Con_Module_Exception_Slot_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *self, *slot_name, *obj;
	CON_UNPACK_ARGS("OOO", &self, &slot_name, &obj);

	Con_Obj *msg = CON_NEW_STRING("No such slot '");
	msg = CON_ADD(msg, slot_name);
	msg = CON_ADD(msg, CON_NEW_STRING("' in instance of '"));
	msg = CON_ADD(msg, CON_GET_SLOT(CON_GET_SLOT(obj, "instance_of"), "name"));
	msg = CON_ADD(msg, CON_NEW_STRING("'."));
	
	CON_SET_SLOT(self, "msg", msg);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'Type_Exception.init(should_be, obj, extra := null)'.
//

Con_Obj *_Con_Module_Exception_Type_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *extra, *o, *self, *should_be;
	CON_UNPACK_ARGS("OOO;O", &self, &should_be, &o, &extra);

	Con_Obj *msg = CON_NEW_STRING("Expected ");
	if (extra != NULL) {
		msg = CON_ADD(msg, extra);
		msg = CON_ADD(msg, CON_NEW_STRING(" to be conformant to "));
	}
	else
		msg = CON_ADD(msg, CON_NEW_STRING("to be conformant to "));
	msg = CON_ADD(msg, should_be);
	msg = CON_ADD(msg, CON_NEW_STRING(" but got instance of "));
	msg = CON_ADD(msg, CON_GET_SLOT_APPLY(CON_GET_SLOT(o, "instance_of"), "path", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)));
	msg = CON_ADD(msg, CON_NEW_STRING("."));
	
	CON_SET_SLOT(self, "msg", msg);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'Unpack_Exception.init(num_elements_expected, num_elements_got)'.
//

Con_Obj *_Con_Module_Exception_Unpack_Exception_init_func(Con_Obj *thread)
{
	Con_Obj *num_elements_expected, *num_elements_got, *self;
	CON_UNPACK_ARGS("OOO", &self, &num_elements_expected, &num_elements_got);

	Con_Obj *msg = CON_NEW_STRING("Unpack of ");
	msg = CON_ADD(msg, CON_GET_SLOT_APPLY(num_elements_expected, "to_str"));
	msg = CON_ADD(msg, CON_NEW_STRING(" elements failed as "));
	msg = CON_ADD(msg, CON_GET_SLOT_APPLY(num_elements_got, "to_str"));
	msg = CON_ADD(msg, CON_NEW_STRING(" elements present."));
	
	CON_SET_SLOT(self, "msg", msg);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'backtrace(exception)' returns a string representation of 'exception', including a backtrace.
//
//

Con_Obj *_Con_Module_Exception_backtrace_func(Con_Obj *thread)
{
	Con_Obj *exception;
	CON_UNPACK_ARGS("E", &exception);
	
	Con_Builtins_Exception_Atom *exception_atom = CON_FIND_ATOM(exception, CON_BUILTIN(CON_BUILTIN_EXCEPTION_ATOM_DEF_OBJECT));

	// The backtrace that this function creates really consists of a backtrace followed by appending
	// exception.to_str.
	
	// So first create the backtrace.

#	ifdef CON_THREADS_SINGLE_THREADED
	Con_Obj *output = CON_NEW_STRING("Traceback (most recent call at bottom):\n");
#	else
	// In a multi-threaded world, we have to be very clear about which exception raised the thread
	// that is being reported.
	
	Con_Obj *output = CON_NEW_STRING("Traceback from exception raised in thread ");
	CON_MUTEX_LOCK(&exception->mutex);
	Con_Obj *exception_thread = exception_atom->exception_thread;
	CON_MUTEX_UNLOCK(&exception->mutex);
	output = CON_ADD(output, CON_GET_SLOT_APPLY(exception_thread, "to_str"));
	output = CON_ADD(output, CON_NEW_STRING(" (most recent call at bottom):\n"));
#endif

	CON_MUTEX_LOCK(&exception->mutex);

	// Iterate over the call chain in reverse order, printing out each entry.
	for (Con_Int i = exception_atom->num_call_chain_entries - 1; i >= 0; i -= 1) {
		Con_Builtins_Exception_Class_Call_Chain_Entry *call_chain_entry = &exception_atom->call_chain[i];
		Con_Obj *func = call_chain_entry->func;
		Con_PC pc = call_chain_entry->pc;
		Con_Int num_call_chain_entries = exception_atom->num_call_chain_entries;
		CON_MUTEX_UNLOCK(&exception->mutex);
		Con_Obj *entry = CON_NEW_STRING("  ");
		entry = CON_ADD(entry, CON_GET_SLOT_APPLY(CON_NEW_INT(num_call_chain_entries - i), "to_str"));
		entry = CON_ADD(entry, CON_NEW_STRING(": "));
		if (pc.type == PC_TYPE_C_FUNCTION) {
			// When the PC is a C function, we can really only print out the function's path.
			
			entry = CON_ADD(entry, CON_NEW_STRING("(internal), in "));
			entry = CON_ADD(entry, CON_GET_SLOT_APPLY(func, "path", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)));
			entry = CON_ADD(entry, CON_NEW_STRING("\n"));
		}
		else if (pc.type == PC_TYPE_BYTECODE) {
			// When the PC points to a bytecode location, we can be much more sophisticated. A single
			// bytecode location points to one or more source code locations. Because of this we
			// format the output as e.g.:
			//
			// Traceback (most recent call at bottom):
			//   1: File "/home/ltratt/src/converge/test/t1.cv", line 32, column 3, in main
			//   2: File "/home/ltratt/src/converge/test/mri.cv", line 5, column 3, in f
			//      File "/home/ltratt/src/converge/test/t1.cv", line 5, column 3, in f
			//   3: File "/home/ltratt/src/converge/test/t1.cv", line 8, column 5, in g
			//   4: get_slot (internal)
			//   5: get_slot (internal)
			// Exception: Unknown slot 'w'.
		
			Con_Obj *src_locations = Con_Builtins_Module_Atom_pc_to_src_locations(thread, pc);
			
			int j = 0;
			CON_PRE_GET_SLOT_APPLY_PUMP(src_locations, "iter");
			while (true) {
				Con_Obj *src_location = CON_APPLY_PUMP();
				if (src_location == NULL)
					break;
					
				if (j > 0) {
					entry = CON_ADD(entry, CON_NEW_STRING("     "));
					entry = CON_ADD(entry, CON_GET_SLOT_APPLY(CON_NEW_STRING(" "), "*", CON_NEW_INT(j / 10)));
				}
					
				Con_Obj *mod_id = CON_GET_SLOT_APPLY(src_location, "get", CON_NEW_INT(0));
				Con_Obj *src_offset = CON_GET_SLOT_APPLY(src_location, "get", CON_NEW_INT(1));
				
				Con_Obj *mod = Con_Modules_get(thread, mod_id);
				Con_Obj *src_path = CON_GET_SLOT(mod, "src_path");

				Con_Int line, column;
				Con_Builtins_Module_Atom_src_offset_to_line_column(thread, mod, Con_Numbers_Number_to_Con_Int(thread, src_offset), &line, &column);
				entry = CON_ADD(entry, CON_NEW_STRING("File \""));
				entry = CON_ADD(entry, src_path);
				entry = CON_ADD(entry, CON_NEW_STRING("\", line "));
				entry = CON_ADD(entry, CON_GET_SLOT_APPLY(CON_NEW_INT(line), "to_str"));
				entry = CON_ADD(entry, CON_NEW_STRING(", column "));
				entry = CON_ADD(entry, CON_GET_SLOT_APPLY(CON_NEW_INT(column), "to_str"));
				// These next two lines are commented out because there's not always a direct link
				// between a src_info and the executing information. For the time being, rather than
				// sometimes printing out incorrect function names, we simply avoid printing
				// anything.
				//entry = CON_ADD(entry, CON_NEW_STRING(", in "));
				//entry = CON_ADD(entry, CON_GET_SLOT_APPLY(func, "path", pc.module));
				entry = CON_ADD(entry, CON_NEW_STRING("\n"));
				
				j += 1;
			}
		}
		else
			CON_XXX;

		output = CON_ADD(output, entry);
		
		CON_MUTEX_LOCK(&exception->mutex);
	}

	CON_MUTEX_UNLOCK(&exception->mutex);
	
	// Append 'exception.to_str' to the backtrace.
	
	output = CON_ADD(output, CON_GET_SLOT_APPLY(exception, "to_str"));
	
	return output;
}
