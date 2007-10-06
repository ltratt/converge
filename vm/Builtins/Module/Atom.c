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
#include <dlfcn.h>

#include "Arch.h"
#include "Bytecode.h"
#include "Core.h"
#include "Hash.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Target.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Closure/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Dict/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Module/Class.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_Module_Class_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Module_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *module_atom_def = CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT);
	
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) module_atom_def->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (atom_def_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Module_Class_gc_scan_func, NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, module_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Module creation functions
//

Con_Obj *Con_Builtins_Module_Atom_new_c(Con_Obj *thread, Con_Obj *identifier, Con_Obj *name, const char* defn_names[], Con_Obj *container)
{
	Con_Obj *new_module = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Module_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_BUILTIN(CON_BUILTIN_MODULE_CLASS));
	Con_Builtins_Module_Atom *module_atom = (Con_Builtins_Module_Atom *) new_module->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (module_atom + 1);
	module_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	module_atom->atom_type = CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT);
	module_atom->state = CON_MODULE_UNINITIALIZED;
	module_atom->identifier = identifier;
	module_atom->name = name;
	module_atom->module_bytecode = NULL;
	module_atom->module_bytecode_offset = -1;
	module_atom->closure = NULL;
	module_atom->init_func = NULL;
	module_atom->imports = Con_Builtins_List_Atom_new_sized(thread, 0);
	module_atom->container = container;
	module_atom->num_constants = 0;
	module_atom->constants_create_offsets = 0;
	module_atom->constants = NULL;
	
	Con_Slots_init(thread, &module_atom->top_level_vars);
	for (int i = 0; defn_names[i] != NULL; i += 1) {
		Con_Slots_set_slot(thread, &new_module->mutex, &module_atom->top_level_vars, defn_names[i], strlen(defn_names[i]), NULL);
	}

	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, new_module, CON_MEMORY_CHUNK_OBJ);
	
	return new_module;
}



Con_Obj *Con_Builtins_Module_Atom_new_from_bytecode(Con_Obj *thread, u_char *bytecode)
{
	int j, imports_bytecode_offset;

	Con_Obj *module = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Module_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_BUILTIN(CON_BUILTIN_MODULE_CLASS));
	Con_Builtins_Module_Atom *module_atom = (Con_Builtins_Module_Atom *) module->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (module_atom + 1);
	module_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	module_atom->atom_type = CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT);
	module_atom->state = CON_MODULE_UNINITIALIZED;

