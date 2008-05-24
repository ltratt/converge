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
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "Arch.h"
#include "Bytecode.h"
#include "Core.h"
#include "Memory.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"
#include "Target.h"

#include "Builtins/Dict/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



#	define ID_BYTECODE_GET_WORD(x) (*(Con_Int*) (bc + (x)))

//
// Import an exec file from memory. bytecode should be a chunk of memory that will not be written to
// and which will not be freed until after this function has completed. Note that holding a lock on
// a segment while calling this function may lead to deadlock.
//

Con_Obj *Con_Bytecode_add_executable(Con_Obj *thread, u_char *bc)
{
	int i, num_modules, mod_offset;
	Con_Obj *main_module_identifier;

	num_modules = ID_BYTECODE_GET_WORD(CON_BYTECODE_NUMBER_OF_MODULES);
	
	main_module_identifier = NULL;
	for (i = 0; i < num_modules; i += 1) {
		mod_offset = ID_BYTECODE_GET_WORD(CON_BYTECODE_MODULES + i * sizeof(Con_Int));

		Con_Obj *module = Con_Builtins_Module_Atom_new_from_bytecode(thread, bc + mod_offset);
		Con_Obj *identifier = CON_GET_SLOT(module, "mod_id");
		CON_GET_SLOT_APPLY(CON_GET_SLOT(Con_Builtins_Thread_Atom_get_vm(thread), "modules"), "set", identifier, module);

		if (i == 0)
			main_module_identifier = identifier;
	}
	
	if (main_module_identifier == NULL)
		CON_XXX;

	return main_module_identifier;
}



//
// XXX This is intended to be a part of a poor mans dynamic linker. It is very limited; in particular
// it doesn't handle packages properly.
//

void Con_Bytecode_add_lib(Con_Obj *thread, u_char *bc)
{
	int i, num_mods, mod_offset;

	num_mods = ID_BYTECODE_GET_WORD(CON_LIBRARY_BYTECODE_NUM_MODULES);
	
	for (i = 0; i < num_mods; i += 1) {
		mod_offset = ID_BYTECODE_GET_WORD(CON_LIBRARY_BYTECODE_MODULE_OFFSETS + i * sizeof(Con_Int));

        // XXX we simply skip packages currently. This needs more thought.

        if (memcmp(bc + mod_offset, "CONVPACK", 8) == 0)
            continue;

#define ID_MODULE_GET_WORD(x) (*(Con_Int*) (bc + mod_offset + (x)))
#define ID_MODULE_GET_OFFSET(x) ((void *) (bc + mod_offset + (ID_MODULE_GET_WORD(x))))

        u_char *id = ID_MODULE_GET_OFFSET(CON_BYTECODE_MODULE_IDENTIFIER);
        size_t id_size = ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_IDENTIFIER_SIZE);
        Con_Obj *cnd_id = Con_Builtins_String_Atom_new_copy(thread, id, id_size, CON_STR_UTF_8);
        
        Con_Obj *mods_dict = CON_GET_SLOT(Con_Builtins_Thread_Atom_get_vm(thread), "modules");
        if (CON_GET_SLOT_APPLY_NO_FAIL(mods_dict, "find", cnd_id) == NULL) {
    		Con_Obj *mod = Con_Builtins_Module_Atom_new_from_bytecode(thread, bc + mod_offset);
		    CON_GET_SLOT_APPLY(mods_dict, "set", cnd_id, mod);
        }
	}
}



//
// Returns false if any of the constituent modules in bytecode is newer than 'mtime'.
//

bool Con_Bytecode_upto_date(Con_Obj *thread, u_char *bc, STAT_ST_MTIMESPEC_TYPE *mtime)
{
	int num_modules = ID_BYTECODE_GET_WORD(CON_BYTECODE_NUMBER_OF_MODULES);
	
	char path[PATH_MAX];
	for (int i = 0; i < num_modules; i += 1) {
		int mod_offset = ID_BYTECODE_GET_WORD(CON_BYTECODE_MODULES + i * sizeof(Con_Int));

		int id_size = ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_SRC_PATH_SIZE);

		assert(id_size + 2 < PATH_MAX);

		memmove(path, ID_MODULE_GET_OFFSET(CON_BYTECODE_MODULE_SRC_PATH), id_size);
		path[ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_SRC_PATH_SIZE)] = '\0';
		
		struct stat cv_sb;
		if (stat(path, &cv_sb) != 0) {
			if (errno == ENOENT)
				continue;
		}
		
#if defined(STAT_ST_MTIMESPEC_TYPE_STRUCT_TIMESPEC)
		if (cv_sb.STAT_ST_MTIMESPEC.tv_sec > mtime->tv_sec)
			return false;
		else if (cv_sb.STAT_ST_MTIMESPEC.tv_sec == mtime->tv_sec &&
			cv_sb.STAT_ST_MTIMESPEC.tv_nsec >= mtime->tv_nsec)
			return false;
#elif defined(STAT_ST_MTIMESPEC_TYPE_TIME_T)
		if (cv_sb.STAT_ST_MTIMESPEC > *mtime)
			return false;
#else
		Unknown type
#endif
	}
	
	return true;
}
