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

#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#ifndef CON_HAVE_NATIVE_FGETLN
#	include "Platform/fgetln.h"
#endif

#include "Core.h"
#include "Modules.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Std_Modules.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Exception/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"




Con_Obj *Con_Module_POSIX_File_init(Con_Obj *, Con_Obj *);
Con_Obj *Con_Module_POSIX_File_import(Con_Obj *, Con_Obj *);

typedef struct {
	CON_ATOM_HEAD
	
	FILE *file;
} _Con_Module_POSIX_File_Atom;

void __Con_Module_POSIX_File_Atom_gc_clean_up(Con_Obj *, Con_Obj *, Con_Atom *);

void _Con_Module_POSIX_File_error(Con_Obj *, Con_Obj *, int);
void _Con_Module_POSIX_File_error_no_path(Con_Obj *, int);

Con_Obj *Con_Module_POSIX_File_Module_init(Con_Obj *thread, Con_Obj *);
Con_Obj *Con_Module_POSIX_File_Module_import(Con_Obj *thread, Con_Obj *);

Con_Obj *_Con_Module_POSIX_File_File_new_func(Con_Obj *);

Con_Obj *_Con_Module_POSIX_File_File_Class_close_func(Con_Obj *);
Con_Obj *_Con_Module_POSIX_File_File_Class_flush_func(Con_Obj *);
Con_Obj *_Con_Module_POSIX_File_File_Class_read_func(Con_Obj *);
Con_Obj *_Con_Module_POSIX_File_File_Class_readln_func(Con_Obj *);
Con_Obj *_Con_Module_POSIX_File_File_Class_write_func(Con_Obj *);
Con_Obj *_Con_Module_POSIX_File_File_Class_writeln_func(Con_Obj *);

Con_Obj *_Con_Module_POSIX_File_canon_path_func(Con_Obj *);
Con_Obj *_Con_Module_POSIX_File_chmod_func(Con_Obj *);
Con_Obj *_Con_Module_POSIX_File_exists_func(Con_Obj *);
Con_Obj *_Con_Module_POSIX_File_is_dir_func(Con_Obj *);
Con_Obj *_Con_Module_POSIX_File_is_file_func(Con_Obj *);
Con_Obj *_Con_Module_POSIX_File_iter_dir_entries_func(Con_Obj *);
Con_Obj *_Con_Module_POSIX_File_mtime_func(Con_Obj *);



