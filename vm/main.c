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
#ifdef CON_PLATFORM_POSIX
#	include <sys/wait.h>
#endif
#ifdef CON_PLATFORM_MINGW
#	include "Platform/MinGW/realpath.h"
#	include <windows.h>
#	include <winsock2.h>
#endif

#ifdef CON_HAVE_NATIVE_ERR
#include <err.h>
#else
#include "Platform/err.h"
#endif
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Arch.h"
#include "Bootstrap.h"
#include "Bytecode.h"
#include "Core.h"
#include "Modules.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"
#include "Std_Modules.h"
#include "Version.h"

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



// How many bytes into a file do we check to see if it is a Converge executeable.

#define BYTES_TO_FIND_EXEC 1024

#define COMPILER_CND_DIRS {".." CON_DIR_SEP "lib" CON_DIR_SEP "converge-" CON_VERSION CON_DIR_SEP, \
    ".." CON_DIR_SEP "compiler" CON_DIR_SEP, NULL}
#define STDLIB_CND_DIRS {".." CON_DIR_SEP "lib" CON_DIR_SEP "converge-" CON_VERSION CON_DIR_SEP, \
    ".." CON_DIR_SEP "lib" CON_DIR_SEP, NULL}


#ifdef CON_DEFINE___PROGNAME
extern char* __progname;
#else
char *__progname;
#endif

int main_do(int, char **, u_char *);
void usage(void);
char *find_con_exec(const char *, const char *);
void import_con_lib(Con_Obj *, const char *, const char**, const char *);
ssize_t find_bytecode_start(u_char *, size_t);
void make_mode(char *, u_char **, size_t *, char *, int, bool);



int main(int argc, char** argv)
{
	// Because we can't be sure exactly where in the local stack the root_stack_start variable would
	// be if this function defined other variables, we stay on the safe side and call main_do; this
	// means we can be confident that root_stack_start points close enough to the root of the stack
	// for our purposes since it must reside before main_do's stack entry.

	u_char *root_stack_start;
	CON_ARCH_GET_STACKP(root_stack_start);
	
	return main_do(argc, argv, root_stack_start);
}



