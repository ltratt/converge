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

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Dict/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"

#include "Modules/libXML2.h"



Con_Obj *_Con_Modules_libXML2_parse_func(Con_Obj *);



Con_Obj *Con_Modules_libXML2_init(Con_Obj *thread, Con_Obj *identifier)
{
	Con_Obj *xdm_mod = Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("XML"), CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
	
	CON_SET_SLOT(xdm_mod, "parse", CON_NEW_UNBOUND_C_FUNC(_Con_Modules_libXML2_parse_func, "parse", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)));
	
	return xdm_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in libXML2 module
//

void _Con_Modules_libXML2_parse_func_characters (void *, const xmlChar *, int);
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
	NULL,
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
	Con_Obj *elements_stack;
	Con_Obj *elements_module;
} _Con_Modules_libXML2_parse_func_state;

Con_Obj *_Con_Modules_libXML2_parse_func(Con_Obj *thread)
{
	// XXX passing in elements_module is a hack, but is needed until we have a sane way to import
	// user modules from C modules.

	Con_Obj *elements_module, *xml_obj;
	CON_UNPACK_ARGS("SM", &xml_obj, &elements_module);
	
	Con_Builtins_String_Atom *xml_string_atom = CON_FIND_ATOM(xml_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));	

	if (xml_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;

	xmlSAXHandler handler;
	bzero(&handler, sizeof(xmlSAXHandler));
	
	_Con_Modules_libXML2_parse_func_state state;
	state.thread = thread;
	Con_Obj *document_elems = Con_Builtins_List_Atom_new(thread);
	state.elements_stack = Con_Builtins_List_Atom_new_va(thread, document_elems, NULL);
	state.elements_module = elements_module;
	
	if (xmlSAXUserParseMemory(&_Con_Modules_libXML2_parse_func_handler, &state, xml_string_atom->str, xml_string_atom->size) < 0)
		CON_XXX;
	
	if (Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(state.elements_stack, "len")) != 1)
		CON_XXX;
	
	CON_RETURN(CON_GET_SLOT_APPLY(CON_GET_MODULE_DEF(elements_module, "Document"), "new", document_elems));
}



void _Con_Modules_libXML2_parse_func_characters(void *user_data, const xmlChar *ch, int len)
{
	_Con_Modules_libXML2_parse_func_state *state = (_Con_Modules_libXML2_parse_func_state *) user_data;
	Con_Obj *thread = state->thread;
	Con_Obj *current_elem = CON_GET_SLOT_APPLY(state->elements_stack, "get", CON_NEW_INT(-1));
	
	Con_Obj *str = Con_Builtins_String_Atom_new_copy(thread, ch, len, CON_STR_UTF_8);
	Con_Obj *text_node = CON_APPLY(CON_GET_SLOT(CON_GET_MODULE_DEF(state->elements_module, "Text"), "new"), str);
	
	CON_GET_SLOT_APPLY(current_elem, "append", text_node);
}



void _Con_Modules_libXML2_parse_start_element(void *user_data, const xmlChar *localname, const xmlChar *prefix, const xmlChar *URI, int nb_namespaces, const xmlChar **namespaces, int nb_attributes, int nb_defaulted, const xmlChar **attributes)
{
	_Con_Modules_libXML2_parse_func_state *state = (_Con_Modules_libXML2_parse_func_state *) user_data;
	Con_Obj *thread = state->thread;
	Con_Obj *current_elem = CON_GET_SLOT_APPLY(state->elements_stack, "get", CON_NEW_INT(-1));
	
	Con_Obj *name_obj = Con_Builtins_String_Atom_new_copy(thread, localname, strlen(localname), CON_STR_UTF_8);
	
	Con_Obj *attribute_namespaces = Con_Builtins_Dict_Class_new(thread);
	const xmlChar **current_attr = attributes;
	for (Con_Int i = 0; i < nb_attributes + nb_defaulted; i += 1) {
		Con_Obj *attr_name = Con_Builtins_String_Atom_new_copy(thread, current_attr[0], strlen(current_attr[0]), CON_STR_UTF_8);
		Con_Obj *attr_val = Con_Builtins_String_Atom_new_copy(thread, current_attr[3], current_attr[4] - current_attr[3], CON_STR_UTF_8);
		
		Con_Obj *namespace;
		if (current_attr[2] != NULL)
			namespace = Con_Builtins_String_Atom_new_copy(thread, current_attr[2], strlen(current_attr[2]), CON_STR_UTF_8);
		else {
			namespace = CON_NEW_STRING("");
		}
		
		Con_Obj *namespace_dict;
		if ((namespace_dict = CON_GET_SLOT_APPLY_NO_FAIL(attribute_namespaces, "find", namespace)) == NULL) {
			namespace_dict = Con_Builtins_Dict_Class_new(thread);
			CON_GET_SLOT_APPLY(attribute_namespaces, "set", namespace, namespace_dict);
		}
	
		CON_GET_SLOT_APPLY(namespace_dict, "set", attr_name, attr_val);
		
		current_attr += 5;
	}
	
	Con_Obj *element = CON_APPLY(CON_GET_SLOT(CON_GET_MODULE_DEF(state->elements_module, "Element"), "new"), name_obj, attribute_namespaces);
	
	CON_GET_SLOT_APPLY(current_elem, "append", element);
	
	CON_GET_SLOT_APPLY(state->elements_stack, "append", element);
}



void _Con_Modules_libXML2_parse_end_element(void *user_data, const xmlChar *localname, const xmlChar *prefix, const xmlChar *URI)
{
	_Con_Modules_libXML2_parse_func_state *state = (_Con_Modules_libXML2_parse_func_state *) user_data;
	Con_Obj *thread = state->thread;
	
	CON_GET_SLOT_APPLY(state->elements_stack, "pop");
}