Con_Obj *Con_Module_POSIX_File_init(Con_Obj *thread, Con_Obj *identifier)
{
	const char* defn_names[] = {"File_Atom_Def", "File", "canon_path", "exists", "is_dir", "is_file",  NULL};

	return Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("POSIX_File"), defn_names, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



Con_Obj *Con_Module_POSIX_File_import(Con_Obj *thread, Con_Obj *posix_file_mod)
{
	// File_Atom_Def
	
	CON_SET_MOD_DEFN(posix_file_mod, "File_Atom_Def", Con_Builtins_Atom_Def_Atom_new(thread, NULL, __Con_Module_POSIX_File_Atom_gc_clean_up));
	
	// POSIX_File.File
	
	Con_Obj *file_class = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("File"), Con_Builtins_List_Atom_new_va(thread, CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL), posix_file_mod, CON_NEW_UNBOUND_C_FUNC(_Con_Module_POSIX_File_File_new_func, "File_new", posix_file_mod));
	CON_SET_MOD_DEFN(posix_file_mod, "File", file_class);
	
	CON_SET_FIELD(file_class, "close", CON_NEW_BOUND_C_FUNC(_Con_Module_POSIX_File_File_Class_close_func, "close", posix_file_mod, file_class));
	CON_SET_FIELD(file_class, "flush", CON_NEW_BOUND_C_FUNC(_Con_Module_POSIX_File_File_Class_flush_func, "flush", posix_file_mod, file_class));
	CON_SET_FIELD(file_class, "read", CON_NEW_BOUND_C_FUNC(_Con_Module_POSIX_File_File_Class_read_func, "read", posix_file_mod, file_class));
	CON_SET_FIELD(file_class, "readln", CON_NEW_BOUND_C_FUNC(_Con_Module_POSIX_File_File_Class_readln_func, "readln", posix_file_mod, file_class));
	CON_SET_FIELD(file_class, "write", CON_NEW_BOUND_C_FUNC(_Con_Module_POSIX_File_File_Class_write_func, "write", posix_file_mod, file_class));
	CON_SET_FIELD(file_class, "writeln", CON_NEW_BOUND_C_FUNC(_Con_Module_POSIX_File_File_Class_writeln_func, "writeln", posix_file_mod, file_class));
	
	// Module-level functions
	
	CON_SET_MOD_DEFN(posix_file_mod, "canon_path", CON_NEW_UNBOUND_C_FUNC(_Con_Module_POSIX_File_canon_path_func, "canon_path", posix_file_mod));
	CON_SET_MOD_DEFN(posix_file_mod, "chmod", CON_NEW_UNBOUND_C_FUNC(_Con_Module_POSIX_File_chmod_func, "chmod", posix_file_mod));
	CON_SET_MOD_DEFN(posix_file_mod, "exists", CON_NEW_UNBOUND_C_FUNC(_Con_Module_POSIX_File_exists_func, "exists", posix_file_mod));
	CON_SET_MOD_DEFN(posix_file_mod, "is_dir", CON_NEW_UNBOUND_C_FUNC(_Con_Module_POSIX_File_is_dir_func, "is_dir", posix_file_mod));
	CON_SET_MOD_DEFN(posix_file_mod, "is_file", CON_NEW_UNBOUND_C_FUNC(_Con_Module_POSIX_File_is_file_func, "is_file", posix_file_mod));
	CON_SET_MOD_DEFN(posix_file_mod, "iter_dir_entries", CON_NEW_UNBOUND_C_FUNC(_Con_Module_POSIX_File_iter_dir_entries_func, "iter_dir_entries", posix_file_mod));
	CON_SET_MOD_DEFN(posix_file_mod, "mtime", CON_NEW_UNBOUND_C_FUNC(_Con_Module_POSIX_File_mtime_func, "mtime", posix_file_mod));
	
	return posix_file_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void __Con_Module_POSIX_File_Atom_gc_clean_up(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	_Con_Module_POSIX_File_Atom *file_atom = (_Con_Module_POSIX_File_Atom *) atom;

	if (file_atom->file != NULL) {
		// if the file is still open (i.e. non-NULL), we close it. Errors from fclose are ignored
		// as there's nothing sensible we can do with such errors during garbage collection.
		
		fclose(file_atom->file);
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//

void _Con_Module_POSIX_File_error(Con_Obj *thread, Con_Obj *path, int errnum)
{
	Con_Obj *msg = Con_Builtins_Exception_Atom_strerror(thread, errnum);
	msg = CON_ADD(msg, CON_NEW_STRING(" '"));
	msg = CON_ADD(msg, path);
	msg = CON_ADD(msg, CON_NEW_STRING("'."));
	CON_RAISE_EXCEPTION("File_Exception", msg);
}



void _Con_Module_POSIX_File_error_no_path(Con_Obj *thread, int errnum)
{
	Con_Obj *msg = Con_Builtins_Exception_Atom_strerror(thread, errnum);
	msg = CON_ADD(msg, CON_NEW_STRING("."));
	CON_RAISE_EXCEPTION("File_Exception", msg);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// class File
//

Con_Obj *_Con_Module_POSIX_File_File_new_func(Con_Obj *thread)
{
	Con_Obj *class_obj, *mode_obj, *path_obj;
	CON_UNPACK_ARGS("COS", &class_obj, &path_obj, &mode_obj);

	Con_Obj *new_file = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(_Con_Module_POSIX_File_Atom) + sizeof(Con_Builtins_Slots_Atom), class_obj);
	_Con_Module_POSIX_File_Atom *file_atom = (_Con_Module_POSIX_File_Atom *) new_file->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (file_atom + 1);
	file_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	file_atom->atom_type = CON_GET_MOD_DEFN(Con_Builtins_VM_Atom_get_functions_module(thread), "File_Atom_Def");

	Con_Atom *atom = path_obj->first_atom;
	while (atom != NULL) {
		if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT) || atom->atom_type == CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT))
			break;
		atom = atom->next_atom;
	}
	if (atom == NULL)
		CON_RAISE_EXCEPTION("Type_Exception", CON_NEW_STRING("[String, Int]"), path_obj, CON_NEW_STRING("path"));
	
	if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)) {
		if ((file_atom->file = fdopen(Con_Numbers_Number_to_c_Int(thread, path_obj), Con_Builtins_String_Atom_to_c_string(thread, mode_obj))) == NULL)
			CON_XXX;
	}
	else {
		if ((file_atom->file = fopen(Con_Builtins_String_Atom_to_c_string(thread, path_obj), Con_Builtins_String_Atom_to_c_string(thread, mode_obj))) == NULL) {
			_Con_Module_POSIX_File_error(thread, path_obj, errno);
		}
	}
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, new_file, CON_MEMORY_CHUNK_OBJ);
	
	return new_file;
}