#	define ID_MODULE_GET_WORD(x) (*(Con_Int*) (bytecode + (x)))
#	define ID_MODULE_GET_OFFSET(x) ((void *) (bytecode + (ID_MODULE_GET_WORD(x))))

	module_atom->identifier = Con_Builtins_String_Atom_new_copy(thread, ID_MODULE_GET_OFFSET(CON_BYTECODE_MODULE_IDENTIFIER), ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_IDENTIFIER_SIZE), CON_STR_UTF_8);
	module_atom->name = Con_Builtins_String_Atom_new_copy(thread, ID_MODULE_GET_OFFSET(CON_BYTECODE_MODULE_NAME), ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NAME_SIZE), CON_STR_UTF_8);

	module_atom->imports = Con_Builtins_List_Atom_new_sized(thread, ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_IMPORTS));
	imports_bytecode_offset = ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_IMPORTS);
	for (j = 0; j < ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_IMPORTS); j += 1) {
		CON_GET_SLOT_APPLY(module_atom->imports, "append", Con_Builtins_String_Atom_new_copy(thread, bytecode + imports_bytecode_offset + CON_BYTECODE_IMPORT_IDENTIFIER, ID_MODULE_GET_WORD(imports_bytecode_offset + CON_BYTECODE_IMPORT_IDENTIFIER_SIZE), CON_STR_UTF_8));
		imports_bytecode_offset += CON_BYTECODE_IMPORT_IDENTIFIER + Con_Arch_align(thread, ID_MODULE_GET_WORD(imports_bytecode_offset + CON_BYTECODE_IMPORT_IDENTIFIER_SIZE));
	}

	Con_Slots_init(thread, &module_atom->top_level_vars);
	Con_Int top_level_vars_map_bytecode_offset = ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_TOP_LEVEL_VARS_MAP);
	for (j = 0; j < ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_TOP_LEVEL_VARS); j += 1) {
		Con_Slots_set_slot(thread, &module->mutex, &module_atom->top_level_vars, bytecode + top_level_vars_map_bytecode_offset + CON_BYTECODE_TOP_LEVEL_VAR_NAME, ID_MODULE_GET_WORD(top_level_vars_map_bytecode_offset + CON_BYTECODE_TOP_LEVEL_VAR_NAME_SIZE), CON_NEW_INT(ID_MODULE_GET_WORD(top_level_vars_map_bytecode_offset + CON_BYTECODE_TOP_LEVEL_VAR_NUM)));
		top_level_vars_map_bytecode_offset += CON_BYTECODE_TOP_LEVEL_VAR_NAME + Con_Arch_align(thread, ID_MODULE_GET_WORD(top_level_vars_map_bytecode_offset + CON_BYTECODE_TOP_LEVEL_VAR_NAME_SIZE));
	}

	module_atom->module_bytecode = Con_Memory_malloc(thread, ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_SIZE), CON_MEMORY_CHUNK_OPAQUE);
	memmove(module_atom->module_bytecode, bytecode, ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_SIZE));
	module_atom->module_bytecode_offset = ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_SIZE);

	module_atom->num_constants = ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_CONSTANTS);
	module_atom->constants_create_offsets = Con_Memory_malloc(thread, sizeof(Con_Int) * ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_CONSTANTS), CON_MEMORY_CHUNK_OPAQUE);
	memmove(module_atom->constants_create_offsets, ID_MODULE_GET_OFFSET(CON_BYTECODE_MODULE_CONSTANTS_CREATE_OFFSETS), sizeof(Con_Int) * ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_CONSTANTS));
	module_atom->constants = Con_Memory_malloc(thread, ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_CONSTANTS) * sizeof(Con_Obj *), CON_MEMORY_CHUNK_OPAQUE);
	bzero(module_atom->constants, ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_CONSTANTS) * sizeof(Con_Obj *));
	
	module_atom->init_func = Con_Builtins_Func_Atom_new(thread, false, Con_Builtins_Func_Atom_make_con_pc_bytecode(thread, module, ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_INSTRUCTIONS)), 0,  ID_MODULE_GET_WORD(CON_BYTECODE_MODULE_NUM_TOP_LEVEL_VARS), NULL, CON_NEW_STRING("$$init$$"), module);

	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

	Con_Memory_change_chunk_type(thread, module, CON_MEMORY_CHUNK_OBJ);

	return module;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Module functions
//

//
// Return the instruction at position 'offset' in 'module's bytecode.
//

Con_Int Con_Builtins_Module_Atom_get_instruction(Con_Obj *thread, Con_Obj *module, Con_Int offset)
{
	assert(offset % sizeof(Con_Int) == 0);
	
	Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	
	// Since the bytecode memory is immutable, as well as the reference to it, there is no need to
	// acquire a mutex for this.
	
	return *((Con_Int *) (module_atom->module_bytecode + offset));
}



//
// Returns a string object representing the string at 'offset' of 'size' bytes.
//

Con_Obj *Con_Builtins_Module_Atom_get_string(Con_Obj *thread, Con_Obj *module, Con_Int offset, Con_Int size)
{
	u_char *str_mem = Con_Memory_malloc(thread, size, CON_MEMORY_CHUNK_OPAQUE);

	Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	
	// Since the bytecode memory is immutable, as well as the reference to it, there is no need to
	// acquire a mutex for this.
	
	memmove(str_mem, module_atom->module_bytecode + offset, size);
	
	return Con_Builtins_String_Atom_new_no_copy(thread, str_mem, (size_t) size, CON_STR_UTF_8);
}



void Con_Builtins_Module_Atom_read_bytes(Con_Obj *thread, Con_Obj *module, Con_Int offset, void *dst, Con_Int size)
{
	Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	
	// Since the bytecode memory is immutable, as well as the reference to it, there is no need to
	// acquire a mutex for this.
	
	memmove(dst, module_atom->module_bytecode + offset, size);
}



//
// Returns the value of constant number 'constant_num' in 'module'. Returns NULL if the constant has
// not yet been initialised (or if its previous value has been discarded).
//

Con_Obj *Con_Builtins_Module_Atom_get_constant(Con_Obj *thread, Con_Obj *module, Con_Int constant_num)
{
	Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&module->mutex);
	assert(constant_num < module_atom->num_constants);
	Con_Obj *constant = module_atom->constants[constant_num];
	CON_MUTEX_UNLOCK(&module->mutex);
	
	return constant;
}


