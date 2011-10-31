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
    sys.path.append(os.getenv("PYPY_SRC"))
    
from pypy.config.config import Config
from pypy.rlib.jit import *
from pypy.rpython.lltypesystem import lltype, rffi
import os, os.path, sys
import Builtins, Bytecode, VM



VM_VERSION = "current"

STDLIB_DIRS = ["../lib/converge-%s/" % VM_VERSION, "../lib/"]
COMPILER_DIRS = ["../lib/converge-%s/" % VM_VERSION, "../compiler/"]


def entry_point(argv):
    try:
        filename = argv[1]
    except IndexError:
        print "You must supply a filename"
        return 1
    
    bc, start = _read_bc(argv[1], "CONVEXEC")

    vm_path = os.path.abspath(argv[0])

    useful_bc = rffi.str2charp(bc[start:])
    vm = VM.new_vm()
    _import_lib(vm, "Stdlib.cvl", vm_path, STDLIB_DIRS)
    _import_lib(vm, "Compiler.cvl", vm_path, COMPILER_DIRS)
    try:
        main_mod_id = Bytecode.add_exec(vm, useful_bc)
        mod = vm.get_mod(main_mod_id)
        vm.apply(mod.get_defn(vm, "main"))
    except VM.Con_Raise_Exception, e:
        ex_mod = vm.get_builtin(Builtins.BUILTIN_EXCEPTIONS_MODULE)
        sys_ex_class = ex_mod.get_defn(vm, "System_Exit_Exception")
        if vm.get_slot_apply(sys_ex_class, "instantiated", [e.ex_obj], allow_fail=True) is not None:
            code = Builtins.type_check_int(vm, e.ex_obj.get_slot(vm, "code"))
            return code.v
        else:
            msg = vm.get_slot_apply(e.ex_obj, "to_str")
            assert isinstance(msg, Builtins.Con_String)
            print msg.v
            raise
    
    return 0


def _read_bc(path, id_):
    s = os.stat(path).st_size
    f = os.open(path, os.O_RDONLY, 0777)
    bc = ""
    i = 0
    while i < s:
        d = os.read(f, 64 * 1024)
        bc += d
        i += len(d)

    i = 0
    s = os.stat(path).st_size
    while i < s:
        if bc[i:].startswith(id_):
            break
        i += 1
    else:
        raise Exception("XXX")

    return bc, i


def _import_lib(vm, leaf, vm_path, cnd_dirs):
    vm_dir = dirname(vm_path)
    for d in cnd_dirs:
        path = "%s/%s/%s" % (vm_dir, d, leaf)
        if os.path.exists(path):
            break
    else:
        raise Exception("XXX")

    bc, start = _read_bc(path, "CONVLIBR")
    if start != 0:
        raise Exception("XXX")
    Bytecode.add_lib(vm, rffi.str2charp(bc))


def dirname(p): # An RPython compatible version since that in os.path currently isn't...
    i = p.rfind('/') + 1
    assert i > 0
    head = p[:i]
    if head and head != '/'*len(head):
        head = head.rstrip('/')
    return head


def target(driver, args):
    VM.global_vm.pypy_config = driver.config
    return entry_point, None


def jitpolicy(driver):
    from pypy.jit.codewriter.policy import JitPolicy
    return JitPolicy()

if __name__ == "__main__":
    entry_point(sys.argv)