int main_do(int argc, char** argv, u_char *root_stack_start)
{
	assert(argc > 0);

	// We define here all variables which contain GC'd objects; these will be explicitly set to NULL
	// so that we can make a reasonable stab at freeing all the memory we've allocated. We define
	// all these variables here so that we can track them.

	Con_Obj *thread, *main_module_identifier, *exception, *main_module, *backtrace;
	char *vm_path, *canon_prog_path;
	u_char *bytecode;
	
	// And now the rest of the function...
	
#	ifndef CON_DEFINE___PROGNAME
	__progname = argv[0];
#	endif

	char *argv0 = argv[0];

	int verbosity = 0;
	bool mk_fresh = false;

	// We include our own simplistic arg parsing code to avoid platform problems with getopt etc.

	int i = 1;
	while (i < argc) {
		char *arg = argv[i];
		if (arg[0] == '-') {
			if (strlen(arg) == 1)
				usage();

			for (unsigned int j = 1; j < strlen(arg); j += 1) {
				switch (arg[j]) {
					case 'v':
						verbosity += 1;
						break;
					case 'f':
						mk_fresh = true;
						break;
					default:
						usage();
				}
			}
		}
		else
			break;

		i += 1;
	}
	argc -= i;
	argv += i;

#	if CON_FULL_DEBUG
		printf("%s %s (%s) %s\n", CON_NAME, CON_VERSION, CON_DATE, CON_COPYRIGHT);
		printf("stack root %p\n", root_stack_start);
#	endif

#	ifdef CON_PLATFORM_MINGW
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		CON_XXX;
#	endif

	// We want to determine the path of the VM executable, when possible, to later populate
	// Sys::vm_path. This is one of those areas where UNIX falls down badly. On Linux, we
	// try reading the symlink /proc/self/exe which should point to the VM executable. On
	// other UNIXes we accept the canonicalised version of argv[0] if it appears to be a real file;
	// otherwise we search through $PATH and see if we can work out where we were called from. None
	// of this seems particularly reliable, but it's not clear that there's any better mechanism.

	struct stat tmp_stat;
	vm_path = NULL;

#	ifdef READ_VM_PATH_FROM_PROC_SELF_EXE_SYMLINK
	// On Linux we first try reading the symlink /proc/self/exe. Since this seems an inherently
	// dodgy proposition, we don't rely on this definitely succeeding, and fall back on our
	// other mechanism if it doesn't work.

	if ((vm_path = malloc(PATH_MAX)) == NULL)
        err(1, "malloc");
    
	int rtn = readlink("/proc/self/exe", vm_path, PATH_MAX);
	if (rtn != -1) {
		vm_path[rtn] = '\0';
		if (stat(vm_path, &tmp_stat) == -1) {
			free(vm_path);
			vm_path = NULL;
		}
	}
	else {
		free(vm_path);
		vm_path = NULL;
	}
#	endif

#	ifdef CON_PLATFORM_MINGW
	// On Windows, GetModuleFileName appears to always return a canonicalised path of the
	// executable path, which is doubly nice because its possible to execute "x.exe" as simply
	// just "x", and that would make working this out painful in the extreme.
	if ((vm_path = malloc(PATH_MAX)) == NULL)
        err(1, "malloc");

	if (GetModuleFileName(NULL, vm_path, PATH_MAX) == PATH_MAX || stat(vm_path, &tmp_stat) == -1) {
		free(vm_path);
		vm_path = NULL;
	}
#	endif

	if (vm_path == NULL) {
	    if ((vm_path = malloc(PATH_MAX)) == NULL)
            err(1, "malloc");

		if (realpath(argv0, vm_path) == NULL || stat(vm_path, &tmp_stat) != 0) {
#		ifdef CON_PLATFORM_POSIX
			// Since realpath failed, we fall back on searching through $PATH and try and find
			// the executable on it.
			char *path;
			// If getenv returns NULL, we're out of ideas.
			if ((path = getenv("PATH")) != NULL) {
				char *cnd;
	            if ((cnd = malloc(PATH_MAX)) == NULL)
                    err(1, "malloc");

				size_t i = 0;
				while (1) {
					size_t j;
					for (j = i; j < strlen(path); j += 1) {
						if (path[j] == ':')
							break;
					}

					memmove(cnd, path + i, j - i);
					if (cnd[j - i - 1] != '/') {
						cnd[j - i] = '/';
						cnd[j - i + 1] = '\0';
					}
					else
						cnd[j - i] = '\0';
					if (strlcat(cnd, argv0, PATH_MAX - (j - i)) > PATH_MAX - (j - i))
						CON_XXX;

					if (realpath(cnd, vm_path) != NULL && stat(vm_path, &tmp_stat) == 0)
						break;

					if (j == i)
						break;

					i = j + 1;
				}
				free(cnd);
				cnd = NULL;
			}
#		else
			// On non-POSIX platforms, we have no real idea what to do if realpath didn't give us
			// a good answer.
			free(vm_path);
			vm_path = NULL;
#		endif
		}
	}

	char *prog_path;
	if (argc == 0) {
		// If we've been called without arguments, we try to load convergei.
		prog_path = find_con_exec("convergei", vm_path);
	}
	else
		prog_path = argv[0];
	
	if ((canon_prog_path = malloc(PATH_MAX)) == NULL)
        err(1, "malloc");

	if (realpath(prog_path, canon_prog_path) == NULL) {
		// If we can't canonicalise the programs path name, we fall back on the "raw" path and cross
		// our fingers.
		if (strlcpy(canon_prog_path, prog_path, PATH_MAX) > PATH_MAX)
			CON_XXX;
	}
	
	struct stat con_binary_file_stat;
	if (stat(prog_path, &con_binary_file_stat) == -1) {
		err(1, ": trying to stat '%s'", prog_path);
	}
	
	FILE *con_binary_file = fopen(prog_path, "rb");
	if (con_binary_file == NULL) {
		err(1, ": unable to open '%s", prog_path);
		exit(1);
	}

	thread = Con_Bootstrap_do(root_stack_start, argc, argv, vm_path, canon_prog_path);
	if (thread == NULL) {
		fprintf(stderr, "%s: error when trying to initialize virtual machine.\n", __progname);
		exit(1);
	}
	
	size_t bytecode_size = con_binary_file_stat.st_size;
	if ((bytecode = malloc(bytecode_size)) == NULL)
        err(1, "malloc");

	fread(bytecode, 1, bytecode_size, con_binary_file);

	if (fclose(con_binary_file) != 0) {
		err(1, ": error closing '%s'", prog_path);
		exit(1);
	}

	// We now see if we've been given something that appears to be an executeable or a source file;
	// if the latter we go into make mode.
	
	ssize_t bytecode_start = find_bytecode_start(bytecode, bytecode_size);
	if (bytecode_start == -1) {
		make_mode(prog_path, &bytecode, &bytecode_size, vm_path, verbosity, mk_fresh);
		bytecode_start = find_bytecode_start(bytecode, bytecode_size);
		if (bytecode_start == -1) {
			fprintf(stderr, "convergec does not appear to have produced an executeable.\n");
			exit(1);
		}
	}
	
	main_module_identifier = Con_Bytecode_add_executable(thread, bytecode + bytecode_start);
	free(bytecode);
	bytecode = NULL;
    
    const char *stdlib_cnd_dirs[] = STDLIB_CND_DIRS;
    import_con_lib(thread, "Stdlib.cvl", stdlib_cnd_dirs, vm_path);
    const char *compiler_cnd_dirs[] = COMPILER_CND_DIRS;
    import_con_lib(thread, "Compiler.cvl", compiler_cnd_dirs, vm_path);
	
	CON_TRY {
		main_module = Con_Modules_import(thread, Con_Modules_get(thread, main_module_identifier));
		CON_APPLY(CON_GET_MOD_DEFN(main_module, "main"));
	}
	CON_CATCH(exception) {
		Con_Int exit_code;
		
		// We check to see if this exception is an instance of System_Exit_Exception; if so, we
		// don't print a backtrace.
		
		if (CON_GET_SLOT_APPLY_NO_FAIL(CON_GET_MOD_DEFN(CON_BUILTIN(CON_BUILTIN_EXCEPTIONS_MODULE), "System_Exit_Exception"), "instantiated", exception) != NULL)
			exit_code = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT(exception, "code"));
		else {
			backtrace = CON_APPLY(CON_GET_MOD_DEFN(
              Con_Modules_import(thread, Con_Modules_get_stdlib(thread, CON_STDLIB_BACKTRACE)),
              "print_best"), exception);
			backtrace = NULL;
			exit_code = 1;
		}
		
#ifndef CON_THREADS_SINGLE_THREADED
		// At this point the main thread has finished. We can't let the application finish until all
		// other threads have finished. We therefore can't have it removed from the thread pool as
		// otherwise bits of this thread (the Con_Stack etc) might start being freed by the garbage
		// collector.

		Con_Builtins_Thread_Atom_mark_as_finished(thread);

		Con_Builtins_Thread_Atom_join_all(thread);
#endif

		// Clear up as much memory as we can. There will still be some live objects after this has
		// executed, but they should mostly (if not all) be objects created during the bootstrap
		// process so hopefully aren't too important.

		main_module_identifier = exception = NULL;
		Con_Memory_gc_force(thread);
		
		exit(exit_code);
	}
	CON_TRY_END

