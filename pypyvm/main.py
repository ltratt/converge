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
import os
import Builtins, Bytecode, VM



def entry_point(argv):
    try:
        filename = argv[1]
    except IndexError:
        print "You must supply a filename"
        return 1
    
    s = os.stat(argv[1]).st_size
    f = os.open(argv[1], os.O_RDONLY, 0777)
    bc = ""
    i = 0
    while i < s:
        d = os.read(f, 64 * 1024)
        bc += d
        i += len(d)

    i = 0
    while i < s:
        if bc[i:].startswith("CONVEXEC"):
            break
        i += 1

    useful_bc = rffi.str2charp(bc[i:])
    vm = VM.new_vm()
    main_mod_id = Bytecode.add_exec(vm, useful_bc)
    try:
        mod = vm.get_mod(main_mod_id)
        vm.apply(mod.get_defn("main"))
    except SystemExit:
        return vm.exit_code
    
    return 0



def target(driver, args):
    VM.global_vm.pypy_config = driver.config
    return entry_point, None
    
def jitpolicy(driver):
    from pypy.jit.codewriter.policy import JitPolicy
    return JitPolicy()

if __name__ == "__main__":
    entry_point(sys.argv)