# Copyright (c) 2011 King's College London, created by Laurence Tratt
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.


try:
    import pypy
except:
    import os, sys
    sys.setrecursionlimit(20000)
    sys.path.append(os.getenv("PYPY_SRC"))
    
from pypy.config.config import Config
from pypy.rlib import rarithmetic, rposix
from pypy.rlib.jit import *
from pypy.rpython.lltypesystem import lltype, rffi
from pypy.rpython.tool import rffi_platform as platform
from pypy.translator.tool.cbuild import ExternalCompilationInfo
import os, os.path, sys
import Builtins, Bytecode, Config, Stdlib_Modules, VM



eci         = ExternalCompilationInfo(includes=["limits.h", "stdlib.h", "string.h"])

class CConfig:
    _compilation_info_ = eci
    BUFSIZ             = platform.DefinedConstantInteger("BUFSIZ")
    PATH_MAX           = platform.DefinedConstantInteger("PATH_MAX")

cconfig  = platform.configure(CConfig)

BUFSIZ   = cconfig["BUFSIZ"]
PATH_MAX = cconfig["PATH_MAX"]
getenv   = rffi.llexternal("getenv", [rffi.CCHARP], rffi.CCHARP, compilation_info=eci)
realpath = rffi.llexternal("realpath", [rffi.CCHARP, rffi.CCHARP], rffi.CCHARP, compilation_info=eci)
strlen   = rffi.llexternal("strlen", [rffi.CCHARP], rffi.SIZE_T, compilation_info=eci)



STDLIB_DIRS = ["../lib/converge-%s/" % Config.CON_VERSION, "../lib/"]
COMPILER_DIRS = ["../lib/converge-%s/" % Config.CON_VERSION, "../compiler/"]


def entry_point(argv):
    vm_path = _get_vm_path(argv)
    
    verbosity = 0
    mk_fresh = False
    i = 1
    for i in range(1, len(argv)):
        arg = argv[i]
        if len(arg) == 0 or (len(arg) == 1 and arg[0] == "-"):
            _usage(vm_path)
            return 1
        if arg[0] == "-":
            for c in arg[1:]:
                if c == "v":
                    verbosity += 1
                elif c == "f":
                    mk_fresh = True
                else:
                    _usage(vm_path)
                    return 1
        else:
            break
    if i < len(argv):
        filename = argv[i]
        if i + 1 < len(argv):
            args = argv[i + 1 : ]
        else:
            args = []
    else:
        filename = None
        args = []

    if filename is None:
        convergeip = _find_con_exec(vm_path, "convergei")
        if convergeip is None:
            return 1
        filename = convergeip

    progp = _canon_path(filename)
    bc, start = _read_bc(progp, "CONVEXEC")
    if len(bc) == 0:
        _error(vm_path, "No such file '%s'." % filename)
        return 1
    
    if start == -1:
        bc, start, rtn = _make_mode(vm_path, progp, bc, verbosity, mk_fresh)
        if rtn != 0:
            return rtn

    if start == -1:
        _error(vm_path, "No valid bytecode to run.")
        return 1

    assert start >= 0
    useful_bc = rffi.str2charp(bc[start:])
    vm = VM.new_vm(vm_path, args)
    _import_lib(vm, "Stdlib.cvl", vm_path, STDLIB_DIRS)
    _import_lib(vm, "Compiler.cvl", vm_path, COMPILER_DIRS)
    try:
        main_mod_id = Bytecode.add_exec(vm, useful_bc)
        mod = vm.get_mod(main_mod_id)
        mod.import_(vm)
        vm.apply(mod.get_defn(vm, "main"))
    except VM.Con_Raise_Exception, e:
        ex_mod = vm.get_builtin(Builtins.BUILTIN_EXCEPTIONS_MODULE)
        sys_ex_class = ex_mod.get_defn(vm, "System_Exit_Exception")
        if vm.get_slot_apply(sys_ex_class, "instantiated", [e.ex_obj], allow_fail=True) is not None:
            code = Builtins.type_check_int(vm, e.ex_obj.get_slot(vm, "code"))
            return int(code.v)
        else:
            pb = vm.import_stdlib_mod(Stdlib_Modules.STDLIB_BACKTRACE).get_defn(vm, "print_best")
            vm.apply(pb, [e.ex_obj])
            return 1
    
    return 0


def _get_vm_path(argv):
    if os.path.exists(argv[0]):
        # argv[0] points to a real file - job done.
        return _canon_path(argv[0])

    # We fall back on searching through $PATH (if it's available) to see if we can find an
    # executable of the name argv[0]
    raw_PATH = getenv("PATH")
    if raw_PATH:
        PATH = rffi.charp2str(raw_PATH)
        for d in PATH.split(":"):
            d = d.strip(" ")
            cnd = os.path.join(d, argv[0])
            if os.path.exists(cnd):
                return _canon_path(cnd)

    # At this point, everything we've tried has failed
    return ""


def _read_bc(path, id_):
    try:
        s = os.stat(path).st_size
        f = os.open(path, os.O_RDONLY, 0777)
        bc = ""
        i = 0
        while i < s:
            d = os.read(f, 64 * 1024)
            bc += d
            i += len(d)
        os.close(f)

        i = 0
        s = os.stat(path).st_size
        i = bc.find(id_)
    except OSError:
        return "", -1

    return bc, i


