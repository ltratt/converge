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

#include <sys/param.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Core.h"
#include "Memory.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"
#include "Version.h"

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *Con_Module_Sys_init(Con_Obj *, Con_Obj *);
Con_Obj *Con_Module_Sys_import(Con_Obj *, Con_Obj *);

Con_Obj *_Con_Module_Sys_exit_func(Con_Obj *);
Con_Obj *_Con_Module_Sys_print_func(Con_Obj *);
Con_Obj *_Con_Module_Sys_println_func(Con_Obj *);



Con_Obj *Con_Module_Sys_init(Con_Obj *thread, Con_Obj *identifier)
{
	const char* defn_names[] = {"print", "println", "stdin", "stdout", "stderr", "vm_path", "program_path", "argv", "version", "exit", NULL};

	return Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("Sys"), defn_names, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



Con_Obj *Con_Module_Sys_import(Con_Obj *thread, Con_Obj *sys_mod)
{
	CON_SET_MOD_DEFN(sys_mod, "exit", CON_NEW_UNBOUND_C_FUNC(_Con_Module_Sys_exit_func, "exit", sys_mod));
	CON_SET_MOD_DEFN(sys_mod, "print", CON_NEW_UNBOUND_C_FUNC(_Con_Module_Sys_print_func, "print", sys_mod));
	CON_SET_MOD_DEFN(sys_mod, "println", CON_NEW_UNBOUND_C_FUNC(_Con_Module_Sys_println_func, "println", sys_mod));
	
	// Setup stdin, stdout, and stderr.
	
	CON_SET_MOD_DEFN(sys_mod, "stdin", CON_GET_SLOT_APPLY(CON_GET_MOD_DEFN(CON_BUILTIN(CON_BUILTIN_C_FILE_MODULE), "File"), "new", CON_NEW_INT(STDIN_FILENO), CON_NEW_STRING("r")));
	CON_SET_MOD_DEFN(sys_mod, "stdout", CON_GET_SLOT_APPLY(CON_GET_MOD_DEFN(CON_BUILTIN(CON_BUILTIN_C_FILE_MODULE), "File"), "new", CON_NEW_INT(STDOUT_FILENO), CON_NEW_STRING("w")));
	CON_SET_MOD_DEFN(sys_mod, "stderr", CON_GET_SLOT_APPLY(CON_GET_MOD_DEFN(CON_BUILTIN(CON_BUILTIN_C_FILE_MODULE), "File"), "new", CON_NEW_INT(STDERR_FILENO), CON_NEW_STRING("w")));

	// Read the path of the VM and the bytecode path.

	char *vm_path, *prog_path;
	Con_Builtins_VM_Atom_get_paths(thread, &vm_path, &prog_path);
	if (vm_path == NULL)
		CON_SET_MOD_DEFN(sys_mod, "vm_path", CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
	else
		CON_SET_MOD_DEFN(sys_mod, "vm_path", Con_Builtins_String_Atom_new_no_copy(thread, (u_char *) vm_path, strlen(vm_path), CON_STR_UTF_8));

	if (prog_path == NULL)
		CON_SET_MOD_DEFN(sys_mod, "program_path", CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
	else
		CON_SET_MOD_DEFN(sys_mod, "program_path", Con_Builtins_String_Atom_new_no_copy(thread, (u_char *) prog_path, strlen(prog_path), CON_STR_UTF_8));

	// Do the necessary fiddlings with command line arguments.
	
	int argc;
	char **argv;
	Con_Builtins_VM_Atom_read_prog_args(thread, &argc, &argv);
	
	// The command line arguments that users see do not include the VM or Converge program.
	
	Con_Obj *argv_list = Con_Builtins_List_Atom_new_sized(thread, argc - 2);
	for (int i = 2; i < argc; i += 1) {
		CON_GET_SLOT_APPLY(argv_list, "append", Con_Builtins_String_Atom_new_no_copy(thread, (u_char *) argv[i], strlen(argv[i]), CON_STR_UTF_8));
	}
	CON_SET_MOD_DEFN(sys_mod, "argv", argv_list);
	
	// Version info.

	CON_SET_MOD_DEFN(sys_mod, "version", CON_NEW_STRING(CON_VERSION));
	
	return sys_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in Sys module
//


//
// 'exit(code)' causes the VM to exit with code 'code'.
//

Con_Obj *_Con_Module_Sys_exit_func(Con_Obj *thread)
{
	Con_Obj *code_obj;
	CON_UNPACK_ARGS("O", &code_obj);
	
	CON_RAISE_EXCEPTION("System_Exit_Exception", code_obj);
}



Con_Obj *_Con_Module_Sys_print_func(Con_Obj *thread)
{
	Con_Obj *sys_mod = Con_Builtins_VM_Atom_get_functions_module(thread);

	Con_Obj *var_args;
	CON_UNPACK_ARGS("v", &var_args);

	Con_Obj *write_func = CON_GET_SLOT(CON_GET_MOD_DEFN(sys_mod, "stdout"), "write");

	CON_PRE_GET_SLOT_APPLY_PUMP(var_args, "iter");
	while (1) {
		Con_Obj *str = CON_APPLY_PUMP();
		if (str == NULL)
			break;

		if (CON_GET_SLOT_APPLY_NO_FAIL(CON_BUILTIN(CON_BUILTIN_STRING_CLASS), "instantiated", str) == NULL)
			str = CON_GET_SLOT_APPLY(str, "to_str");
		CON_APPLY(write_func, str);
	}

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}




Con_Obj *_Con_Module_Sys_println_func(Con_Obj *thread)
{
	Con_Obj *sys_mod = Con_Builtins_VM_Atom_get_functions_module(thread);

	Con_Obj *var_args;
	CON_UNPACK_ARGS("v", &var_args);

	Con_Obj *write_func = CON_GET_SLOT(CON_GET_MOD_DEFN(sys_mod, "stdout"), "write");

	CON_PRE_GET_SLOT_APPLY_PUMP(var_args, "iter");
	while (1) {
		Con_Obj *str = CON_APPLY_PUMP();
		if (str == NULL)
			break;

		if (CON_GET_SLOT_APPLY_NO_FAIL(CON_BUILTIN(CON_BUILTIN_STRING_CLASS), "instantiated", str) == NULL)
			str = CON_GET_SLOT_APPLY(str, "to_str");
		CON_APPLY(write_func, str);
	}
	CON_APPLY(write_func, CON_NEW_STRING("\n"));

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}
