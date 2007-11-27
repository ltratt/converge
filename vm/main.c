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

#ifdef CON_HAVE_NATIVE_ERR
#include <err.h>
#else
#include "Platform/err.h"
#endif
#include <libgen.h>
#include <setjmp.h>
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
#include "Version.h"

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



// How many bytes into a file do we check to see if it is a Converge executeable.

#define BYTES_TO_FIND_EXEC 1024


#ifdef CON_DEFINE___PROGNAME
extern char* __progname;
#else
char *__progname;
#endif

int main_do(int, char **, u_char *);
void usage(void);
char *find_con_exec(const char *, const char *);
ssize_t find_bytecode_start(u_char *, size_t);
void make_mode(char *, u_char **, size_t *, char *, int);



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
	
#	ifndef CON_DEFINE___PROGNAME
	__progname = argv[0];
#	endif

	char *argv0 = argv[0];

	int verbosity = 0;

	int i = 1;
	while (i < argc) {
		if (argv[i][0] == '-') {
			if (strlen(argv[i]) == 1 || strlen(argv[i]) > 2)
				usage();

			switch (argv[i][1]) {
				case 'v':
					verbosity += 1;
					i += 1;
					break;
				default:
					usage();
			}
		}
		else
			break;
	}
	argc -= i;
	argv += i;

#if CON_FULL_DEBUG
		printf("%s %s (%s) %s\n", CON_NAME, CON_VERSION, CON_DATE, CON_COPYRIGHT);
		printf("stack root %p\n", root_stack_start);
#endif

	// We want to determine the path of the VM executable, when possible, to later populate
	// Sys::vm_path. This is one of those areas where UNIX falls down badly. On Linux, we
	// try reading the symlink /proc/self/exe which should point to the VM executable. On
	// other UNIXes we accept the canonicalised version of argv[0] if it appears to be a real file;
	// otherwise we search through $PATH and see if we can work out where we were called from. None
	// of this seems particularly reliable, but it's not clear that there's any better mechanism.

	struct stat tmp_stat;
	char *vm_path = NULL;

#ifdef READ_VM_PATH_FROM_PROC_SELF_EXE_SYMLINK
	// On Linux we first try reading the symlink /proc/self/exe. Since this seems an inherently
	// dodgy proposition, we don't rely on this definitely succeeding, and fall back on our
	// other mechanism if it doesn't work.

	vm_path = malloc(PATH_MAX);
	if (readlink("/proc/self/exe", vm_path, PATH_MAX) == -1 || stat(vm_path, &tmp_stat) == 0)
		vm_path = NULL;