#ifndef CON_THREADS_SINGLE_THREADED
	// At this point the main thread has finished. We can't let the application finish until all
	// other threads have finished. We therefore can't have it removed from the thread pool as
	// otherwise bits of this thread (the Con_Stack etc) might start being freed by the garbage
	// collector.

	Con_Builtins_Thread_Atom_mark_as_finished(thread);

	Con_Builtins_Thread_Atom_join_all(thread);
#endif

	// Clear up as much memory as we can. There will still be some live objects after this has
	// executed, but they should mostly (if not all) be objects created during the bootstrap
	// process so hopefully aren't too important.

	main_module_identifier = exception = main_module = backtrace = NULL;
	if (vm_path != NULL) {
		free(vm_path);
		vm_path = NULL;
	}
	if (canon_prog_path != NULL) {
		free(canon_prog_path);
		canon_prog_path = NULL;
	}
	Con_Memory_gc_force(thread);
	
	return 0;
}



void usage()
{
	fprintf(stderr, "Usage: %s [-vf] [source file | executeable file\n", __progname);
	exit(1);
}



//
// Return a malloc'd string of the canonicalised path of the Converge executeable 'name'. 'vm_path'
// should be the canonicalised pathname of the VM so that this function has some chance of finding
// other executeables.
//

char *find_con_exec(const char *name, const char *vm_path)
{
	char *prog_path = NULL;
	if (vm_path != NULL) {
		const char* cnds[] = {"", ".." CON_DIR_SEP "compiler" CON_DIR_SEP, NULL};
		char *cnd, *canon_cnd;
	    if ((cnd = malloc(PATH_MAX)) == NULL)
            err(1, "malloc");

	    if ((canon_cnd = malloc(PATH_MAX)) == NULL)
            err(1, "malloc");

		int i;
		for (i = 0; cnds[i] != NULL; i += 1) {
			if (strlcpy(cnd, vm_path, PATH_MAX) > PATH_MAX)
				CON_XXX;
			int j;
			for (j = strlen(cnd) - 1; j >= 0
			  && strlen(cnd) - j >= strlen(CON_DIR_SEP)
			  && memcmp(cnd + j, CON_DIR_SEP, strlen(CON_DIR_SEP)) != 0
			  ; j -= 1) {}
			if (memcmp(cnd + j, CON_DIR_SEP, strlen(CON_DIR_SEP)) == 0) {
				if (strlcpy(cnd + j + strlen(CON_DIR_SEP), cnds[i],
					PATH_MAX - j - strlen(CON_DIR_SEP)) >= PATH_MAX - j - strlen(CON_DIR_SEP))
					CON_XXX;
				j += strlen(cnds[i]) + strlen(CON_DIR_SEP);
			}
			else {
				if (strlcpy(cnd + j, cnds[i], PATH_MAX - j) > (size_t) (PATH_MAX - j))
					CON_XXX;
				j += strlen(cnds[i]);
			}
			if (strlcpy(cnd + j, name, PATH_MAX - j) > (size_t) (PATH_MAX - j))
				CON_XXX;
			struct stat tmp_stat;
			if (realpath(cnd, canon_cnd) != NULL && stat(canon_cnd, &tmp_stat) == 0)
				break;
		}
		free(cnd);
		if (cnds[i] == NULL)
			free(canon_cnd);
		else {
			prog_path = canon_cnd;
		}
	}
	if (prog_path == NULL) {
		fprintf(stderr, "%s: unable to locate %s.\n", __progname, name);
		exit(1);
	}
	
	return prog_path;
}