//
// 'close()'. Returns null.
//

Con_Obj *_Con_Module_POSIX_File_File_Class_close_func(Con_Obj *thread)
{
	Con_Obj *file_atom_def = CON_GET_MOD_DEFN(Con_Builtins_VM_Atom_get_functions_module(thread), "File_Atom_Def");

	Con_Obj *self_obj;
	CON_UNPACK_ARGS("U", file_atom_def, &self_obj);
	
	_Con_Module_POSIX_File_Atom *file_atom = CON_GET_ATOM(self_obj, file_atom_def);
	
	if (file_atom->file == NULL)
		CON_RAISE_EXCEPTION("File_Exception", CON_NEW_STRING("File previously closed."));

	if (fclose(file_atom->file) != 0)
		_Con_Module_POSIX_File_error_no_path(thread, errno);
	
	file_atom->file = NULL;
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'flush()' attempts to flush this file.
//

Con_Obj *_Con_Module_POSIX_File_File_Class_flush_func(Con_Obj *thread)
{
	Con_Obj *file_atom_def = CON_GET_MOD_DEFN(Con_Builtins_VM_Atom_get_functions_module(thread), "File_Atom_Def");

	Con_Obj *self_obj;
	CON_UNPACK_ARGS("U", file_atom_def, &self_obj);

	_Con_Module_POSIX_File_Atom *file_atom = CON_GET_ATOM(self_obj, file_atom_def);

	if (file_atom->file == NULL)
		CON_RAISE_EXCEPTION("File_Exception", CON_NEW_STRING("File previously closed."));

	if (fflush(file_atom->file) != 0)
		_Con_Module_POSIX_File_error_no_path(thread, errno);

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'read(size := null)' reads 'size' bytes from the file; if size == -1, the entire contents of the
// file are read in.
//

Con_Obj *_Con_Module_POSIX_File_File_Class_read_func(Con_Obj *thread)
{
	Con_Obj *file_atom_def = CON_GET_MOD_DEFN(Con_Builtins_VM_Atom_get_functions_module(thread), "File_Atom_Def");

	Con_Obj *requested_size_obj, *self_obj;
	CON_UNPACK_ARGS("U;N", file_atom_def, &self_obj, &requested_size_obj);
	
	_Con_Module_POSIX_File_Atom *file_atom = CON_GET_ATOM(self_obj, file_atom_def);

	if (file_atom->file == NULL)
		CON_RAISE_EXCEPTION("File_Exception", CON_NEW_STRING("File previously closed."));

	struct stat file_stat;
	if (fstat(fileno(file_atom->file), &file_stat) == -1)
		CON_XXX;
	
	size_t data_size;
	if (requested_size_obj != NULL) {
		Con_Int user_size = Con_Numbers_Number_to_Con_Int(thread, requested_size_obj);
		if (user_size < 0)
			CON_RAISE_EXCEPTION("File_Exception", CON_NEW_STRING("Can not read less than 0 bytes from file."));
		data_size = user_size;
	}
	else
		data_size = file_stat.st_size - ftell(file_atom->file);

	u_char *data = Con_Memory_malloc(thread, data_size, CON_MEMORY_CHUNK_OPAQUE);
	size_t amount_read;
	if ((amount_read = fread(data, 1, data_size, file_atom->file)) != data_size) {
		if (ferror(file_atom->file))
			CON_XXX;
	}

	return Con_Builtins_String_Atom_new_no_copy(thread, data, data_size, CON_STR_UTF_8);
}



//
// 'readln()' is a generator which reads lines from a file, failing when no lines remain to be read.
//

Con_Obj *_Con_Module_POSIX_File_File_Class_readln_func(Con_Obj *thread)
{
	Con_Obj *file_atom_def = CON_GET_MOD_DEFN(Con_Builtins_VM_Atom_get_functions_module(thread), "File_Atom_Def");

	Con_Obj *requested_size_obj, *self_obj;
	CON_UNPACK_ARGS("U;N", file_atom_def, &self_obj, &requested_size_obj);
	
	_Con_Module_POSIX_File_Atom *file_atom = CON_GET_ATOM(self_obj, file_atom_def);

	if (file_atom->file == NULL)
		CON_RAISE_EXCEPTION("File_Exception", CON_NEW_STRING("File previously closed."));

	while (1) {
		size_t len;
		char *line = fgetln(file_atom->file, &len);
		if (line == NULL) {
			if (feof(file_atom->file) != 0)
				break;
			else if (ferror(file_atom->file) != 0)
				CON_XXX;
			else
				CON_XXX;
		}
		
		CON_YIELD(Con_Builtins_String_Atom_new_copy(thread, line, len, CON_STR_UTF_8));
	}

	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'write(s)' writes 's' to the file.
//

Con_Obj *_Con_Module_POSIX_File_File_Class_write_func(Con_Obj *thread)
{
	Con_Obj *file_atom_def = CON_GET_MOD_DEFN(Con_Builtins_VM_Atom_get_functions_module(thread), "File_Atom_Def");

	Con_Obj *s_obj, *self_obj;
	CON_UNPACK_ARGS("US", file_atom_def, &self_obj, &s_obj);
	
	_Con_Module_POSIX_File_Atom *file_atom = CON_GET_ATOM(self_obj, file_atom_def);

	if (file_atom->file == NULL)
		CON_RAISE_EXCEPTION("File_Exception", CON_NEW_STRING("File previously closed."));

	Con_Builtins_String_Atom *s_string_atom = CON_FIND_ATOM(s_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	if (s_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;

	if (fwrite(s_string_atom->str, s_string_atom->size, 1, file_atom->file) < 1)
		_Con_Module_POSIX_File_error_no_path(thread, errno);

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'writeln(s)' writes 's' to the file followed by a newline.
//

Con_Obj *_Con_Module_POSIX_File_File_Class_writeln_func(Con_Obj *thread)
{
	Con_Obj *file_atom_def = CON_GET_MOD_DEFN(Con_Builtins_VM_Atom_get_functions_module(thread), "File_Atom_Def");

	Con_Obj *s_obj, *self_obj;
	CON_UNPACK_ARGS("US", file_atom_def, &self_obj, &s_obj);
	
	_Con_Module_POSIX_File_Atom *file_atom = CON_GET_ATOM(self_obj, file_atom_def);

	if (file_atom->file == NULL)
		CON_RAISE_EXCEPTION("File_Exception", CON_NEW_STRING("File previously closed."));

	Con_Builtins_String_Atom *s_string_atom = CON_FIND_ATOM(s_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	if (s_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;

	if ((fwrite(s_string_atom->str, s_string_atom->size, 1, file_atom->file) < 1) ||
		(fwrite("\n", 1, 1, file_atom->file) < 1))
		_Con_Module_POSIX_File_error_no_path(thread, errno);

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}




////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in POSIX_File module
//

//
// 'canon_path(path)' returns a canonicalised version of 'path'.
//
// Note that (apparently) some operating systems may not canonacalise some paths; there's nothing
// that this function can do about that in such cases.
//

Con_Obj *_Con_Module_POSIX_File_canon_path_func(Con_Obj *thread)
{
	Con_Obj *path;
	CON_UNPACK_ARGS("S", &path);
	
	char resolved[PATH_MAX];
	if (realpath(Con_Builtins_String_Atom_to_c_string(thread, path), resolved) == NULL)
		_Con_Module_POSIX_File_error(thread, path, errno);

	return Con_Builtins_String_Atom_new_copy(thread, (u_char *) resolved, strlen(resolved), CON_STR_UTF_8);
}



//
// 'chmod(path)' returns a canonicalised version of 'path'.
//
// Note that (apparently) some operating systems may not canonacalise some paths; there's nothing
// that this function can do about that in such cases.
//

Con_Obj *_Con_Module_POSIX_File_chmod_func(Con_Obj *thread)
{
	Con_Obj *mode, *path;
	CON_UNPACK_ARGS("SN", &path, &mode);
	
	if (chmod(Con_Builtins_String_Atom_to_c_string(thread, path), Con_Numbers_Number_to_Con_Int(thread, mode)) == -1)
		_Con_Module_POSIX_File_error(thread, path, errno);

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'exists(path)' returns null if 'path' exists, or fails otherwise.
//

Con_Obj *_Con_Module_POSIX_File_exists_func(Con_Obj *thread)
{
	Con_Obj *path;
	CON_UNPACK_ARGS("S", &path);
	
	struct stat sb;
	
	int rtn;
	if ((rtn = stat(Con_Builtins_String_Atom_to_c_string(thread, path), &sb)) != 0) {
		if (errno == ENOENT)
			return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
		else
			_Con_Module_POSIX_File_error(thread, path, errno);
	}

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'is_dir(path)' returns null if 'path' is a directory, or fails otherwise.
//
// Note that an exception is raised if 'path' does not exist.
//

Con_Obj *_Con_Module_POSIX_File_is_dir_func(Con_Obj *thread)
{
	Con_Obj *path;
	CON_UNPACK_ARGS("S", &path);
	
	struct stat sb;
	
	int rtn;
	if ((rtn = stat(Con_Builtins_String_Atom_to_c_string(thread, path), &sb)) != 0)
		_Con_Module_POSIX_File_error(thread, path, errno);
	
	if (sb.st_mode & S_IFDIR)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}




//
// 'is_file(path)' returns null if 'path' is a normal file, or fails otherwise.
//
// Note that an exception is raised if 'path' does not exist.
//

Con_Obj *_Con_Module_POSIX_File_is_file_func(Con_Obj *thread)
{
	Con_Obj *path;
	CON_UNPACK_ARGS("S", &path);
	
	struct stat sb;
	
	int rtn;
	if ((rtn = stat(Con_Builtins_String_Atom_to_c_string(thread, path), &sb)) != 0)
		_Con_Module_POSIX_File_error(thread, path, errno);
	
	if (sb.st_mode & S_IFREG)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// Successively generates each leaf name in 'dir_path'.
//

Con_Obj *_Con_Module_POSIX_File_iter_dir_entries_func(Con_Obj *thread)
{
	Con_Obj *dir_path;
	CON_UNPACK_ARGS("S", &dir_path);
	
	DIR *dir = opendir(Con_Builtins_String_Atom_to_c_string(thread, dir_path));
	if (dir == NULL)
		_Con_Module_POSIX_File_error(thread, dir_path, errno);
	
	while (1) {
		errno = 0;
		struct dirent *de = readdir(dir);
		if (de == NULL) {
			if (errno == 0)
				break;
			else
				_Con_Module_POSIX_File_error(thread, dir_path, errno);
		}

#		ifdef CON_PLATFORM_POSIX
		// We strip out the "." and ".." entries if they appear, as they are a portability headache
		// waiting to happen.

		if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
			continue;
#		endif

		CON_YIELD(Con_Builtins_String_Atom_new_copy(thread, de->d_name, strlen(de->d_name), CON_STR_UTF_8));
	}

	closedir(dir);

	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);	
}



//
// Successively generates each leaf name in 'dir_path'.
//

Con_Obj *_Con_Module_POSIX_File_mtime_func(Con_Obj *thread)
{
	Con_Obj *path;
	CON_UNPACK_ARGS("S", &path);
	
	struct stat sb;
	
	int rtn;
	if ((rtn = stat(Con_Builtins_String_Atom_to_c_string(thread, path), &sb)) != 0)
		_Con_Module_POSIX_File_error(thread, path, errno);
	
	Con_Obj *time_mod = Con_Modules_import(thread, Con_Modules_get(thread, CON_NEW_STRING(CON_MOD_ID_TIME)));
	
	Con_Obj *sec = CON_NEW_INT(sb.STAT_ST_MTIMESPEC.tv_sec);
	Con_Obj *nsec = CON_NEW_INT(sb.STAT_ST_MTIMESPEC.tv_nsec);
	
	return CON_APPLY(CON_GET_MOD_DEFN(time_mod, "mk_timespec"), sec, nsec);
}
