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
from pypy.rpython.lltypesystem import lltype, rffi
from pypy.rpython.tool import rffi_platform as platform
from pypy.rlib import rarithmetic, rposix
from pypy.translator.tool.cbuild import ExternalCompilationInfo
from Builtins import *
import Stdlib_Modules



eci         = ExternalCompilationInfo(includes=["limits.h", "stdio.h", "stdlib.h", "string.h",
             "unistd.h"])

FILEP       = rffi.COpaquePtr("FILE")
fclose      = rffi.llexternal("fclose", [FILEP], rffi.INT, compilation_info=eci)
fdopen      = rffi.llexternal("fdopen", [rffi.INT, rffi.CCHARP], FILEP, compilation_info=eci)
feof        = rffi.llexternal("feof", [FILEP], rffi.INT, compilation_info=eci)
ferror      = rffi.llexternal("ferror", [FILEP], rffi.INT, compilation_info=eci)
fflush      = rffi.llexternal("fflush", [FILEP], rffi.INT, compilation_info=eci)
fgetln      = rffi.llexternal("fgetln", [FILEP, rffi.INTP], rffi.CCHARP, compilation_info=eci)
fileno      = rffi.llexternal("fileno", [FILEP], rffi.INT, compilation_info=eci)
flockfile   = rffi.llexternal("flockfile", [FILEP], lltype.Void, compilation_info=eci)
fopen       = rffi.llexternal("fopen", [rffi.CCHARP, rffi.CCHARP], FILEP, compilation_info=eci)
fread       = rffi.llexternal("fread", [rffi.CCHARP, rffi.SIZE_T, rffi.SIZE_T, FILEP], rffi.SIZE_T, \
                compilation_info=eci)
fseek       = rffi.llexternal("fseek", [FILEP, rffi.INT, rffi.INT], rffi.INT, compilation_info=eci)
funlockfile = rffi.llexternal("funlockfile", [FILEP], lltype.Void, compilation_info=eci)
fwrite      = rffi.llexternal("fwrite", [rffi.CCHARP, rffi.SIZE_T, rffi.SIZE_T, FILEP], rffi.SIZE_T, \
                compilation_info=eci)

if platform.has("mkstemp", "#include <stdlib.h>"):
    HAS_MKSTEMP = True
    mkstemp = rffi.llexternal("mkstemp", [rffi.CCHARP], rffi.INT, compilation_info=eci)
else:
    HAS_MKSTEMP = False
    tmpnam  = rffi.llexternal("tmpnam", [rffi.CCHARP], rffi.CCHARP, compilation_info=eci)
realpath    = rffi.llexternal("realpath", \
                [rffi.CCHARP, rffi.CCHARP], rffi.CCHARP, compilation_info=eci)

strlen= rffi.llexternal("strlen", \
                [rffi.CCHARP], rffi.SIZE_T, compilation_info=eci)

class CConfig:
    _compilation_info_ = eci
    PATH_MAX           = platform.DefinedConstantInteger("PATH_MAX")
    SEEK_SET           = platform.DefinedConstantInteger("SEEK_SET")

cconfig = platform.configure(CConfig)

PATH_MAX = cconfig["PATH_MAX"]
SEEK_SET = cconfig["SEEK_SET"]



def init(vm):
    return new_c_con_module(vm, "POSIX_File", "POSIX_File", __file__, import_, \
      ["DIR_SEP", "EXT_SEP", "NULL_DEV", "File_Atom_Def", "File", "canon_path", "exists", "is_dir",
       "is_file", "chmod", "iter_dir_entries", "mtime", "rm", "temp_file"])


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
    new_c_con_func_for_mod(vm, "rm", rm, mod)
    new_c_con_func_for_mod(vm, "temp_file", temp_file, mod)
    
    vm.set_builtin(BUILTIN_C_FILE_MODULE, mod)
    
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def _errno_raise(vm, path):
    if path is not None:
        msg = "File '%s': %s." % (path, os.strerror(rposix.get_errno()))
    else:
        msg = os.strerror(rposix.get_errno())
    vm.raise_helper("File_Exception", [Con_String(vm, msg)])


################################################################################
# class File
#

class File(Con_Boxed_Object):
    __slots__ = ("path", "filep", "closed")
    _immutable_fields_ = ("path", "file")


    def __init__(self, vm, instance_of, path, filep):
        Con_Boxed_Object.__init__(self, vm, instance_of)
        self.path = path
        self.filep = filep
        self.closed = False


    def __del__(self):
        if not self.closed:
		    # If the file is still open, we now close it to prevent a memory leak, as well as return
            # resources to the OS. Errors from fclose are ignored as there's nothing sensible we can
            # do with them at this point.
            fclose(self.filep)


    def has_slot_override(self, vm, n):
        if n == "path":
            return True
        
        return Con_Boxed_Object.has_slot_override(self, vm, n)


    def get_slot_override(self, vm, n):
        if n == "path":
            if self.path:
                return Con_String(vm, self.path)
            else:
                return vm.get_builtin(BUILTIN_NULL_OBJ)
        
        return Con_Boxed_Object.get_slot_override(self, vm, n)


