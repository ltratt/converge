AC_INIT([converge], CONVERGE_VERSION)

# If shipping a production release, these two variables should be changed to e.g.:
#
#   converge_version=0.7
#   converge_date=2006/01/04

converge_version=current
converge_date=`date +%Y/%m/%d`


####################################################################################################
# Generic initialization
#

AC_CONFIG_HEADER(vm/Config.h)

AC_LANG(C)

AC_PROG_CC
AC_ISC_POSIX
AC_PROG_GCC_TRADITIONAL

AC_SUBST(CONVERGE_VERSION, [$converge_version])
AC_SUBST(CONVERGE_DATE, [$converge_date])

AC_SUBST(CONVERGE_DIR, [`readlink -f .`])


####################################################################################################
# Generic config
#

AH_TOP([
#ifndef _CON_CONFIG_H
#define _CON_CONFIG_H

// Switch to 1 to turn debugging output on

#define CON_FULL_DEBUG 0

// Some constants which may be useful to bump up for performance, at the expense of memory

#define CON_DEFAULT_VM_NUM_THREADS 10

#define CON_DEFAULT_FIRST_N_INTS_TO_INTERN 1024

#define CON_DEFAULT_STRING_NUM_CACHE_ENTRIES 256

#define CON_DEFAULT_NUM_COUNTING_MUTEXES 10
#define CON_DEFAULT_MAX_USERS_PER_COUNTING_MUTEX 512
#define CON_DEFAULT_NUM_SLOTS 16
#define CON_DEFAULT_FULL_ENTRIES_SIZE (16 * (sizeof(Con_Slots_Full_Entry) + 8))

#define CON_DEFAULT_GC_STACK_NUM_ENTRIES 5000

#define CON_DEFAULT_DICT_NUM_ENTRIES_ALLOCATED 32
#define CON_DEFAULT_LIST_NUM_ENTRIES_ALLOCATED 16
#define CON_DEFAULT_SET_NUM_ENTRIES_ALLOCATED 32
#define CON_DEFAULT_ARRAY_NUM_ENTRIES_ALLOCATED 256

#define CON_DEFAULT_CON_C_CLASS_OBJ_NUM_INSTANCE_MUTEXES 8

#define CON_DEFAULT_NUM_OBJECTS_TO_TRACK 8192

// Default Converge stack size in words

#define CON_DEFAULT_CON_STACK_SIZE (4096-12)

/* Define this for Linux */

#define _BSD_SOURCE 1
])

AH_BOTTOM([#endif])



####################################################################################################
# Compiler configuration
#

AH_TEMPLATE(CON_CC, [The name of your C compiler.])
AC_DEFINE_UNQUOTED(CON_CC, [$CC])


extra_cflags="-I/usr/local/include -I/usr/include/pcre"
extra_libs="-L/usr/local/lib"



####################################################################################################
# Machine dependent configuration
#

machine_type=`uname -m`

AH_TEMPLATE(CON_ARCH, [Define to your system architecture.])

AH_TEMPLATE(CON_C_STACK_GROWS_DOWN, [Define if the stack grows down from a particular word. Undefine if the stack grows upwards.])

AC_MSG_CHECKING([machine type])
case $machine_type in
	amd64 | x86_64 )
		AC_MSG_RESULT(x86_64)
		AC_DEFINE(CON_ARCH_X86_64, [], [Define if running on x86_64])
		AC_SUBST(CON_ARCH, "x86_64")
		AC_DEFINE(CON_C_STACK_GROWS_DOWN);;
	i386 | i686 )
		AC_MSG_RESULT(x86)
		AC_DEFINE(CON_ARCH_X86, [], [Define if running on x86])
		AC_SUBST(CON_ARCH, "x86")
		AC_DEFINE(CON_C_STACK_GROWS_DOWN);;
	sun4u) 
		AC_MSG_RESULT(sun4u)
		AC_DEFINE(CON_ARCH_SUN4U, [], [Define if running on sun4u])
		AC_SUBST(CON_ARCH, "sun4u")
		AC_DEFINE(CON_C_STACK_GROWS_DOWN);;
	*) AC_MSG_ERROR([Unknown machine type "$machine_type"]);;