def _import_lib(vm, leaf, vm_path, cnd_dirs):
    vm_dir = _dirname(vm_path)
    for d in cnd_dirs:
        path = "%s/%s/%s" % (vm_dir, d, leaf)
        if os.path.exists(path):
            break
    else:
        _warning(vm_path, "Warning: Can't find %s." % leaf)
        return

    bc, start = _read_bc(path, "CONVLIBR")
    if start != 0:
        raise Exception("XXX")
    Bytecode.add_lib(vm, rffi.str2charp(bc))


def _make_mode(vm_path, path, bc, verbosity, mk_fresh):
    # Try to work out a plausible cached path name.
    dp = path.rfind(os.extsep)
    if dp >= 0 and os.sep not in path[dp:]:
        cp = path[:dp]
    else:
        cp = None
    
    if not cp or mk_fresh:
        return _do_make_mode(vm_path, path, None, verbosity, mk_fresh)
    else:
		# There is a cached path, so now we try and load it and see if it is upto date. If any part
		# of this fails, we simply go straight to full make mode.
        try:
            st = os.stat(cp)
        except OSError:
            return _do_make_mode(vm_path, path, cp, verbosity, mk_fresh)
        
        cbc, start = _read_bc(cp, "CONVEXEC")
        if start == -1:
            return _do_make_mode(vm_path, path, cp, verbosity, mk_fresh)
        
        assert start >= 0
        useful_bc = cbc[start:]
        if Bytecode.exec_upto_date(None, rffi.str2charp(useful_bc), st.st_mtime):
            return cbc, start, 0
        
        return _do_make_mode(vm_path, path, cp, verbosity, mk_fresh)


def _do_make_mode(vm_path, path, cp, verbosity, mk_fresh):
	# Fire up convergec -m on progpath. We do this by creating a pipe, forking, getting the child
    # to output to the pipe (although note that we leave stdin and stdout unmolested on the child
	# process, as user programs might want to print stuff to screen) and reading from that pipe
	# to get the necessary bytecode.

    convergecp = _find_con_exec(vm_path, "convergec")
    if convergecp is None:
        return None, 0, 1
    
    rfd, wfd = os.pipe()
    pid = os.fork()
    if pid == -1:
        raise Exception("XXX")
    elif pid == 0:
        # Child process.
        fdp = "/dev/fd/%d" % wfd
        
        args = [vm_path, convergecp, "-m", "-o", fdp]
        while verbosity > 0:
            args.append("-v")
            verbosity -= 1
        if mk_fresh:
            args.append("-f")
        
        args.append(path)
        os.execv(vm_path, args)
        _error(vm_path, "Couldn't execv convergec.")
        return None, -1, 1
    
    # Parent process
    
    os.close(wfd)
    
    # Read in the output from the child process.
    
    bc = ""
    while 1:
        try:
            r = os.read(rfd, BUFSIZ)
        except OSError:
            # Reading from a pipe seems to throw an exception when finished, which isn't the
            # documented behaviour...
            break
        if r == "":
            break
        bc += r

	# Now we've read all the data from the child convergec, we check its return status; if it
	# returned something other than 0 then we return that value and do not continue.

    _, status = os.waitpid(pid, 0)
    if os.WIFEXITED(status):
        rtn = os.WEXITSTATUS(status)
        if rtn != 0:
            return None, -1, rtn

    start = bc.find("CONVEXEC")
    if start == -1:
        _error(vm_path, "convergec failed to produce valid output.")
        return None, -1, 1

    if cp:
        # Try and write the file to its cached equivalent. Since this isn't strictly necessary, if
        # at any point anything fails, we simply give up without reporting an error.
        s = -1
        try:
            s = os.stat(cp).st_size
        except OSError:
            pass

        if s > 0:
            try:
                f = os.open(cp, os.O_RDONLY, 0777)
                d = os.read(f, 512)
                os.close(f)
            except OSError:
                return bc, start, 0

            if d.find("CONVEXEC") == -1:
                return bc, start, 0

        try:
            f = os.open(cp, os.O_WRONLY | os.O_CREAT, 0777)
            os.write(f, bc)
            os.close(f)
        except OSError:
            try:
                os.unlink(cp)
            except OSError:
                pass

    return bc, start, 0


def _find_con_exec(vm_path, leaf):
    cnds = [_dirname(vm_path), os.path.join(_dirname(_dirname(vm_path)), "compiler")]
    for cl in cnds:
        cp = os.path.join(cl, leaf)
        if os.path.exists(cp):
            return cp
    _error(vm_path, "Unable to locate %s." % leaf)
    return None


def _dirname(path): # An RPython compatible version since that in os.path currently isn't...
    i = path.rfind('/') + 1
    assert i > 0
    head = path[:i]
    if head and head != '/' * len(head):
        head = head.rstrip('/')
    return head


def _leafname(path):
    i = path.rfind('/') + 1
    assert i > 0
    return path[i:]


# Attempt to canonicalise path 'p'; at the very worst, 'p' is returned unchanged.

def _canon_path(path):
    with lltype.scoped_alloc(rffi.CCHARP.TO, PATH_MAX) as rp:
        r = realpath(path, rp)
        if not r:
            return path
        return rffi.charpsize2str(rp, rarithmetic.intmask(strlen(rp)))


def _error(vm_path, msg):
    print "%s: %s" % (_leafname(vm_path), msg)


def _warning(vm_path, msg):
    print "%s: %s" % (_leafname(vm_path), msg)


def _usage(vm_path):
    print "Usage: %s [-vf] [source file | executable file]" % _leafname(vm_path)


def target(driver, args):
    VM.global_vm.pypy_config = driver.config
    return entry_point, None


def jitpolicy(driver):
    from pypy.jit.codewriter.policy import JitPolicy
    return JitPolicy()

if __name__ == "__main__":
    entry_point(sys.argv)