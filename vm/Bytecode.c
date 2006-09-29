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

#include <string.h>

#include "Arch.h"
#include "Bytecode.h"
#include "Core.h"
#include "Memory.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Dict/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



//
// Import an exec file from memory. bytecode should be a chunk of memory that will not be written to
// and which will not be freed until after this function has completed. Note that holding a lock on
// a segment while calling this function may lead to deadlock.
//

Con_Obj *Con_Bytecode_add_executable(Con_Obj *thread, u_char *bytecode)
{
	int i, num_modules, module_offset;
	Con_Obj *main_module_identifier;

#	define ID_BYTECODE_GET_WORD(x) (*(Con_Int*) (bytecode + (x)))

	num_modules = ID_BYTECODE_GET_WORD(CON_BYTECODE_NUMBER_OF_MODULES);
	
	main_module_identifier = NULL;
	for (i = 0; i < num_modules; i += 1) {
		module_offset = ID_BYTECODE_GET_WORD(CON_BYTECODE_MODULES + i * sizeof(Con_Int));

		Con_Obj *identifier = Con_Bytecode_add_module(thread, bytecode + module_offset);

		if (i == 0)
			main_module_identifier = identifier;
	}
	
	if (main_module_identifier == NULL)
		CON_XXX;

	return main_module_identifier;
}



Con_Obj *Con_Bytecode_add_module(Con_Obj *thread, u_char *bytecode)
{
	int j, imports_bytecode_offset;
	Con_Obj *identifier, *imports, *init_func, *module, *name;

#	define ID_MODULE_GET_WORD(x) (*(Con_Int*) (bytecode + (x)))
#	define ID_MODULE_GET_OFFSET(x) ((void *) (bytecode + (ID_MODULE_GET_WORD(x))))

	name = Con_Builtins_String_Atom_new_copy(thread, ID_MODULE_GET_OFFSET(CON_BYTECODE_MODULE_NAME), ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NAME_SIZE), CON_STR_UTF_8);

	identifier = Con_Builtins_String_Atom_new_copy(thread, ID_MODULE_GET_OFFSET(CON_BYTECODE_MODULE_IDENTIFIER), ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_IDENTIFIER_SIZE), CON_STR_UTF_8);

	imports = Con_Builtins_List_Atom_new_sized(thread, ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_IMPORTS));
	imports_bytecode_offset = ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_IMPORTS);
	for (j = 0; j < ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_IMPORTS); j += 1) {
		CON_GET_SLOT_APPLY(imports, "append", Con_Builtins_String_Atom_new_copy(thread, (char *) (bytecode + imports_bytecode_offset + CON_BYTECODE_IMPORT_IDENTIFIER), ID_MODULE_GET_WORD(imports_bytecode_offset + CON_BYTECODE_IMPORT_IDENTIFIER_SIZE), CON_STR_UTF_8));
		imports_bytecode_offset += CON_BYTECODE_IMPORT_IDENTIFIER + Con_Arch_align(thread, ID_MODULE_GET_WORD(imports_bytecode_offset + CON_BYTECODE_IMPORT_IDENTIFIER_SIZE));
	}

	Con_Obj *top_level_vars_map = Con_Builtins_Dict_Class_new(thread);
	Con_Int top_level_vars_map_bytecode_offset = ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_TOP_LEVEL_VARS_MAP);
	for (j = 0; j < ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_TOP_LEVEL_VARS); j += 1) {
		Con_Obj *var_name = Con_Builtins_String_Atom_new_copy(thread, (char *) (bytecode + top_level_vars_map_bytecode_offset + CON_BYTECODE_TOP_LEVEL_VAR_NAME), ID_MODULE_GET_WORD(top_level_vars_map_bytecode_offset + CON_BYTECODE_TOP_LEVEL_VAR_NAME_SIZE), CON_STR_UTF_8);
		CON_GET_SLOT_APPLY(top_level_vars_map, "set", var_name, CON_NEW_INT(ID_MODULE_GET_WORD(top_level_vars_map_bytecode_offset + CON_BYTECODE_TOP_LEVEL_VAR_NUM)));
		top_level_vars_map_bytecode_offset += CON_BYTECODE_TOP_LEVEL_VAR_NAME + Con_Arch_align(thread, ID_MODULE_GET_WORD(top_level_vars_map_bytecode_offset + CON_BYTECODE_TOP_LEVEL_VAR_NAME_SIZE));
	}

	u_char *module_bytecode = Con_Memory_malloc(thread, ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_SIZE), CON_MEMORY_CHUNK_OPAQUE);
	memmove(module_bytecode, bytecode, ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_SIZE));

	Con_Int *constants_offsets = Con_Memory_malloc(thread, sizeof(Con_Int) * ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_CONSTANTS), CON_MEMORY_CHUNK_OPAQUE);
	memmove(constants_offsets, ID_MODULE_GET_OFFSET(CON_BYTECODE_MODULE_CONSTANTS_CREATE_OFFSETS), sizeof(Con_Int) * ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_CONSTANTS));

	module = Con_Builtins_Module_Atom_new(thread, identifier, module_bytecode, ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_SIZE), imports, top_level_vars_map, ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_CONSTANTS), constants_offsets, name, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));

	init_func = Con_Builtins_Func_Atom_new(thread, false, Con_Builtins_Func_Atom_make_con_pc_bytecode(thread, module, ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_INSTRUCTIONS)), 0,  ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_TOP_LEVEL_VARS), NULL, CON_NEW_STRING("$$init$$"), module);
	Con_Builtins_Module_Atom_set_init_func(thread, module, init_func);

	CON_GET_SLOT_APPLY(CON_GET_SLOT(Con_Builtins_Thread_Atom_get_vm(thread), "modules"), "set", identifier, module);

	return identifier;
}