def _new_func_File(vm):
    (class_, path_o, mode_o), vargs = vm.decode_args("COS")
    assert isinstance(mode_o, Con_String)

    f = path_s = None
    if isinstance(path_o, Con_String):
        path_s = path_o.v
        f = fopen(path_s, mode_o.v)
    elif isinstance(path_o, Con_Int):
        path_s = None
        f = fdopen(path_o.v, mode_o.v)
    else:
        vm.raise_helper("Type_Exception", [Con_String(vm, "[String, Int]"), path_o])

    if not f:
        _errno_raise(vm, path_s)

    f_o = File(vm, class_, path_s, f)
    vm.get_slot_apply(f_o, "init", [path_o, mode_o])

    vm.return_(f_o)


def File_close(vm):
    (self,),_ = vm.decode_args("!", self_of=File)
    assert isinstance(self, File)
    _check_open(vm, self)

    if fclose(self.filep) != 0:
        _errno_raise(vm, self.path)
    self.closed = True

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def File_fileno(vm):
    (self,),_ = vm.decode_args("!", self_of=File)
    assert isinstance(self, File)
    _check_open(vm, self)

    vm.return_(Con_Int(vm, int(fileno(self.filep))))


def File_flush(vm):
    (self,),_ = vm.decode_args("!", self_of=File)
    assert isinstance(self, File)
    _check_open(vm, self)
    
    if fflush(self.filep) != 0:
        _errno_raise(vm, self.path)

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def File_read(vm):
    (self, rsize_o),_ = vm.decode_args(mand="!", opt="I", self_of=File)
    assert isinstance(self, File)
    _check_open(vm, self)
    
    flockfile(self.filep)
    fsize = os.fstat(fileno(self.filep)).st_size
    if rsize_o is None:
        rsize = fsize
    else:
        assert isinstance(rsize_o, Con_Int)
        rsize = rsize_o.v
        if rsize < 0:
            vm.raise_helper("File_Exception", \
              [Con_String(vm, "Can not read less than 0 bytes from file.")])
        elif rsize > fsize:
            rsize = fsize

    with lltype.scoped_alloc(rffi.CCHARP.TO, rsize) as buf:
        r = fread(buf, 1, rsize, self.filep)
        if r < rffi.r_uint(rsize) and ferror(self.filep) != 0:
            vm.raise_helper("File_Exception", [Con_String(vm, "Read error.")])
        s = rffi.charpsize2str(buf, rarithmetic.intmask(r))
    funlockfile(self.filep)

    vm.return_(Con_String(vm, s))


def File_readln(vm):
    (self,),_ = vm.decode_args(mand="!", self_of=File)
    assert isinstance(self, File)
    _check_open(vm, self)
    
    while 1:
        with lltype.scoped_alloc(rffi.INTP.TO, 1) as lenp:
            l = fgetln(self.filep, lenp)
            if not l:
                if feof(self.filep) != 0:
                    break
                _errno_raise(vm, self.path)
            l_o = Con_String(vm, rffi.charpsize2str(l, rarithmetic.intmask(lenp[0])))
            vm.yield_(l_o)

    vm.return_(vm.get_builtin(BUILTIN_FAIL_OBJ))


def File_seek(vm):
    (self, off_o),_ = vm.decode_args(mand="!I", self_of=File)
    assert isinstance(self, File)
    assert isinstance(off_o, Con_Int)
    _check_open(vm, self)

    if fseek(self.filep, off_o.v, SEEK_SET) != 0:
        _errno_raise(vm, self.path)

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def File_write(vm):
    (self, s_o),_ = vm.decode_args(mand="!S", self_of=File)
    assert isinstance(self, File)
    assert isinstance(s_o, Con_String)
    _check_open(vm, self)
    
    s = s_o.v
    if len(s) > 0 and fwrite(s, len(s), 1, self.filep) < 1:
        vm.raise_helper("File_Exception", [Con_String(vm, "Write error.")])

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def File_writeln(vm):
    (self, s_o),_ = vm.decode_args(mand="!S", self_of=File)
    assert isinstance(self, File)
    assert isinstance(s_o, Con_String)
    _check_open(vm, self)
    
    s = s_o.v + "\n"
    if fwrite(s, len(s), 1, self.filep) < 1:
        vm.raise_helper("File_Exception", [Con_String(vm, "Write error.")])

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def _check_open(vm, file):
    if file.closed:
        vm.raise_helper("File_Exception", [Con_String(vm, "File previously closed.")])


def bootstrap_file_class(vm, mod):
    file_class = Con_Class(vm, Con_String(vm, "File"), [vm.get_builtin(BUILTIN_OBJECT_CLASS)], mod)
    mod.set_defn(vm, "File", file_class)
    file_class.new_func = new_c_con_func(vm, Con_String(vm, "new_File"), False, _new_func_File, mod)

    new_c_con_func_for_class(vm, "close", File_close, file_class)
    new_c_con_func_for_class(vm, "fileno", File_fileno, file_class)
    new_c_con_func_for_class(vm, "flush", File_flush, file_class)
    new_c_con_func_for_class(vm, "read", File_read, file_class)
    new_c_con_func_for_class(vm, "readln", File_readln, file_class)
    new_c_con_func_for_class(vm, "seek", File_seek, file_class)
    new_c_con_func_for_class(vm, "write", File_write, file_class)
    new_c_con_func_for_class(vm, "writeln", File_writeln, file_class)



