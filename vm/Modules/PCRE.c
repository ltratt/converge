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

#include <pcre.h>
#include <string.h>

#include "Core.h"
#include "Misc.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"




// Patterns are immutable.

typedef struct {
	CON_ATOM_HEAD
	
	pcre* compiled_re;
	int num_captures;
} _Con_Module_PCRE_Pattern_Atom;

void _Con_Module_PCRE_Pattern_Atom_gc_clean_up(Con_Obj *, Con_Obj *, Con_Atom *);

// Matches are immutable.

typedef struct {
	CON_ATOM_HEAD

	int* ovector;
	int num_captures;
	Con_Obj *string;
} _Con_Module_PCRE_Match_Atom;

void _Con_Module_PCRE_Match_Atom_gc_scan(Con_Obj *, Con_Obj *, Con_Atom *);


Con_Obj *Con_Module_PCRE_init(Con_Obj *thread, Con_Obj *);
Con_Obj *Con_Module_PCRE_import(Con_Obj *thread, Con_Obj *);

Con_Obj *_Con_Module_PCRE_compile_func(Con_Obj *);

Con_Obj *_Con_Module_PCRE_Pattern_new(Con_Obj *);
Con_Obj *_Con_Module_PCRE_Pattern_match_func(Con_Obj *);
Con_Obj *_Con_Module_PCRE_Pattern_search_func(Con_Obj *);

Con_Obj *_Con_Module_PCRE_Match_get_func(Con_Obj *);
Con_Obj *_Con_Module_PCRE_Match_get_indexes_func(Con_Obj *);



