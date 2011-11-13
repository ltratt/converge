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


import os
from Builtins import *




def init(vm):
    return new_c_con_module(vm, "POSIX_File", "POSIX_File", __file__, import_, \
      ["DIR_SEP", "EXT_SEP", "NULL_DEV", "File_Atom_Def", "File", "canon_path", "exists", "is_dir",
       "is_file", "chmod", "rdmod", "iter_dir_entries", "mtime", "rm", "temp_file"])


def import_(vm):
    (mod,),_ = vm.decode_args("O")

    mod.set_defn(vm, "DIR_SEP", Con_String(vm, os.sep))
    mod.set_defn(vm, "EXT_SEP", Con_String(vm, os.extsep))
    mod.set_defn(vm, "NULL_DEV", Con_String(vm, "/dev/null"))

    bootstrap_file_class(vm, mod)

    new_c_con_func_for_mod(vm, "canon_path", canon_path, mod)
    new_c_con_func_for_mod(vm, "chmod", chmod, mod)
    new_c_con_func_for_mod(vm, "exists", exists, mod)
    new_c_con_func_for_mod(vm, "is_dir", is_dir, mod)
    new_c_con_func_for_mod(vm, "is_file", is_file, mod)
    new_c_con_func_for_mod(vm, "iter_dir_entries", iter_dir_entries, mod)
    new_c_con_func_for_mod(vm, "mtime", mtime, mod)
    new_c_con_func_for_mod(vm, "rdmod", rdmod, mod)
    new_c_con_func_for_mod(vm, "rm", rm, mod)
    new_c_con_func_for_mod(vm, "temp_file", temp_file, mod)
    
    vm.set_builtin(BUILTIN_C_FILE_MODULE, mod)
    
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))



################################################################################
# class File
#

class File(Con_Boxed_Object):
    __slots__ = ("path", "fd")
    _immutable_fields_ = ("path", "fd")


    def __init__(self, vm, instance_of, path, fd):
        Con_Boxed_Object.__init__(self, vm, instance_of)
        self.path = path
        self.fd = fd


    def has_slot_override(self, vm, n):
        if n == "fileno":
            return True
        else:
            return False


    def get_slot_override(self, vm, n):
        if n == "fileno":
            return Con_Int(vm, self.fd)
        else:
            return Con_Boxed_Object.get_slot_override(self, vm, n)


def _new_func_File(vm):
    (class_, path_o, mode_o), vargs = vm.decode_args("COS")
    assert isinstance(mode_o, Con_String)

    if isinstance(path_o, Con_String):
        path_s = path_o.v
        flags = _sflags(vm, mode_o.v)
        fd = os.open(path_s, flags, 0777)
    elif isinstance(path_o, Con_Int):
        path_s = None
        fd = path_o.v
    else:
        raise Exception("XXX")

    f = File(vm, class_, path_s, fd)
    vm.get_slot_apply(f, "init", [path_o, mode_o])

    vm.return_(f)


# Shamelessly influenced by libc/flags.c in OpenBSD.

def _sflags(vm, mode_s):
    err = False
    if len(mode_s) == 0:
        err = True

    if not err:
        c = mode_s[0]
        if c == "r":
            m = os.O_RDONLY
            o = 0
        elif c == "w":
            m = os.O_WRONLY
            o = os.O_CREAT | os.O_TRUNC
        elif c == "a":
		    m = os.O_WRONLY
		    o = os.O_CREAT | os.O_APPEND
        else:
            err = True

        if not err:
            if (len(mode_s) > 1 and mode_s[1] == "+") \
              or (len(mode_s) > 2 and mode_s[1] == "b" and mode_s[2] == "+"):
                m = os.O_RDWR
            
            return m | o

    vm.raise_helper("File_Exception", [Con_String(vm, "Illegal mode '%s'" % mode_s)])


def File_writeln(vm):
    (self, s_o),_ = vm.decode_args(mand="!S", self_of=File)
    assert isinstance(self, File)
    assert isinstance(s_o, Con_String)
    
    s = s_o.v + "\n"
    i = 0
    while i < len(s):
        i += os.write(self.fd, s[i:])

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def bootstrap_file_class(vm, mod):
    file_class = Con_Class(vm, Con_String(vm, "File"), [vm.get_builtin(BUILTIN_OBJECT_CLASS)], mod)
    mod.set_defn(vm, "File", file_class)
    file_class.new_func = new_c_con_func(vm, Con_String(vm, "new_File"), False, _new_func_File, mod)

    new_c_con_func_for_class(vm, "writeln", File_writeln, file_class)



################################################################################
# Other module-level functions
#

def canon_path(vm):
    raise Exception("XXX")


def chmod(vm):
    raise Exception("XXX")


def exists(vm):
    raise Exception("XXX")


def is_dir(vm):
    raise Exception("XXX")


def is_file(vm):
    raise Exception("XXX")


def iter_dir_entries(vm):
    (dp_o,),_ = vm.decode_args("S")
    assert isinstance(dp_o, Con_String)
    print "iter_dir_entries"
    
    for p in os.listdir(dp_o.v):
        vm.yield_(Con_String(vm, p))

    vm.return_(vm.get_builtin(BUILTIN_FAIL_OBJ))


def mtime(vm):
    raise Exception("XXX")


def rdmod(vm):
    raise Exception("XXX")


def rm(vm):
    raise Exception("XXX")


def temp_file(vm):
    raise Exception("XXX")

