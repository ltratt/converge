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
#include "Hash.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_String_Class_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);

Con_Obj *_Con_Builtins_String_Class_eq_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_add_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_String_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *string_atom_def = CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT);
	
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) string_atom_def->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (atom_def_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_String_Class_gc_scan_func, NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, string_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// String creation functions
//

Con_Obj *Con_Builtins_String_Atom_new_copy(Con_Obj *thread, const char *str, Con_Int size, Con_String_Encoding enc)
{
	Con_Obj *new_str;

	new_str = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_String_Atom), CON_BUILTIN(CON_BUILTIN_STRING_CLASS));
	new_str->first_atom->next_atom = NULL;
	
	u_char *str_mem = Con_Memory_malloc(thread, size, CON_MEMORY_CHUNK_OPAQUE);
	memmove(str_mem, str, size);
	Con_Builtins_String_Atom_init_atom(thread, (Con_Builtins_String_Atom *) new_str->first_atom, str_mem, size, enc);
	
	Con_Memory_change_chunk_type(thread, new_str, CON_MEMORY_CHUNK_OBJ);
	
	return new_str;
}



Con_Obj *Con_Builtins_String_Atom_new_no_copy(Con_Obj *thread, const char *str, Con_Int size, Con_String_Encoding enc)
{
	Con_Obj *new_str;

	new_str = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_String_Atom), CON_BUILTIN(CON_BUILTIN_STRING_CLASS));
	new_str->first_atom->next_atom = NULL;
	
	Con_Builtins_String_Atom_init_atom(thread, (Con_Builtins_String_Atom *) new_str->first_atom, str, size, enc);
	
	Con_Memory_change_chunk_type(thread, new_str, CON_MEMORY_CHUNK_OBJ);
	
	return new_str;
}



void Con_Builtins_String_Atom_init_atom(Con_Obj *thread, Con_Builtins_String_Atom *str_atom, const char *str, Con_Int size, Con_String_Encoding enc)
{
	if (enc != CON_STR_UTF_8)
		CON_XXX;

	str_atom->atom_type = CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT);

	str_atom->encoding = enc;
	str_atom->size = size;
	str_atom->str = str;
	str_atom->hash = Con_Hash_calc_string_hash(thread, str_atom->str, size);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//

//
// Given a Converge string 'string', return a pointer to a NULL-terminated C string.
//
// The pointer returned must not be explicitly freed; if memory is allocated, it will be freed by
// the garbage collector.
//

char *Con_Builtins_String_Atom_to_c_string(Con_Obj *thread, Con_Obj *string)
{
	Con_Builtins_String_Atom *string_atom = CON_GET_ATOM(string, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	assert(string_atom->encoding == CON_STR_UTF_8);
	
	char *c_str = Con_Memory_malloc(thread, string_atom->size + 1, CON_MEMORY_CHUNK_OPAQUE);
	memmove(c_str, string_atom->str, string_atom->size);
	c_str[string_atom->size] = 0;
	
	return c_str;
}



//
// Compare the UTF-8 string 'c_string' of size 'c_string_size' with 'string_obj' for equality.
//

bool Con_Builtins_String_Atom_c_string_eq(Con_Obj *thread, const char *c_string, Con_Int c_string_size, Con_Obj *string_obj)
{
	Con_Builtins_String_Atom *string_obj_atom = CON_FIND_ATOM(string_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (string_obj_atom == NULL)
		return false;
	
	if (string_obj_atom->encoding == CON_STR_UTF_8) {
		if (string_obj_atom->size != c_string_size)
			return false;
			
		if (memcmp(c_string, string_obj_atom->str, c_string_size) == 0)
			return true;
		else
			return false;
	}
	else
		CON_XXX;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_String_Class_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_String_Atom * str_atom = (Con_Builtins_String_Atom *) atom;

	Con_Memory_gc_push_ptr(thread, str_atom->str);
}