esac

AH_TEMPLATE(CON_BYTEORDER, [Whether this is a big endian (CON_BIG_ENDIAN) or little endian (CON_LITTLE_ENDIAN) machine.])
AC_C_BIGENDIAN([AC_DEFINE(CON_BYTEORDER, CON_BIG_ENDIAN)],
	[AC_DEFINE(CON_BYTEORDER, CON_LITTLE_ENDIAN)])

AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)

AC_CHECK_TYPE(u_char, unsigned char)

AH_TEMPLATE(CON_INT, [The integer type equivalent to a machine word.])
AC_CHECK_TYPE(ptrdiff_t)

AC_MSG_CHECKING([integer type equivalent to a machine word])
AC_TRY_RUN([
#include <stddef.h>

int main()
{
	if (sizeof(ptrdiff_t) == sizeof(int))
		return 0;
	else
		return 1;
}], 
	[AC_MSG_RESULT(int)
	AC_DEFINE(CON_INT, int)
	],
	[AC_TRY_RUN([
#include <stddef.h>

int main()
{
	if (sizeof(ptrdiff_t) == sizeof(long))
		return 0;
	else
		return 1;
}],
		[AC_MSG_RESULT(long)
		AC_DEFINE(CON_INT, long)
		],
		[AC_TRY_RUN([
#include <stddef.h>

int main()
{
	if (sizeof(ptrdiff_t) == sizeof(long long))
		return 0;
	else
		return 1;
}],
			[AC_MSG_RESULT(long long)
			AC_DEFINE(CON_INT, long long)
			],
			AC_MSG_FAILURE(Unknown word size)
		)]
	)]
)

AC_CHECK_SIZEOF(CON_INT)

AC_MSG_CHECKING([for architecture specific memmove])
if [[ -f vm/Platform/Arch/$CON_ARCH/memmove.S ]]; then
	AC_MSG_RESULT(Platform/Arch/$CON_ARCH/memmove.S)
	AC_SUBST(CON_MEMMOVE, "Platform/Arch/$CON_ARCH/memmove.o")
else
	AC_MSG_RESULT(none found)
fi



####################################################################################################
# Platform check
#

AC_MSG_CHECKING(platform)
operating_system=`uname -s`
case $operating_system in
	OpenBSD | FreeBSD | NetBSD | Linux | CYGWIN_NT*) AC_MSG_RESULT(POSiX); AC_DEFINE(CON_PLATFORM_POSIX, [], [Define if you are running on a POSiX platform. Note that you can can only define one CON_PLATFORM_* variable.]);;
	*) AC_MSG_ERROR([Unknown platform "$operating_system"]);;
esac


case $ac_cv_sizeof_CON_INT in
	4 ) AC_C_BIGENDIAN([bootstrap_dir="bootstrap/32bit_big_endian"], [bootstrap_dir="bootstrap/32bit_little_endian"]);;
	8 ) AC_C_BIGENDIAN([bootstrap_dir="bootstrap/64bit_big_endian"], [bootstrap_dir="bootstrap/64bit_little_endian"]);;
	* ) AC_MSG_ERROR([Unknown word size "$ac_cv_sizeof_CON_INT"]);;
esac


####################################################################################################
# Thread library checks
#

AC_MSG_CHECKING(whether thread support required)
AC_ARG_WITH(threads, AC_HELP_STRING(--with-threads, enable thread support), [], [with_threads="no"])