Con_Obj *Con_Module_PCRE_init(Con_Obj *thread, Con_Obj *identifier)
{
	const char* defn_names[] = {"PCRE_Exception", "Pattern_Atom_Def", "Pattern", "Match_Atom_Def", "Match", "compile", NULL};

	return Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("PCRE"), defn_names, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



Con_Obj *Con_Module_PCRE_import(Con_Obj *thread, Con_Obj *pcre_mod)
{
	// PCRE_Exception

	Con_Obj *user_exception = CON_GET_MOD_DEFN(CON_BUILTIN(CON_BUILTIN_EXCEPTIONS_MODULE), "User_Exception");
	Con_Obj *pcre_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("PCRE_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), pcre_mod);
	CON_SET_MOD_DEFN(pcre_mod, "PCRE_Exception", pcre_exception);

	// Pattern_Atom_Def
	
	CON_SET_MOD_DEFN(pcre_mod, "Pattern_Atom_Def", Con_Builtins_Atom_Def_Atom_new(thread, NULL, _Con_Module_PCRE_Pattern_Atom_gc_clean_up));

	// PCRE.Pattern
	
	Con_Obj *pattern_class = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Pattern"), Con_Builtins_List_Atom_new_va(thread, CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL), pcre_mod, CON_NEW_UNBOUND_C_FUNC(_Con_Module_PCRE_Pattern_new, "Pattern_new", pcre_mod));
	CON_SET_MOD_DEFN(pcre_mod, "Pattern", pattern_class);
	
	CON_SET_FIELD(pattern_class, "match", CON_NEW_BOUND_C_FUNC(_Con_Module_PCRE_Pattern_match_func, "match", pcre_mod, pattern_class));
	CON_SET_FIELD(pattern_class, "search", CON_NEW_BOUND_C_FUNC(_Con_Module_PCRE_Pattern_search_func, "search", pcre_mod, pattern_class));

	// Match_Atom_Def
	
	CON_SET_MOD_DEFN(pcre_mod, "Match_Atom_Def", Con_Builtins_Atom_Def_Atom_new(thread, _Con_Module_PCRE_Match_Atom_gc_scan, NULL));

	// PCRE.Match
	
	Con_Obj *match_class = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Match"), Con_Builtins_List_Atom_new_va(thread, CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL), pcre_mod);
	CON_SET_MOD_DEFN(pcre_mod, "Match", match_class);
	
	CON_SET_FIELD(match_class, "get", CON_NEW_BOUND_C_FUNC(_Con_Module_PCRE_Match_get_func, "get", pcre_mod, match_class));
	CON_SET_FIELD(match_class, "get_indexes", CON_NEW_BOUND_C_FUNC(_Con_Module_PCRE_Match_get_indexes_func, "get_indexes", pcre_mod, match_class));
	
	// PCRE module
	
	CON_SET_MOD_DEFN(pcre_mod, "compile", CON_NEW_UNBOUND_C_FUNC(_Con_Module_PCRE_compile_func, "compile", pcre_mod));
	
	return pcre_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Module_PCRE_Pattern_Atom_gc_clean_up(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	_Con_Module_PCRE_Pattern_Atom *pattern_atom = (_Con_Module_PCRE_Pattern_Atom *) atom;

	free(pattern_atom->compiled_re);
}



void _Con_Module_PCRE_Match_Atom_gc_scan(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	_Con_Module_PCRE_Match_Atom *match_atom = (_Con_Module_PCRE_Match_Atom *) atom;

	Con_Memory_gc_push(thread, match_atom->ovector);
	Con_Memory_gc_push(thread, match_atom->string);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Top-level module functions
//

Con_Obj *_Con_Module_PCRE_compile_func(Con_Obj *thread)
{
	Con_Obj *pattern;
	CON_UNPACK_ARGS("O", &pattern);
	
	return CON_GET_SLOT_APPLY(CON_GET_MOD_DEFN(Con_Builtins_VM_Atom_get_functions_module(thread), "Pattern"), "new", pattern);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// func Pattern_new
//

//
// 'Pattern_new(class_, pattern)' pre-compiles a new pattern object.
//

Con_Obj *_Con_Module_PCRE_Pattern_new(Con_Obj *thread)
{
	Con_Obj *pcre_mod = Con_Builtins_VM_Atom_get_functions_module(thread);

	Con_Obj *class, *pattern;
	CON_UNPACK_ARGS("CO", &class, &pattern);

	Con_Obj *new_pattern = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(_Con_Module_PCRE_Pattern_Atom) + sizeof(Con_Builtins_Slots_Atom), class);
	_Con_Module_PCRE_Pattern_Atom *pattern_atom = (_Con_Module_PCRE_Pattern_Atom *) new_pattern->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (pattern_atom + 1);
	pattern_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	pattern_atom->atom_type = CON_GET_MOD_DEFN(pcre_mod, "Pattern_Atom_Def");
	const char *errptr;
	int erroffset;
	if ((pattern_atom->compiled_re = pcre_compile(Con_Builtins_String_Atom_to_c_string(thread, pattern), PCRE_DOTALL | PCRE_MULTILINE, &errptr, &erroffset, NULL)) == NULL) {
		Con_Obj *msg = Con_Builtins_String_Atom_new_no_copy(thread, (const u_char *) errptr, strlen(errptr), CON_STR_UTF_8);
		msg = CON_ADD(msg, CON_NEW_STRING(" at pattern position "));
		msg = CON_ADD(msg, CON_GET_SLOT_APPLY(CON_NEW_INT(erroffset), "to_str"));
		msg = CON_ADD(msg, CON_NEW_STRING("."));
		Con_Obj *exception = CON_GET_SLOT_APPLY(CON_GET_MOD_DEFN(pcre_mod, "PCRE_Exception"), "new", msg);
	Con_Builtins_VM_Atom_raise(thread, exception);
		CON_XXX;
	}
	
	if (pcre_fullinfo(pattern_atom->compiled_re, NULL, PCRE_INFO_CAPTURECOUNT, &pattern_atom->num_captures) != 0)
		CON_XXX;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, new_pattern, CON_MEMORY_CHUNK_OBJ);
	
	return new_pattern;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// class Pattern
//

//
// 'match(string, start_pos := 0)' returns a match object if the pattern matches at 'start_pos', or
// returns fail otherwise.
//

Con_Obj *_Con_Module_PCRE_Pattern_match_func(Con_Obj *thread)
{
	Con_Obj *pcre_mod = Con_Builtins_VM_Atom_get_functions_module(thread);
	Con_Obj *pattern_atom_def = CON_GET_MOD_DEFN(pcre_mod, "Pattern_Atom_Def");

	Con_Obj *self, *start_pos_obj, *string;
	CON_UNPACK_ARGS("OS;n", &self, &string, &start_pos_obj);
	
	_Con_Module_PCRE_Pattern_Atom *pattern_atom = CON_GET_ATOM(self, pattern_atom_def);
	Con_Builtins_String_Atom *string_atom = CON_FIND_ATOM(string, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;

	int start_pos;
	if (start_pos_obj == NULL)
		start_pos = 0;
	else
		start_pos = Con_Numbers_Con_Int_to_c_int(thread, Con_Misc_translate_index(thread, NULL, Con_Numbers_Number_to_Con_Int(thread, start_pos_obj), string_atom->size));
	
	int err;
	// The first entry in ovector will be the index of the entire match, so we need to allocate
	// enough memory for 1 + num_captures.
	int ovector_size = (1 + pattern_atom->num_captures) * 3;
	int *ovector = Con_Memory_malloc(thread, ovector_size * sizeof(int), CON_MEMORY_CHUNK_OPAQUE);
	if ((err = pcre_exec(pattern_atom->compiled_re, NULL, (char *) string_atom->str, string_atom->size, start_pos, PCRE_ANCHORED, ovector, ovector_size)) < 0) {
		if (err == PCRE_ERROR_NOMATCH)
			return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
		else
			CON_XXX;
	}
	
	// We've matched successfully, so create a match object.
	
	Con_Obj *new_match = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(_Con_Module_PCRE_Match_Atom), CON_GET_MOD_DEFN(pcre_mod, "Match"));
	_Con_Module_PCRE_Match_Atom *match_atom = (_Con_Module_PCRE_Match_Atom *) new_match->first_atom;
	match_atom->next_atom = NULL;
	
	match_atom->atom_type = CON_GET_MOD_DEFN(pcre_mod, "Match_Atom_Def");
	match_atom->ovector = ovector;
	match_atom->num_captures = pattern_atom->num_captures;
	match_atom->string = string;

	Con_Memory_change_chunk_type(thread, new_match, CON_MEMORY_CHUNK_OBJ);
	
	return new_match;
}



//
// 'search(string, start_pos := 0)' returns a match object if a match is found at some position
// after 'start_pos'; it fails otherwise.
//

Con_Obj *_Con_Module_PCRE_Pattern_search_func(Con_Obj *thread)
{
	Con_Obj *pcre_mod = Con_Builtins_VM_Atom_get_functions_module(thread);
	Con_Obj *pattern_atom_def = CON_GET_MOD_DEFN(pcre_mod, "Pattern_Atom_Def");

	Con_Obj *self, *start_pos_obj, *string;
	CON_UNPACK_ARGS("OS;n", &self, &string, &start_pos_obj);
	
	_Con_Module_PCRE_Pattern_Atom *pattern_atom = CON_GET_ATOM(self, pattern_atom_def);
	Con_Builtins_String_Atom *string_atom = CON_FIND_ATOM(string, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;

	int start_pos;
	if (start_pos_obj == NULL)
		start_pos = 0;
	else
		start_pos = Con_Numbers_Con_Int_to_c_int(thread, Con_Misc_translate_index(thread, NULL, Con_Numbers_Number_to_Con_Int(thread, start_pos_obj), string_atom->size));
	
	int err;
	// The first entry in ovector will be the index of the entire match, so we need to allocate
	// enough memory for 1 + num_captures.
	int ovector_size = (1 + pattern_atom->num_captures) * 3;
	int *ovector = Con_Memory_malloc(thread, ovector_size * sizeof(int), CON_MEMORY_CHUNK_OPAQUE);
	if ((err = pcre_exec(pattern_atom->compiled_re, NULL, (char *) string_atom->str, string_atom->size, start_pos, 0, ovector, ovector_size)) < 0) {
		if (err == PCRE_ERROR_NOMATCH)
			return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
		else
			CON_XXX;
	}
	
	// We've matched successfully, so create a match object.
	
	Con_Obj *new_match = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(_Con_Module_PCRE_Match_Atom), CON_GET_MOD_DEFN(pcre_mod, "Match"));
	_Con_Module_PCRE_Match_Atom *match_atom = (_Con_Module_PCRE_Match_Atom *) new_match->first_atom;
	match_atom->next_atom = NULL;
	
	match_atom->atom_type = CON_GET_MOD_DEFN(pcre_mod, "Match_Atom_Def");
	match_atom->ovector = ovector;
	match_atom->num_captures = pattern_atom->num_captures;
	match_atom->string = string;

	Con_Memory_change_chunk_type(thread, new_match, CON_MEMORY_CHUNK_OBJ);
	
	return new_match;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// class Match
//

//
// 'get(i)'
//

Con_Obj *_Con_Module_PCRE_Match_get_func(Con_Obj *thread)
{
	Con_Obj *pcre_mod = Con_Builtins_VM_Atom_get_functions_module(thread);

	Con_Obj *group_num_obj, *self;
	CON_UNPACK_ARGS("ON", &self, &group_num_obj);
	
	_Con_Module_PCRE_Match_Atom *match_atom = CON_GET_ATOM(self, CON_GET_MOD_DEFN(pcre_mod, "Match_Atom_Def"));
	
	// Group 0 in the match is the entire match, so when translating indices, we need to add 1 onto
	// num_captures.
	Con_Int group_num = Con_Misc_translate_index(thread, NULL, Con_Numbers_Number_to_Con_Int(thread, group_num_obj), 1 + match_atom->num_captures);
	
	return CON_GET_SLOT_APPLY(match_atom->string, "get_slice", CON_NEW_INT(match_atom->ovector[group_num * 2]), CON_NEW_INT(match_atom->ovector[group_num * 2 + 1]));
}



//
// 'get_indexes(i)'
//

Con_Obj *_Con_Module_PCRE_Match_get_indexes_func(Con_Obj *thread)
{
	Con_Obj *pcre_mod = Con_Builtins_VM_Atom_get_functions_module(thread);

	Con_Obj *group_num_obj, *self;
	CON_UNPACK_ARGS("ON", &self, &group_num_obj);
	
	_Con_Module_PCRE_Match_Atom *match_atom = CON_GET_ATOM(self, CON_GET_MOD_DEFN(pcre_mod, "Match_Atom_Def"));
	
	// Group 0 in the match is the entire match, so when translating indices, we need to add 1 onto
	// num_captures.
	Con_Int group_num = Con_Misc_translate_index(thread, NULL, Con_Numbers_Number_to_Con_Int(thread, group_num_obj), 1 + match_atom->num_captures);
	
	return Con_Builtins_List_Atom_new_va(thread, CON_NEW_INT(match_atom->ovector[group_num * 2]), CON_NEW_INT(match_atom->ovector[group_num * 2 + 1]), NULL);
}
