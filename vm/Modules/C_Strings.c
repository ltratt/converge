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

#include "Core.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"

#include "Modules/Sys.h"



Con_Obj *Con_Modules_C_Strings_init(Con_Obj *, Con_Obj *);

Con_Obj *_Con_Modules_C_Strings_join_func(Con_Obj *);



Con_Obj *Con_Modules_C_Strings_init(Con_Obj *thread, Con_Obj *identifier)
{
	Con_Obj *c_strings_mod = Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("C_Strings"), CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
	
	CON_SET_SLOT(c_strings_mod, "join", CON_NEW_UNBOUND_C_FUNC(_Con_Modules_C_Strings_join_func, "join", c_strings_mod));
	
	return c_strings_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in Sys module
//

Con_Obj *_Con_Modules_C_Strings_join_func(Con_Obj *thread)
{
	Con_Obj *list, *separator_obj;
	CON_UNPACK_ARGS("OS", &list, &separator_obj);

	Con_Builtins_String_Atom *separator_string_atom = CON_FIND_ATOM(separator_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (separator_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;

	// We have to make some sort of guess about how much memory we're going to need. Unfortunately
	// the penalty for guessing wrong either way is likely to be quite high.

	Con_Int str_mem_size_allocated;
	Con_Int num_list_entries = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(list, "len"));
	if (num_list_entries == 0)
		str_mem_size_allocated = 0;
	else
		str_mem_size_allocated = num_list_entries * 8 + num_list_entries * separator_string_atom->size;
	
	u_char *str_mem = Con_Memory_malloc(thread, str_mem_size_allocated, CON_MEMORY_CHUNK_OPAQUE);
	
	Con_Int i = 0;
	Con_Int str_mem_size = 0;
	CON_PRE_GET_SLOT_APPLY_PUMP(list, "iterate");
	while (1) {
		Con_Obj *val = CON_APPLY_PUMP();
		if (val == NULL)
			break;
		Con_Builtins_String_Atom *val_string_atom = CON_FIND_ATOM(val, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		if (val_string_atom == NULL)
			CON_RAISE_EXCEPTION("Type_Exception", CON_BUILTIN(CON_BUILTIN_STRING_CLASS), val, CON_NEW_STRING("member of list"));

		Con_Int extra_size_needed;
		if (i == 0)
			extra_size_needed = val_string_atom->size;
		else
			extra_size_needed = val_string_atom->size + separator_string_atom->size;

		if (str_mem_size + extra_size_needed > str_mem_size_allocated) {
			Con_Int new_str_mem_size_allocated = str_mem_size_allocated + str_mem_size_allocated / 3;
			if (new_str_mem_size_allocated < str_mem_size + extra_size_needed)
				new_str_mem_size_allocated = str_mem_size + extra_size_needed;
			str_mem = Con_Memory_realloc(thread, str_mem, new_str_mem_size_allocated);
			str_mem_size_allocated = new_str_mem_size_allocated;
		}
		
		if (separator_string_atom->encoding != val_string_atom->encoding)
			CON_XXX;
		
		if (i > 0) {
			memmove(str_mem + str_mem_size, separator_string_atom->str, separator_string_atom->size);
			str_mem_size += separator_string_atom->size;
		}

		memmove(str_mem + str_mem_size, val_string_atom->str, val_string_atom->size);
		str_mem_size += val_string_atom->size;
		
		i += 1;
	}
	
	if (str_mem_size_allocated > str_mem_size)
		str_mem = Con_Memory_realloc(thread, str_mem, str_mem_size);

	return Con_Builtins_String_Atom_new_no_copy(thread, str_mem, str_mem_size, CON_STR_UTF_8);
}