################################################################################
# Other module-level functions
#

def canon_path(vm):
    (p_o,),_ = vm.decode_args("S")
    assert isinstance(p_o, Con_String)

    with lltype.scoped_alloc(rffi.CCHARP.TO, PATH_MAX) as resolved:
        r = realpath(p_o.v, resolved)
        if not r:
            _errno_raise(vm, p_o.v)
        rp = rffi.charpsize2str(resolved, rarithmetic.intmask(strlen(resolved)))

    vm.return_(Con_String(vm, rp))


def chmod(vm):
    (p_o, mode_o),_ = vm.decode_args("SI")
    assert isinstance(p_o, Con_String)
    assert isinstance(mode_o, Con_Int)
    
    try:
        os.chmod(p_o.v, mode_o.v)
    except OSError, e:
        _errno_raise(vm, p_o.v)

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def exists(vm):
    (p_o,),_ = vm.decode_args("S")
    assert isinstance(p_o, Con_String)
    
    try:
        if os.path.exists(p_o.v):
            vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))
        else:
            vm.return_(vm.get_builtin(BUILTIN_FAIL_OBJ))
    except OSError, e:
        _errno_raise(vm, p_o.v)


def is_dir(vm):
    (p_o,),_ = vm.decode_args("S")
    assert isinstance(p_o, Con_String)
    
    try:
        if os.path.isdir(p_o.v):
            vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))
        else:
            vm.return_(vm.get_builtin(BUILTIN_FAIL_OBJ))
    except OSError, e:
        _errno_raise(vm, p_o.v)


def is_file(vm):
    (p_o,),_ = vm.decode_args("S")
    assert isinstance(p_o, Con_String)
    
    try:
        if os.path.isfile(p_o.v):
            vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))
        else:
            vm.return_(vm.get_builtin(BUILTIN_FAIL_OBJ))
    except OSError, e:
        _errno_raise(vm, p_o.v)


def iter_dir_entries(vm):
    (dp_o,),_ = vm.decode_args("S")
    assert isinstance(dp_o, Con_String)
    
    try:
        for p in os.listdir(dp_o.v):
            vm.yield_(Con_String(vm, p))
    except OSError, e:
        _errno_raise(vm, dp_o.v)

    vm.return_(vm.get_builtin(BUILTIN_FAIL_OBJ))


def mtime(vm):
    (p_o,),_ = vm.decode_args("S")
    assert isinstance(p_o, Con_String)
    
    time_mod = vm.import_stdlib_mod(Stdlib_Modules.STDLIB_TIME)
    mk_timespec = time_mod.get_defn(vm, "mk_timespec")

    # XXX Ideally we'd use our own stat implementation here, but it's a cross-platform nightmare, so
    # this is a reasonable substitute. We might lose a bit of accuracy because floating point
    # numbers won't be a totally accurate representation of nanoseconds, but the difference
    # probably isn't enough to worry about.

    mtime = 0
    try:
        mtime = os.stat(p_o.v).st_mtime
    except OSError, e:
        _errno_raise(vm, p_o.v)
    
    sec = int(mtime)
    nsec = int((mtime - int(mtime)) * 1E9)
    
    vm.return_(vm.apply(mk_timespec, [Con_Int(vm, sec), Con_Int(vm, nsec)]))


def rm(vm):
    (p_o,),_ = vm.decode_args("S")
    assert isinstance(p_o, Con_String)
    
    st = [p_o.v]
    i = 0
    while len(st) > 0:
        p = st[i]
        try:
            if os.path.isdir(p):
                leafs = os.listdir(p)
                if len(leafs) == 0:
                    print "rmdir", p
                    os.rmdir(p)
                    del st[i]
                else:
                    i += 1
                    st.extend([os.path.join(p, l) for l in leafs])
            else:
                os.unlink(p)
                del st[i]
        except OSError, e:
            _errno_raise(vm, p)

        if i == len(st):
            i -= 1

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def temp_file(vm):
    mod = vm.get_funcs_mod()
    file_class = type_check_class(vm, mod.get_defn(vm, "File"))
    vm.decode_args()
    
    if HAS_MKSTEMP:
        #tmpdir = None
        #if os.environ.has_key("TMPDIR"):
        #    tmpdir = os.environ["TMPDIR"]
        #if tmpdir is None:
        #    tmpdir = "/tmp"
        tmpp = "/tmp/tmp.XXXXXXXXXX"
        with rffi.scoped_str2charp(tmpp) as buf:
            fd = mkstemp(buf)
            tmpp = rffi.charp2str(buf)
            
        if fd == -1:
            _errno_raise(vm, tmpp)
        
        f = fdopen(fd, "w+")
        if not f:
            _errno_raise(vm, tmpp)
        
        vm.return_(File(vm, file_class, tmpp, f))
    else:
        raise Exception("XXX")