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

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"

#include "Modules/Sys.h"



Con_Obj *_Con_Modules_Sys_exit_func(Con_Obj *);
Con_Obj *_Con_Modules_Sys_println_func(Con_Obj *);



Con_Obj *Con_Modules_Sys_init(Con_Obj *thread, Con_Obj *identifier)
{
	Con_Obj *sys_mod = Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("Sys"), CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
	
	CON_SET_SLOT(sys_mod, "exit", CON_NEW_UNBOUND_C_FUNC(_Con_Modules_Sys_exit_func, "exit", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)));
	CON_SET_SLOT(sys_mod, "println", CON_NEW_UNBOUND_C_FUNC(_Con_Modules_Sys_println_func, "get", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)));
	
	// Setup stdin, stdout, and stderr.
	
	CON_SET_SLOT(sys_mod, "stdin", CON_GET_SLOT_APPLY(CON_GET_MODULE_DEF(CON_BUILTIN(CON_BUILTIN_C_FILE_MODULE), "File"), "new", CON_NEW_INT(STDIN_FILENO), CON_NEW_STRING("r")));
	CON_SET_SLOT(sys_mod, "stdout", CON_GET_SLOT_APPLY(CON_GET_MODULE_DEF(CON_BUILTIN(CON_BUILTIN_C_FILE_MODULE), "File"), "new", CON_NEW_INT(STDOUT_FILENO), CON_NEW_STRING("w")));
	CON_SET_SLOT(sys_mod, "stderr", CON_GET_SLOT_APPLY(CON_GET_MODULE_DEF(CON_BUILTIN(CON_BUILTIN_C_FILE_MODULE), "File"), "new", CON_NEW_INT(STDERR_FILENO), CON_NEW_STRING("w")));
	
	// Do the necessary fiddlings with command line arguments.
	
	int argc;
	char **argv;
	Con_Builtins_VM_Atom_read_prog_args(thread, &argc, &argv);
	
	if (argc >= 2) {
		// Read the paths of the VM and the Converge program from the command line arguments. To
		// avoid potential problems we convert these into absolute pathnames straight away.
	
		char *vm_path = Con_Memory_malloc(thread, MAXPATHLEN, CON_MEMORY_CHUNK_OPAQUE);
		if (realpath(argv[0], vm_path) == NULL)
			CON_SET_SLOT(sys_mod, "vm_path", CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
		else
			CON_SET_SLOT(sys_mod, "vm_path", Con_Builtins_String_Atom_new_no_copy(thread, vm_path, strlen(vm_path), CON_STR_UTF_8));
			
		char *program_path = Con_Memory_malloc(thread, MAXPATHLEN, CON_MEMORY_CHUNK_OPAQUE);
		if (realpath(argv[1], program_path) == NULL)
			CON_SET_SLOT(sys_mod, "program_path", CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
		else
			CON_SET_SLOT(sys_mod, "program_path", Con_Builtins_String_Atom_new_no_copy(thread, program_path, strlen(program_path), CON_STR_UTF_8));
	}
	else
		CON_XXX;
	
	// The command line arguments that users see do not include the VM or Converge program.
	
	Con_Obj *argv_list = Con_Builtins_List_Atom_new_sized(thread, argc - 2);
	for (int i = 2; i < argc; i += 1) {
		CON_GET_SLOT_APPLY(argv_list, "append", Con_Builtins_String_Atom_new_no_copy(thread, argv[i], strlen(argv[i]), CON_STR_UTF_8));
	}
	CON_SET_SLOT(sys_mod, "argv", argv_list);
	
	return sys_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in Sys module
//


//
// 'exit(code)' causes the VM to exit with code 'code'.
//

Con_Obj *_Con_Modules_Sys_exit_func(Con_Obj *thread)
{
	Con_Obj *code_obj;
	CON_UNPACK_ARGS("O", &code_obj);
	
	CON_RAISE_EXCEPTION("System_Exit_Exception", code_obj);
}



Con_Obj *_Con_Modules_Sys_println_func(Con_Obj *thread)
{
	Con_Obj *var_args;
	CON_UNPACK_ARGS("v", &var_args);

	CON_PRE_GET_SLOT_APPLY_PUMP(var_args, "iterate");
	while (1) {
		Con_Obj *val = CON_APPLY_PUMP();
		if (val == NULL)
			break;
		Con_Builtins_String_Atom *val_string_atom = CON_FIND_ATOM(val, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		if (val_string_atom == NULL) {
			val_string_atom = CON_GET_ATOM(CON_GET_SLOT_APPLY(val, "to_str"), CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		}
		Con_Int i = 0;
		while (i < val_string_atom->size) {
			Con_Int j = i;
			while ((j < val_string_atom->size) && (*(val_string_atom->str + j) != '\0'))
				j += 1;
				
			printf("%.*s", j - i, val_string_atom->str + i);
			if (j == val_string_atom->size)
				break;
			else {
				// We've hit a NUL.
				printf("\\0");
				i = j + 1;
			}
		}
	}
	printf("\n");

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}