if [[ "$with_threads" = "yes" ]]; then
	AC_MSG_RESULT(yes)
	AC_CHECK_HEADER(pthread.h, , [AC_MSG_ERROR([Unable to find pthread.h])], [#include "pthread.h"])
	AC_DEFINE(CON_THREADS_PTHREADS, [], [Define if you have, and want to use, pthreads. Note that you can can only define one CON_THREADS_* variable.])
	
	AH_TEMPLATE(CON_HAVE_PTHREAD_YIELD,
		[Define if you have the pthread_yield function. If pthreads are available, this function is used to avoid spin locking.])
	AC_CHECK_DECL([pthread_yield], [AC_DEFINE(CON_HAVE_PTHREAD_YIELD)], [], [#include <pthread.h>])
	
	AC_SUBST(CON_THREAD_ATOM_OBJ, "Platform/Threads/PThreads/PThreads_Atom.o")
	AC_SUBST(CON_THREAD_CLASS_OBJ, "Platform/Threads/PThreads/PThreads_Class.o")
	AC_SUBST(CON_THREAD_MODULE_OBJ, "Platform/Threads/PThreads/PThreads_Module.o")
	AC_SUBST(CON_THREAD_TEST_DIR, "pthreads")
	
	extra_libs="$extra_libs -lpthread"
else
	AC_MSG_RESULT(no)
	AC_DEFINE(CON_THREADS_SINGLE_THREADED, [], [Define if you don't have a supported threads library on your platform, or if you wish to use the single threaded version of Converge. Note that you can can only define one CON_THREADS_* variable.])
	AC_SUBST(CON_THREAD_ATOM_OBJ, "Platform/Threads/Single_Thread/Single_Thread_Atom.o")
	AC_SUBST(CON_THREAD_CLASS_OBJ, "Platform/Threads/Single_Thread/Single_Thread_Class.o")
	AC_SUBST(CON_THREAD_TEST_DIR, "single_thread")
fi



####################################################################################################
# Package / header file checks
#

CFLAGS="$extra_cflags"

AC_CHECK_HEADER(pcre.h, , [AC_MSG_ERROR([Unable to find pcre.h])], [#include "pcre.h"])

case $operating_system in
	CYGWIN_NT*) extra_libs="$extra_libs /usr/lib/libpcre.a";;
	*) extra_libs="$extra_libs -lpcre";;
esac


AC_PATH_PROG(xml2_config, xml2-config, fail)
if [[ "$xml2_config" = "fail" ]]; then
	AC_MSG_ERROR([Unable to find xml2-config])
fi
extra_cflags="$extra_cflags `$xml2_config --cflags`"
extra_libs="$extra_libs `$xml2_config --libs`"



####################################################################################################
# Function / feature checks
#

# mergesort

AH_TEMPLATE(CON_HAVE_MERGESORT,
  [Define if you have the mergesort function. If available, this is used to speed up the garbage collector.])

AC_CHECK_FUNC(mergesort, [AC_DEFINE(CON_HAVE_MERGESORT)])


# fgetln

AC_CHECK_FUNC(fgetln, [], [AC_SUBST(FGETLN, misc/fgetln.o)])


# getdirentries

AH_TEMPLATE(CON_HAVE_GETDIRENTRIES,
  [Define if your platform has the getdirentries function.])

AC_CHECK_FUNC(getdirentries, [AC_DEFINE(CON_HAVE_GETDIRENTRIES)])


# alloca

AC_FUNC_ALLOCA


# see what dirent calls d_fileno

AH_TEMPLATE(CON_DIRENT_D_FILENO,
  [Define to whatever your dirent struct calls d_fileno (e.g. d_ino under Linux and Cygwin).])

AC_CHECK_MEMBER(struct dirent.d_fileno,
	[AC_DEFINE(CON_DIRENT_D_FILENO, d_fileno)],
	[AC_CHECK_MEMBER(struct dirent.d_ino,
		[AC_DEFINE(CON_DIRENT_D_FILENO, d_ino)],
		,
		[#include "dirent.h"])
	],
	[#include "dirent.h"])


# see whether dirent has d_reclen or not

AH_TEMPLATE(CON_DIRENT_HAS_D_RECLEN,
  [Define if your dirent struct has the d_reclen member.])

AC_CHECK_MEMBER(struct dirent.d_reclen,
	[AC_DEFINE(CON_DIRENT_HAS_D_RECLEN)],
	,
	[#include "dirent.h"])


# see what stat calls st_mtimespec

AH_TEMPLATE(STAT_ST_MTIMESPEC,
  [Define to whatever your stat struct calls st_mtimespec (e.g. st_mtim under Linux and Cygwin).])

AC_CHECK_MEMBER(struct stat.st_mtimespec,
	[AC_DEFINE(STAT_ST_MTIMESPEC, st_mtimespec)],
	[AC_CHECK_MEMBER(struct stat.st_mtim,
		[AC_DEFINE(STAT_ST_MTIMESPEC, st_mtim)],
		,
		[#include <sys/stat.h>])
	],
	[#include <sys/stat.h>])


# glibc is brain dead with respect to strerror_r.

AC_FUNC_STRERROR_R


# srandomdev

AH_TEMPLATE(CON_HAVE_SRANDOMDEV,
  [Define if your platform has the srandomdev function.])

AC_CHECK_FUNC(srandomdev, [AC_DEFINE(CON_HAVE_SRANDOMDEV)])


# sigsetjmp

AC_MSG_CHECKING([for sigsetjmp])

AH_TEMPLATE(_POSIX_C_SOURCE,
  [Define if your platform needs _POSIX_C_SOURCE for sigsetjmp to be defined.])

CFLAGS="-std=c99"
AC_TRY_RUN([
#include <setjmp.h>

int main()
{
	sigjmp_buf return_env;
	sigsetjmp(return_env, 0);
}], [AC_MSG_RESULT(yes)], [CFLAGS="-std=c99 -D_POSIX_C_SOURCE"
	AC_TRY_RUN([
#include <setjmp.h>

int main()
{
	sigjmp_buf return_env;
	sigsetjmp(return_env, 0);
}], [AC_MSG_RESULT(-D_POSIX_C_SOURCE)
	extra_cflags="$extra_cflags -D_POSIX_C_SOURCE"], [AC_MSG_ERROR(unable to find sigsetjmp)])])


####################################################################################################
# C compiler
#

if [[ "$GCC" = "yes" ]]; then
	CFLAGS="-std=c99 -g -DNDEBUG -Wall -Wstrict-prototypes -Wmissing-prototypes -Wwrite-strings -Wpointer-arith -Wbad-function-cast -Wsign-compare $extra_cflags"

	# We only enable optimisation on GCC 3.3 - on later versions it seems to cause problems.
	$CC -v 2>&1 /dev/null | grep "gcc version 3\.3" > /dev/null
	if [[ $? -eq 0 ]]; then
		CFLAGS="$CFLAGS -O2"
	fi
else
	CFLAGS="-DNDEBUG"
fi


####################################################################################################
# Output
#

LIBS="$extra_libs"

AC_CONFIG_FILES([Makefile Makefile.inc compiler/Makefile vm/Version.h vm/Makefile lib/Makefile
	bootstrap/Makefile tests/Makefile tests/threads/Makefile tests/threads/pthreads/Makefile
	tests/threads/single_thread/Makefile manuals/Makefile examples/Makefile
	examples/compile_time/Makefile examples/dsls/Makefile examples/metaclasses/Makefile])
AC_OUTPUT


if [[ ! -f compiler/convergec ]]; then
	echo "copying over bootstrap compiler"
	cp $bootstrap_dir/convergec.bootstrap compiler/convergec
else
	echo "preserving existing bootstrap compiler"
fi

if [[ ! -f compiler/convergel ]]; then
	echo "copying over bootstrap linker"
	cp $bootstrap_dir/convergel.bootstrap compiler/convergel
else
	echo "preserving existing bootstrap linker"
fi