//
// Return a malloc'd string of the canonicalised path of the Converge executeable 'name'. 'vm_path'
// should be the canonicalised pathname of the VM so that this function has some chance of finding
// other executeables.
//

void import_con_lib(Con_Obj *thread, const char *leaf, const char** cnd_dirs, const char *vm_path)
{
	if (vm_path == NULL)
        return;
    
    char *lib_path = NULL;
    char *cnd, *canon_cnd;
	if ((cnd = malloc(PATH_MAX)) == NULL)
        err(1, "malloc");

	if ((canon_cnd = malloc(PATH_MAX)) == NULL)
        err(1, "malloc");

    int i;
    for (i = 0; cnd_dirs[i] != NULL; i += 1) {
        if (strlcpy(cnd, vm_path, PATH_MAX) > PATH_MAX)
            CON_XXX;
        int j;
        for (j = strlen(cnd) - 1; j >= 0
          && strlen(cnd) - j >= strlen(CON_DIR_SEP)
          && memcmp(cnd + j, CON_DIR_SEP, strlen(CON_DIR_SEP)) != 0
          ; j -= 1) {}
        if (memcmp(cnd + j, CON_DIR_SEP, strlen(CON_DIR_SEP)) == 0) {
            if (strlcpy(cnd + j + strlen(CON_DIR_SEP), cnd_dirs[i],
                PATH_MAX - j - strlen(CON_DIR_SEP)) >= PATH_MAX - j - strlen(CON_DIR_SEP))
                CON_XXX;
            j += strlen(cnd_dirs[i]) + strlen(CON_DIR_SEP);
        }
        else {
            if (strlcpy(cnd + j, cnd_dirs[i], PATH_MAX - j) > (size_t) (PATH_MAX - j))
                CON_XXX;
            j += strlen(cnd_dirs[i]);
        }
        if (strlcpy(cnd + j, leaf, PATH_MAX - j) > (size_t) (PATH_MAX - j))
            CON_XXX;
        struct stat tmp_stat;
        if (realpath(cnd, canon_cnd) != NULL && stat(canon_cnd, &tmp_stat) == 0)
            break;
    }
    free(cnd);
    if (cnd_dirs[i] == NULL)
        free(canon_cnd);
    else {
        lib_path = canon_cnd;
    }
    
    if (lib_path == NULL)
        return;

	struct stat st;
	if (stat(lib_path, &st) == -1) {
		err(1, ": trying to stat '%s'", lib_path);
	}

	FILE *lib_file = fopen(lib_path, "rb");
	if (lib_file == NULL) {
		err(1, ": unable to open '%s", lib_path);
		exit(1);
	}

	size_t lib_size = st.st_size;
	u_char *bc;
    if ((bc = malloc(lib_size)) == NULL)
        err(1, "malloc");
	fread(bc, 1, lib_size, lib_file);

	if (fclose(lib_file) != 0) {
		err(1, ": error closing '%s'", lib_path);
		exit(1);
	}
    
    Con_Bytecode_add_lib(thread, bc);
    
    free(bc);
}