//
// Sets the value of constant number 'constant_num' in 'module' to 'val'.
//

void Con_Builtins_Module_Atom_set_constant(Con_Obj *thread, Con_Obj *module, Con_Int constant_num, Con_Obj *val)
{
	Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&module->mutex);
	assert(constant_num < module_atom->num_constants);
	module_atom->constants[constant_num] = val;
	CON_MUTEX_UNLOCK(&module->mutex);
}



//
// Get the bytecode offset of the bytecode to create connstant number 'constant_num'.
//

Con_Int Con_Builtins_Module_Atom_get_constant_create_offset(Con_Obj *thread, Con_Obj *module, Con_Int constant_num)
{
	Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&module->mutex);
	assert(constant_num < module_atom->num_constants);
	Con_Int constant_create_offset = module_atom->constants_create_offsets[constant_num];
	CON_MUTEX_UNLOCK(&module->mutex);
	
	return constant_create_offset;
}



//
// Returns a list of tuples [<file name>, <src offset>]. The list will be at least length 1.
//

Con_Obj *Con_Builtins_Module_Atom_pc_to_src_locations(Con_Obj *thread, Con_PC pc)
{
	assert(pc.type == PC_TYPE_BYTECODE);

	Con_Obj *module = pc.module;

	Con_Obj *src_locations = Con_Builtins_List_Atom_new(thread);
	
	Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&module->mutex);

#	define MODULE_GET_WORD(x) (*(Con_Int*) (module_atom->module_bytecode + (x)))

	Con_Int instruction_num = 0;
	Con_Int current_pc = MODULE_GET_WORD(CON_BYTECODE_MODULE_INSTRUCTIONS);
	while (current_pc < pc.pc.bytecode_offset) {
		Con_Int instruction = MODULE_GET_WORD(current_pc);
		switch (instruction & 0x000000FF) {
			case CON_INSTR_EXBI:
				current_pc += Con_Arch_align(thread, CON_INSTR_DECODE_EXBI_START(instruction) + CON_INSTR_DECODE_EXBI_SIZE(instruction));
				break;
			case CON_INSTR_IS_ASSIGNED:
				current_pc += sizeof(Con_Int) + sizeof(Con_Int);
				break;
			case CON_INSTR_SLOT_LOOKUP:
				current_pc += Con_Arch_align(thread, CON_INSTR_DECODE_SLOT_LOOKUP_START(instruction) + CON_INSTR_DECODE_SLOT_LOOKUP_SIZE(instruction));
				break;
			case CON_INSTR_STRING:
				current_pc += Con_Arch_align(thread, CON_INSTR_DECODE_STRING_START(instruction) + CON_INSTR_DECODE_STRING_SIZE(instruction));
				break;
			case CON_INSTR_ASSIGN_SLOT:
				current_pc += Con_Arch_align(thread, CON_INSTR_DECODE_ASSIGN_SLOT_START(instruction) + + CON_INSTR_DECODE_ASSIGN_SLOT_SIZE(instruction));
				break;
			case CON_INSTR_UNPACK_ARGS:
				if (CON_INSTR_DECODE_UNPACK_ARGS_HAS_VAR_ARGS(instruction))
					current_pc += sizeof(Con_Int) + (CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction) + 1) * sizeof(Con_Int);
				else
					current_pc += sizeof(Con_Int) + CON_INSTR_DECODE_UNPACK_ARGS_NUM_NORMAL_ARGS(instruction) * sizeof(Con_Int);
				break;
			case CON_INSTR_PRE_SLOT_LOOKUP_APPLY:
				current_pc += Con_Arch_align(thread, CON_INSTR_DECODE_PRE_SLOT_LOOKUP_START(instruction) + + CON_INSTR_DECODE_PRE_SLOT_LOOKUP_SIZE(instruction));
				break;
			case CON_INSTR_MODULE_LOOKUP:
				current_pc += Con_Arch_align(thread, CON_INSTR_DECODE_MODULE_LOOKUP_START(instruction) + CON_INSTR_DECODE_MODULE_LOOKUP_SIZE(instruction));
				break;
			case CON_INSTR_VAR_LOOKUP:
			case CON_INSTR_VAR_ASSIGN:
			case CON_INSTR_INT:
			case CON_INSTR_ADD_FAILURE_FRAME:
			case CON_INSTR_ADD_FAIL_UP_FRAME:
			case CON_INSTR_REMOVE_FAILURE_FRAME:
			case CON_INSTR_IS:
			case CON_INSTR_FAIL_NOW:
			case CON_INSTR_POP:
			case CON_INSTR_IMPORT:
			case CON_INSTR_LIST:
			case CON_INSTR_APPLY:
			case CON_INSTR_FUNC_DEFN:
			case CON_INSTR_RETURN:
			case CON_INSTR_BRANCH:
			case CON_INSTR_YIELD:
			case CON_INSTR_DICT:
			case CON_INSTR_DUP:
			case CON_INSTR_PULL:
			case CON_INSTR_BUILTIN_LOOKUP:
			case CON_INSTR_EYIELD:
			case CON_INSTR_ADD_EXCEPTION_FRAME:
			case CON_INSTR_REMOVE_EXCEPTION_FRAME:
			case CON_INSTR_RAISE:
			case CON_INSTR_SET:
			case CON_INSTR_BRANCH_IF_NOT_FAIL:
			case CON_INSTR_BRANCH_IF_FAIL:
			case CON_INSTR_CONSTANT_GET:
			case CON_INSTR_CONSTANT_SET:
			case CON_INSTR_UNPACK_ASSIGN:
			case CON_INSTR_EQ:
			case CON_INSTR_NEQ:
			case CON_INSTR_GT:
			case CON_INSTR_LE:
			case CON_INSTR_LE_EQ:
			case CON_INSTR_GR_EQ:
			case CON_INSTR_ADD:
			case CON_INSTR_SUBTRACT:
				current_pc += sizeof(Con_Int);
				break;
			default:
				CON_XXX;
		}
		instruction_num += 1;
	}

	assert(current_pc == pc.pc.bytecode_offset);

