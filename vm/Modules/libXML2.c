// Copyright (c) 2006 Laurence Tratt
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

#include <libxml/parser.h>

#include "Core.h"
#include "Memory.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Dict/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Set/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *Con_Module_libXML2_init(Con_Obj *, Con_Obj *);
Con_Obj *Con_Module_libXML2_import(Con_Obj *, Con_Obj *);

Con_Obj *_Con_Modules_libXML2_parse_func(Con_Obj *);



Con_Obj *Con_Module_libXML2_init(Con_Obj *thread, Con_Obj *identifier)
{
	const char* defn_names[] = {"XML_Exception", "parse", NULL};

	return Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("libXML"), defn_names, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



Con_Obj *Con_Module_libXML2_import(Con_Obj *thread, Con_Obj *libxml2_mod)
{
	Con_Obj *user_exception = CON_GET_MOD_DEFN(CON_BUILTIN(CON_BUILTIN_EXCEPTIONS_MODULE), "User_Exception");
	Con_Obj *xml_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("XML_Exception"), Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), libxml2_mod);
	CON_SET_MOD_DEF(libxml2_mod, "XML_Exception", xml_exception);
	
	CON_SET_MOD_DEF(libxml2_mod, "parse", CON_NEW_UNBOUND_C_FUNC(_Con_Modules_libXML2_parse_func, "parse", libxml2_mod));

	return libxml2_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in libXML2 module
//

void _Con_Modules_libXML2_parse_func_characters(void *, const xmlChar *, int);
void _Con_Modules_libXML2_parse_error(void *, const char *, ...);
void _Con_Modules_libXML2_parse_start_element(void *, const xmlChar *, const xmlChar *, const xmlChar *, int, const xmlChar **, int, int, const xmlChar **);
void _Con_Modules_libXML2_parse_end_element(void *, const xmlChar *, const xmlChar *, const xmlChar *);

static xmlSAXHandler _Con_Modules_libXML2_parse_func_handler = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	_Con_Modules_libXML2_parse_func_characters,
	NULL,
	NULL,
	NULL,
	NULL,
	_Con_Modules_libXML2_parse_error,
	NULL,
	NULL,
	NULL,
	NULL,
	XML_SAX2_MAGIC,
	NULL,
	_Con_Modules_libXML2_parse_start_element,
	_Con_Modules_libXML2_parse_end_element,
	NULL
};

typedef struct {
	Con_Obj *thread;
	Con_Obj *libxml2_mod;
	Con_Obj *elements_stack;
	Con_Obj *nodes_module;
} _Con_Modules_libXML2_parse_func_state;

Con_Obj *_Con_Modules_libXML2_parse_func(Con_Obj *thread)
{
	Con_Obj *libxml2_mod = Con_Builtins_VM_Atom_get_functions_module(thread);

	// XXX passing in nodes_module is a hack, but is needed until we have a sane way to import
	// user modules from C modules.

	Con_Obj *nodes_module, *xml_obj;
	CON_UNPACK_ARGS("SM", &xml_obj, &nodes_module);
	
	Con_Builtins_String_Atom *xml_string_atom = CON_FIND_ATOM(xml_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));	

	if (xml_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;

	xmlSAXHandler handler;
	bzero(&handler, sizeof(xmlSAXHandler));
	
	_Con_Modules_libXML2_parse_func_state state;
	state.thread = thread;
	Con_Obj *document_elems = Con_Builtins_List_Atom_new(thread);
	state.elements_stack = Con_Builtins_List_Atom_new_va(thread, document_elems, NULL);
	state.nodes_module = nodes_module;
	state.libxml2_mod = libxml2_mod;
	
	if (xmlSAXUserParseMemory(&_Con_Modules_libXML2_parse_func_handler, &state, (char *) xml_string_atom->str, xml_string_atom->size) < 0)
		CON_XXX;
	
	if (Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(state.elements_stack, "len")) != 1)
		CON_XXX;
	
	return CON_GET_SLOT_APPLY(CON_GET_MOD_DEFN(nodes_module, "Document"), "new", document_elems);
}



void _Con_Modules_libXML2_parse_func_characters(void *user_data, const xmlChar *ch, int len)
{
	_Con_Modules_libXML2_parse_func_state *state = (_Con_Modules_libXML2_parse_func_state *) user_data;
	Con_Obj *thread = state->thread;
	Con_Obj *current_elem = CON_GET_SLOT_APPLY(state->elements_stack, "get", CON_NEW_INT(-1));
	
	Con_Obj *str = Con_Builtins_String_Atom_new_copy(thread, ch, len, CON_STR_UTF_8);
	Con_Obj *text_node = CON_APPLY(CON_GET_SLOT(CON_GET_MOD_DEFN(state->nodes_module, "Text"), "new"), str);
	
	CON_GET_SLOT_APPLY(current_elem, "append", text_node);
}