//
// Find the start of an executeable within 'bytecode', or return -1 if an executeable is not fonud.
//

ssize_t find_bytecode_start(u_char *bytecode, size_t bytecode_size)
{
	ssize_t bytecode_start = 0;
	while (bytecode_size - bytecode_start >= 8 && bytecode_start < BYTES_TO_FIND_EXEC) {
		if (memcmp(bytecode + bytecode_start, "CONVEXEC", 8) == 0)
			return bytecode_start;
		
		bytecode_start += 1;
	}

	return -1;
}



//
// Compile and link 'prog_path'.
//

void make_mode(char *prog_path, u_char **bytecode, size_t *bytecode_size, char *vm_path, int verbosity, bool mk_fresh)
{
	// First of all we try and work out if we can construct a cached path name.

	char cache_path[PATH_MAX];
	bool have_cache_path = false;
	struct stat st;
	int i;
	for (i = strlen(prog_path) - 1; i >= 0 && prog_path[i] != '.'; i -= 1) {}
	FILE *cache_file;
	if (i >= 0) {
		have_cache_path = true;
		memmove(cache_path, prog_path, i);
		cache_path[i] = '\0';
	}

	// If we've constructed a cached path name, and unless fresh compilation is being forced, we see
	// if we can read in the cached bytecode; if it's upto date we'll use it as is.

	if (have_cache_path && !mk_fresh) {
		if (stat(cache_path, &st) == -1)
			goto make;
        
        if (st.st_size == 0) {
            unlink(cache_path);
            goto make;
        }

		// There is a cached path, so now we try and load it and see if it is upto date. If any part
		// of this fails, we simply go straight to full make mode.
		
		*bytecode_size = st.st_size;
		*bytecode = realloc(*bytecode, *bytecode_size);
		if (*bytecode == NULL)
			CON_XXX;
	
		cache_file = fopen(cache_path, "rb");
		if (cache_file == NULL)
			goto make;
		
		if (fread(*bytecode, 1, *bytecode_size, cache_file) < *bytecode_size) {
			if (ferror(cache_file) != 0) {
				fclose(cache_file); // Ignore errors
				goto make;
			}
		}
		fclose(cache_file); // Ignore errors

		ssize_t bytecode_start = find_bytecode_start(*bytecode, *bytecode_size);
		if (bytecode_start != -1 && Con_Bytecode_upto_date(NULL, *bytecode + bytecode_start, &st.STAT_ST_MTIMESPEC)) {
			// The cached file is completely upto date. We don't need to do anything else.
			return;
		}
	}

#	ifdef CON_PLATFORM_MINGW
	// XXX On MinGW pipes seem too complicated, so we take a simpler approach and dump
	// everything out to a temporary file. This is ugly and someone should fix it.
	
	char *convergec_path;
	// GCC 3.3.5 (at least) barfs if a label comes before a variable declaration.
make:	
	convergec_path = find_con_exec("convergec", vm_path);
	char tmp_path[PATH_MAX];
	if (GetTempFileName(".", "con", 1, tmp_path) == 0)
		CON_XXX;

	const char *first_args[] = {vm_path, convergec_path, "-m", "-o", tmp_path, (char *) NULL};
	int num_first_args = verbosity;
	if (mk_fresh)
		num_first_args += 1;
	for (int i = 0; first_args[i] != NULL; i += 1) {
		num_first_args += 1;
	}

	const char *args[num_first_args + verbosity + 1];
	int j = 0;
	for (int i = 0; first_args[j] != NULL; i += 1) {
		args[j] = first_args[j];
		j += 1;
	}
	for (int i = 0; i < verbosity; i += 1) {
		args[j++] = "-v";
	}
	if (mk_fresh)
		args[j++] = "-f";
	args[j++] = prog_path;
	args[j++] = NULL;
	
	int rtn = _spawnv(_P_WAIT, vm_path, args);
	if (rtn != 0)
		exit(rtn);
	
	struct stat tmp_stat;
	if (stat(tmp_path, &tmp_stat) != 0)
		CON_XXX;
	
	*bytecode_size = tmp_stat.st_size;
	*bytecode = realloc(*bytecode, *bytecode_size);
	FILE *tmp_file = fopen(tmp_path, "rb");
	if (tmp_file == NULL)
		CON_XXX;

	if (fread(*bytecode, 1, tmp_stat.st_size, tmp_file) < (size_t) tmp_stat.st_size)
		CON_XXX;
	
	// Ignore errors when closing and deleting the temporary file - errors are annoying, but not
	// fatal.
	
	fclose(tmp_file);
	unlink(tmp_path);
	
#	else

	// GCC 3.3.5 (at least) barfs if a label comes before a variable declaration.
	int filedes[2];
make:
	// Fire up convergec -m on progpath. We do this by creating a pipe, forking, getting the child
	// to output to the pipe (although note that we leave stdin and stdout unmolested on the child
	// process, as user programs might want to print stuff to screen) and reading from that pipe
	// to get the necessary bytecode.

	pipe(filedes);
	
	pid_t pid = fork();
	if (pid == -1)
		CON_XXX;
	else if (pid == 0) {
		// Child process.
		char *convergec_path = find_con_exec("convergec", vm_path);
		char fd_path[PATH_MAX];
		if (snprintf(fd_path, PATH_MAX, "/dev/fd/%d", filedes[1]) >= PATH_MAX)
			CON_XXX;

		const char *first_args[] = {vm_path, convergec_path, "-m", "-o", fd_path, (char *) NULL};
		int num_first_args = verbosity;
		if (mk_fresh)
			num_first_args += 1;
		for (int i = 0; first_args[i] != NULL; i += 1) {
			num_first_args += 1;
		}
		
		const char *args[num_first_args + verbosity + 1];
		int j = 0;
		for (int i = 0; first_args[j] != NULL; i += 1) {
			args[j] = first_args[j];
			j += 1;
		}
		for (int i = 0; i < verbosity; i += 1) {
			args[j++] = "-v";
		}
		if (mk_fresh)
			args[j++] = "-f";
		args[j++] = prog_path;
		args[j++] = NULL;
		execv(vm_path, (char **const) args);
		err(1, ": trying to execv convergec");
	}
	
	// Parent process.
	
	close(filedes[1]);

	// Read in the output from the child process.

	i = sizeof(Con_Int) * 2 * *bytecode_size;
	if (i < BUFSIZ)
		i = BUFSIZ;
	*bytecode_size = 0;
	if ((*bytecode = malloc(i)) == NULL)
        err(1, "malloc");
	while (1) {
		int rtn = read(filedes[0], *bytecode + *bytecode_size, i);
		if (rtn == 0)
			break;
		else if (rtn == -1)
			err(1, ": error reading");
		else {
			*bytecode_size += rtn;
			i += rtn;
			*bytecode = realloc(*bytecode, *bytecode_size + i);
			if (*bytecode == NULL)
				CON_XXX;
		}
	}
	
	// Now we've read all the data from the child convergec, we check its return status; if it
	// returned something other than 0 then we return that value and do not continue.
	
	int status;
	waitpid(pid, &status, 0);
	if (WIFEXITED(status)) {
		int child_rtn = WEXITSTATUS(status);
		if (child_rtn != 0)
			exit(child_rtn);
	}

	// Since we might have grossly over-guessed how much memory to allocate, we now resize to the
	// correct value.

	*bytecode = realloc(*bytecode, *bytecode_size);
	if (*bytecode == NULL)
		CON_XXX;
#	endif

	// Try and write the file to its cached equivalent. Since this isn't strictly necessary, if at
	// any point anything fails, we simply give up without reporting an error.

	if (have_cache_path) {
		FILE *cache_file;

        // Note that if the potentially cached file is 0 bytes, we treat it as if it doesn't exist
        // and merrily overwrite it.
		if (stat(cache_path, &st) != -1 && st.st_size > 0) {
			// As a file with the cached name already exists, we want to ensure that it's a Converge
			// executeable to be reasonably sure that we're not overwriting a normal user file.

			FILE *cache_file = fopen(cache_path, "rb");
			if (cache_file == NULL)
				return;

			u_char *tmp;
            if ((tmp = malloc(BYTES_TO_FIND_EXEC)) == NULL)
                err(1, "malloc");
			int read = fread(tmp, 1, BYTES_TO_FIND_EXEC, cache_file);
			if (read < BYTES_TO_FIND_EXEC && ferror(cache_file)) {
				free(tmp);
				return;
			}

			if (find_bytecode_start(tmp, read) == -1) {
				free(tmp);
				return;
			}

			free(tmp);
			fclose(cache_file);
		}

		cache_file = fopen(cache_path, "wb");
		if (cache_file == NULL)
			return;
		if (fwrite(*bytecode, *bytecode_size, 1, cache_file) < 1) {
			fclose(cache_file); // Ignore errors.
			unlink(cache_path);
			return;
		}
		fclose(cache_file); // Ignore errors.
	}
}