#	define MODULE_GET_32BIT(x) (*(uint32_t*) (module_atom->module_bytecode + (x)))

	Con_Int src_info_pos = 0;
	Con_Int src_info_num = 0;
	while (1) {
		Con_Int src_info = MODULE_GET_32BIT(MODULE_GET_WORD(CON_BYTECODE_MODULE_SRC_POSITIONS) + src_info_pos * sizeof(uint32_t));

		if (src_info_num + (src_info & 0x000007) > instruction_num)
			break;

		src_info_num += src_info & 0x000007;

		while (src_info & (1 << 3)) {
			src_info_pos += 1;
			src_info = MODULE_GET_32BIT(MODULE_GET_WORD(CON_BYTECODE_MODULE_SRC_POSITIONS) + src_info_pos * sizeof(uint32_t));
		}
		src_info_pos += 1;
	}

	Con_Obj *imports = module_atom->imports;
	Con_Obj *self_file_name = module_atom->identifier;
	while (1) {
		uint32_t src_info = MODULE_GET_32BIT(MODULE_GET_WORD(CON_BYTECODE_MODULE_SRC_POSITIONS) + src_info_pos * sizeof(uint32_t));
		CON_MUTEX_UNLOCK(&module->mutex);
		Con_Obj *entry = Con_Builtins_List_Atom_new_sized(thread, 2);

		if ((src_info & 0x3FF0) >> 4 == (1 << 10) - 1)
			CON_GET_SLOT_APPLY(entry, "append", self_file_name);
		else {
			Con_Obj *import_file_name = CON_GET_SLOT_APPLY(imports, "get", CON_NEW_INT((src_info & 0x3FF0) >> 4));
			CON_GET_SLOT_APPLY(entry, "append", import_file_name);
		}
		CON_GET_SLOT_APPLY(entry, "append", CON_NEW_INT(src_info >> 14));

		CON_GET_SLOT_APPLY(src_locations, "append", entry);
		CON_MUTEX_LOCK(&module->mutex);

		if (!(src_info & (1 << 3)))
			break;

		src_info_pos += 1;
	}
	
	return src_locations;
}



//
//
//

void Con_Builtins_Module_Atom_src_offset_to_line_column(Con_Obj *thread, Con_Obj *module, Con_Int src_offset, Con_Int *line_num, Con_Int *column)
{
	Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&module->mutex);

	Con_Int j = 0;
	while (true) {
		if (src_offset < MODULE_GET_WORD(MODULE_GET_WORD(CON_BYTECODE_MODULE_NEWLINES) + j * sizeof(Con_Int))) {
			break;
		}
		j += 1;
	}
	CON_MUTEX_UNLOCK(&module->mutex);

	*line_num = j;
	*column = src_offset - MODULE_GET_WORD(MODULE_GET_WORD(CON_BYTECODE_MODULE_NEWLINES) + (j - 1) * sizeof(Con_Int));
}