#define ERROR_BUF_SIZE 1024

void _Con_Modules_libXML2_parse_error(void *user_data, const char *msg, ...)
{
	_Con_Modules_libXML2_parse_func_state *state = (_Con_Modules_libXML2_parse_func_state *) user_data;
	Con_Obj *thread = state->thread;

	char *buf = Con_Memory_malloc(thread, ERROR_BUF_SIZE, CON_MEMORY_CHUNK_OPAQUE);

	va_list args;
	va_start(args, msg);
	if (vsnprintf(buf, ERROR_BUF_SIZE, msg, args) > ERROR_BUF_SIZE)
		CON_XXX;
	va_end(args);
	
	Con_Obj *exception = CON_GET_SLOT_APPLY(CON_GET_MOD_DEFN(state->libxml2_mod, "XML_Exception"), "new", Con_Builtins_String_Atom_new_no_copy(thread, (u_char *) buf, strlen(buf), CON_STR_UTF_8));
	Con_Builtins_VM_Atom_raise(thread, exception);
}



void _Con_Modules_libXML2_parse_start_element(void *user_data, const xmlChar *localname, const xmlChar *prefix, const xmlChar *URI, int nb_namespaces, const xmlChar **namespaces, int nb_attributes, int nb_defaulted, const xmlChar **attributes)
{
	_Con_Modules_libXML2_parse_func_state *state = (_Con_Modules_libXML2_parse_func_state *) user_data;
	Con_Obj *thread = state->thread;
	Con_Obj *current_elem = CON_GET_SLOT_APPLY(state->elements_stack, "get", CON_NEW_INT(-1));
	
	Con_Obj *name_obj = Con_Builtins_String_Atom_new_copy(thread, localname, strlen((char *) localname), CON_STR_UTF_8);
	assert(prefix != NULL || namespaces == NULL);
	Con_Obj *prefix_obj, *namespace_obj;
	if (prefix != NULL) {
		prefix_obj = Con_Builtins_String_Atom_new_copy(thread, prefix, strlen((char *) prefix), CON_STR_UTF_8);
		namespace_obj = Con_Builtins_String_Atom_new_copy(thread, URI, strlen((char *) URI), CON_STR_UTF_8);
	}
	else
		prefix_obj = namespace_obj = CON_NEW_STRING("");
	
	Con_Obj *attributes_obj = Con_Builtins_Set_Atom_new(thread);
	const xmlChar **current_attr = attributes;
	for (Con_Int i = 0; i < nb_attributes + nb_defaulted; i += 1) {
		Con_Obj *attr_name = Con_Builtins_String_Atom_new_copy(thread, current_attr[0], strlen((char *) current_attr[0]), CON_STR_UTF_8);
		Con_Obj *attr_val = Con_Builtins_String_Atom_new_copy(thread, current_attr[3], current_attr[4] - current_attr[3], CON_STR_UTF_8);
		
		Con_Obj *attr_namespace, *attr_prefix;
		assert(current_attr[1] != NULL || current_attr[2] == NULL);
		if (current_attr[1] != NULL) {
			attr_prefix = Con_Builtins_String_Atom_new_copy(thread, current_attr[1], strlen((char *) current_attr[1]), CON_STR_UTF_8);
			attr_namespace = Con_Builtins_String_Atom_new_copy(thread, current_attr[2], strlen((char *) current_attr[2]), CON_STR_UTF_8);
		}
		else {
			attr_namespace = attr_prefix = CON_NEW_STRING("");
		}
		
		Con_Obj *attr = CON_APPLY(CON_GET_SLOT(CON_GET_MOD_DEFN(state->nodes_module, "Attribute"), "new"), attr_name, attr_val, attr_prefix, attr_namespace);
		CON_GET_SLOT_APPLY(attributes_obj, "add", attr);
		
		current_attr += 5;
	}
	
	Con_Obj *element = CON_APPLY(CON_GET_SLOT(CON_GET_MOD_DEFN(state->nodes_module, "Element"), "new"), name_obj, attributes_obj, prefix_obj, namespace_obj);
	
	CON_GET_SLOT_APPLY(current_elem, "append", element);
	
	CON_GET_SLOT_APPLY(state->elements_stack, "append", element);
}



void _Con_Modules_libXML2_parse_end_element(void *user_data, const xmlChar *localname, const xmlChar *prefix, const xmlChar *URI)
{
	_Con_Modules_libXML2_parse_func_state *state = (_Con_Modules_libXML2_parse_func_state *) user_data;
	Con_Obj *thread = state->thread;
	
	CON_GET_SLOT_APPLY(state->elements_stack, "pop");
}