#endif

	if (vm_path == NULL) {
		vm_path = malloc(PATH_MAX);
		if (realpath(argv0, vm_path) == NULL || stat(vm_path, &tmp_stat) != 0) {
			// Since realpath failed, we fall back on searching through $PATH and try and find
			// the executable on it.
			char *path;
			// If getenv returns NULL, we're out of ideas.
			if ((path = getenv("PATH")) != NULL) {
				char *cnd = malloc(PATH_MAX);
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
					strcat(cnd, argv0);

					if (realpath(cnd, vm_path) != NULL && stat(vm_path, &tmp_stat) == 0)
						break;

					if (j == i)
						break;

					i = j + 1;
				}
				free(cnd);
			}
		}
	}

	char *prog_path;
	if (argc == 0) {
		// If we've been called without arguments, we try to load convergei.
		prog_path = find_con_exec("convergei", vm_path);
	}
	else
		prog_path = argv[0];
	
	char *canon_prog_path = malloc(PATH_MAX);
	if (realpath(prog_path, canon_prog_path) == NULL) {
		// If we can't canonicalise the programs path name, we fall back on the "raw" path and cross
		// our fingers.
		strcpy(canon_prog_path, prog_path);
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

	Con_Obj *thread = Con_Bootstrap_do(root_stack_start, argc, argv, vm_path, canon_prog_path);
	if (thread == NULL) {
		fprintf(stderr, "%s: error when trying to initialize virtual machine.\n", __progname);
		exit(1);
	}
	
	size_t bytecode_size = con_binary_file_stat.st_size;
	u_char *bytecode = malloc(bytecode_size);
	fread(bytecode, 1, bytecode_size, con_binary_file);

	if (fclose(con_binary_file) != 0) {
		err(1, ": error closing '%s'", prog_path);
		exit(1);
	}

	// We now see if we've been given something that appears to be an executeable or a source file;
	// if the latter we go into make mode.
	
	ssize_t bytecode_start = find_bytecode_start(bytecode, bytecode_size);
	if (bytecode_start == -1) {
		make_mode(prog_path, &bytecode, &bytecode_size, vm_path, verbosity);
		bytecode_start = find_bytecode_start(bytecode, bytecode_size);
		if (bytecode_start == -1) {
			fprintf(stderr, "convergec does not appear to have produced an executeable.\n");
			exit(1);
		}
	}
	Con_Obj *main_module_identifier = Con_Bytecode_add_executable(thread, bytecode + bytecode_start);
	free(bytecode);

	Con_Obj *exception;
	CON_TRY {
		Con_Obj *main_module = Con_Modules_import(thread, Con_Modules_get(thread, main_module_identifier));
		CON_APPLY(CON_GET_MOD_DEFN(main_module, "main"));
	}
	CON_CATCH(exception) {
		Con_Int exit_code;
		
		// We check to see if this exception is an instance of System_Exit_Exception; if so, we
		// don't print a backtrace.
		
		if (CON_GET_SLOT_APPLY_NO_FAIL(CON_GET_MOD_DEFN(CON_BUILTIN(CON_BUILTIN_EXCEPTIONS_MODULE), "System_Exit_Exception"), "instantiated", exception) != NULL)
			exit_code = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT(exception, "code"));
		else {
			Con_Obj *backtrace = CON_APPLY(CON_GET_MOD_DEFN(CON_BUILTIN(CON_BUILTIN_EXCEPTIONS_MODULE), "backtrace"), exception);
			fprintf(stderr, "%s\n", Con_Builtins_String_Atom_to_c_string(thread, backtrace));
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

	main_module_identifier = NULL;
	Con_Memory_gc_force(thread);
	
	return 0;
}



void usage()
{
	fprintf(stderr, "Usage: %s [-v] [source file | executeable file\n", __progname);
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
		const char* cnds[] = {"", "../compiler/", NULL};
		char *cnd = malloc(PATH_MAX);
		char *canon_cnd = malloc(PATH_MAX);
		int i;
		for (i = 0; cnds[i] != NULL; i += 1) {
			strcpy(cnd, vm_path);
			int j;
			for (j = strlen(cnd); j >= 0 && cnd[j] != '/'; j -= 1) {}
			if (cnd[j] == '/') {
				strcpy(cnd + j + 1, cnds[i]);
				j += strlen(cnds[i]) + 1;
			}
			else {
				strcpy(cnd + j, cnds[i]);
				j += strlen(cnds[i]);
			}
			strcpy(cnd + j, name);
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

void make_mode(char *prog_path, u_char **bytecode, size_t *bytecode_size, char *vm_path, int verbosity)
{
#	ifdef CON_PLATFORM_MINGW
	CON_XXX;
#	else
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

	if (have_cache_path) {
		if (stat(cache_path, &st) == -1)
			goto make;

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

		ssize_t bytecode_start = find_bytecode_start(*bytecode, *bytecode_size);
		if (bytecode_start != -1 && Con_Bytecode_upto_date(NULL, *bytecode + bytecode_start, &st.STAT_ST_MTIMESPEC)) {
			// The cached file is completely upto date. We don't need to do anything else.
			return;
		}
	}

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
		args[j++] = prog_path;
		args[j++] = NULL;
		execv(vm_path, (char **const) args);
		err(1, ": trying to execv convergec");
	}
	
	// Parent process.
	
	close(filedes[1]);

	// Read in the output from the child process.

	i = sizeof(Con_Int) * 2 * *bytecode_size;
	*bytecode_size = 0;
	*bytecode = malloc(i);
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

	// Try and write the file to its cached equivalent. Since this isn't strictly necessary, if at
	// any point anything fails, we simply give up without reporting an error.

	if (have_cache_path) {
		FILE *cache_file;

		if (stat(cache_path, &st) != -1) {
			// As a file with the cached name already exists, we want to ensure that it's a Converge
			// executeable to be reasonably sure that we're not overwriting a normal user file.

			FILE *cache_file = fopen(cache_path, "rb");
			if (cache_file == NULL)
				return;

			u_char *tmp = malloc(BYTES_TO_FIND_EXEC);
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

		cache_file = fopen(cache_path, "w");
		if (cache_file == NULL)
			return;
		// We intentionally ignore any errors from fwrite or fclose.
		fwrite(*bytecode, 1, *bytecode_size, cache_file);
		fclose(cache_file);
	}
#	endif
}