Con_Obj	*Con_Builtins_Module_Atom_get_defn(Con_Obj *thread, Con_Obj *module, const u_char *defn_name, Con_Int defn_name_size)
{
	Con_Builtins_Module_Atom *module_atom = CON_FIND_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	
	if (module_atom == NULL) {
		Con_Obj *msg = CON_NEW_STRING("Trying to lookup a definition in a non-module.");
		CON_RAISE_EXCEPTION("VM_Exception", msg);
	}

	CON_MUTEX_LOCK(&module->mutex);
	Con_Obj *closure = module_atom->closure;
	Con_Obj *slot_val;
	if (!Con_Slots_get_slot(thread, &module_atom->top_level_vars, defn_name, defn_name_size, &slot_val))
		CON_RAISE_EXCEPTION("Mod_Defn_Exception", Con_Builtins_String_Atom_new_copy(thread, defn_name, defn_name_size, CON_STR_UTF_8));
	
	if (closure == NULL) {
		// Builtin module.
		if (slot_val == NULL)
			CON_XXX;
		return slot_val;
	}
	else { 
		// User module.
		return Con_Builtins_Closure_Atom_get_var(thread, closure, 0, Con_Numbers_Number_to_Con_Int(thread, slot_val));
	}
}



void Con_Builtins_Module_Atom_set_defn(Con_Obj *thread, Con_Obj *module, const u_char *defn_name, Con_Int defn_name_size, Con_Obj *val)
{
	Con_Builtins_Module_Atom *module_atom = CON_FIND_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	
	if (module_atom == NULL) {
		Con_Obj *msg = CON_NEW_STRING("Trying to lookup a definition in a non-module.");
		CON_RAISE_EXCEPTION("VM_Exception", msg);
	}

	CON_MUTEX_LOCK(&module->mutex);
	Con_Obj *closure = module_atom->closure;
	
	if (closure == NULL) {
		// Builtin module.
		CON_MUTEX_UNLOCK(&module->mutex);
#		ifndef NDEBUG
		Con_Obj *dummy;
		Con_Slots_get_slot(thread, &module_atom->top_level_vars, defn_name, defn_name_size, &dummy);
#		endif
		Con_Slots_set_slot(thread, &module->mutex, &module_atom->top_level_vars, defn_name, defn_name_size, val);
	}
	else { 
		// User module.
		Con_Obj *slot_val;
		if (!Con_Slots_get_slot(thread, &module_atom->top_level_vars, defn_name, defn_name_size, &slot_val))
			CON_XXX;
		CON_MUTEX_UNLOCK(&module->mutex);
		Con_Builtins_Closure_Atom_set_var(thread, closure, 0, Con_Numbers_Number_to_Con_Int(thread, slot_val), val);
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Module_Class_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Module_Atom *module_atom = (Con_Builtins_Module_Atom *) atom;

	if (module_atom->module_bytecode != NULL)
		Con_Memory_gc_push(thread, module_atom->module_bytecode);
	Con_Memory_gc_push(thread, module_atom->identifier);
	Con_Memory_gc_push(thread, module_atom->name);
	if (module_atom->closure != NULL)
		Con_Memory_gc_push(thread, module_atom->closure);
	Con_Memory_gc_push(thread, module_atom->imports);
	if (module_atom->init_func != NULL)
		Con_Memory_gc_push(thread, module_atom->init_func);
	Con_Slots_gc_scan_slots(thread, &module_atom->top_level_vars);
	
	if (module_atom->constants_create_offsets != NULL)
		Con_Memory_gc_push(thread, module_atom->constants_create_offsets);
	if (module_atom->constants != NULL)
		Con_Memory_gc_push(thread, module_atom->constants);
	// XXX there should probably be a mechanism to 'age' constants so they don't consume memory
	// permanently.
	for (Con_Int i = 0; i < module_atom->num_constants; i += 1) {
		if (module_atom->constants[i] != NULL)
			Con_Memory_gc_push(thread, module_atom->constants[i]);
	}
}
