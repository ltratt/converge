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


from rpython.rtyper.lltypesystem import lltype, rffi
from rpython.rtyper.tool import rffi_platform as platform
from rpython.rlib.rsre import rsre_re
from rpython.translator.tool.cbuild import ExternalCompilationInfo
import Config
from Builtins import *
from Core import *


eci                        = ExternalCompilationInfo(includes=["pcre.h"], \
                               include_dirs=Config.LIBPCRE_INCLUDE_DIRS, \
                               library_dirs=Config.LIBPCRE_LIBRARY_DIRS, \
                               libraries=Config.LIBPCRE_LIBRARIES, \
                               link_extra=Config.LIBPCRE_LINK_FLAGS, \
                               link_files=[Config.LIBPCRE_A])

class CConfig:
    _compilation_info_     = eci
    PCRE_DOTALL            = platform.DefinedConstantInteger("PCRE_DOTALL")
    PCRE_MULTILINE         = platform.DefinedConstantInteger("PCRE_MULTILINE")
    PCRE_INFO_CAPTURECOUNT = platform.DefinedConstantInteger("PCRE_INFO_CAPTURECOUNT")
    PCRE_ANCHORED          = platform.DefinedConstantInteger("PCRE_ANCHORED")
    PCRE_ERROR_NOMATCH     = platform.DefinedConstantInteger("PCRE_ERROR_NOMATCH")

cconfig = platform.configure(CConfig)

PCREP                  = rffi.COpaquePtr("pcre")
PCRE_DOTALL            = cconfig["PCRE_DOTALL"]
PCRE_MULTILINE         = cconfig["PCRE_MULTILINE"]
PCRE_INFO_CAPTURECOUNT = cconfig["PCRE_INFO_CAPTURECOUNT"]
PCRE_ANCHORED          = cconfig["PCRE_ANCHORED"]
PCRE_ERROR_NOMATCH     = cconfig["PCRE_ERROR_NOMATCH"]

pcre_compile = rffi.llexternal("pcre_compile", \
  [rffi.CCHARP, rffi.INT, rffi.CCHARPP, rffi.INTP, rffi.VOIDP], PCREP, compilation_info=eci)
pcre_fullinfo = rffi.llexternal("pcre_fullinfo", \
  [PCREP, rffi.VOIDP, rffi.INT, rffi.INTP], rffi.INT, compilation_info=eci)
pcre_exec = rffi.llexternal("pcre_exec", \
  [PCREP, rffi.VOIDP, rffi.CCHARP, rffi.INT, rffi.INT, rffi.INT, rffi.INTP, rffi.INT], \
  rffi.INT, compilation_info=eci)



def init(vm):
    return new_c_con_module(vm, "PCRE", "PCRE", __file__, import_, \
      ["PCRE_Exception", "Pattern", "Match", "compile"])


@con_object_proc
def import_(vm):
    (mod,),_ = vm.decode_args("O")

    bootstrap_pattern_class(vm, mod)
    bootstrap_match_class(vm, mod)
    new_c_con_func_for_mod(vm, "compile", compile, mod)
    
    return vm.get_builtin(BUILTIN_NULL_OBJ)



################################################################################
# class PCRE
#

class Pattern(Con_Boxed_Object):
    __slots__ = ("cp", "num_caps")
    _immutable_fields_ = ("cp", "num_caps")


    def __init__(self, vm, instance_of, cp, num_caps):
        Con_Boxed_Object.__init__(self, vm, instance_of)
        self.cp = cp
        self.num_caps = num_caps


@con_object_proc
def Pattern_match(vm):
    return _Pattern_match_search(vm, True)


@con_object_proc
def Pattern_search(vm):
    return _Pattern_match_search(vm, False)


def _Pattern_match_search(vm, anchored):
    mod = vm.get_funcs_mod()
    (self, s_o, sp_o),_ = vm.decode_args(mand="!S", opt="I", self_of=Pattern)
    assert isinstance(self, Pattern)
    assert isinstance(s_o, Con_String)
    
    ovect_size = (1 + self.num_caps) * 3
    ovect = lltype.malloc(rffi.INTP.TO, ovect_size, flavor="raw")
    if anchored:
        flags = PCRE_ANCHORED
    else:
        flags = 0
    sp = translate_idx_obj(vm, sp_o, len(s_o.v))
    rs, is_pinned, is_raw = rffi.get_nonmovingbuffer(s_o.v)
    r = int(pcre_exec(self.cp, None, rs, len(s_o.v), sp, flags, ovect, ovect_size))
    rffi.free_nonmovingbuffer(s_o.v, rs, is_pinned, is_raw)
    if r < 0:
        if r == PCRE_ERROR_NOMATCH:
            lltype.free(ovect, flavor="raw")
            return vm.get_builtin(BUILTIN_FAIL_OBJ)
        else:
            raise Exception("XXX")
            
    return Match(vm, mod.get_defn(vm, "Match"), ovect, self.num_caps, s_o)


def bootstrap_pattern_class(vm, mod):
    pattern_class = Con_Class(vm, Con_String(vm, "Pattern"), [vm.get_builtin(BUILTIN_OBJECT_CLASS)], mod)
    mod.set_defn(vm, "Pattern", pattern_class)

    new_c_con_func_for_class(vm, "match", Pattern_match, pattern_class)
    new_c_con_func_for_class(vm, "search", Pattern_search, pattern_class)



#
# func compile(s)
#
# This is defined here because it's tightly coupled to the Pattern class.
#

@con_object_proc
def compile(vm):
    mod = vm.get_funcs_mod()
    (pat,),_ = vm.decode_args("S")
    assert isinstance(pat, Con_String)
    
    errptr = lltype.malloc(rffi.CCHARPP.TO, 1, flavor="raw")
    erroff = lltype.malloc(rffi.INTP.TO, 1, flavor="raw")
    try:
        cp = pcre_compile(pat.v, PCRE_DOTALL | PCRE_MULTILINE, errptr, erroff, None)
        if cp is None:
            raise Exception("XXX")
    finally:
        lltype.free(errptr, flavor="raw")
        lltype.free(erroff, flavor="raw")

    with lltype.scoped_alloc(rffi.INTP.TO, 1) as num_capsp:
        r = int(pcre_fullinfo(cp, None, PCRE_INFO_CAPTURECOUNT, num_capsp))
        if r != 0:
            raise Exception("XXX")
        num_caps = int(num_capsp[0])

    return Pattern(vm, mod.get_defn(vm, "Pattern"), cp, num_caps)



################################################################################
# class Match
#

class Match(Con_Boxed_Object):
    __slots__ = ("ovect", "num_caps", "s")
    _immutable_fields_ = ("ovect", "num_caps", "s")


    def __init__(self, vm, instance_of, ovect, num_caps, s):
        Con_Boxed_Object.__init__(self, vm, instance_of)
        self.ovect = ovect
        self.num_caps = num_caps
        self.s = s


    def __del__(self):
        lltype.free(self.ovect, flavor="raw")


@con_object_proc
def Match_get(vm):
    (self, i_o),_ = vm.decode_args(mand="!I", self_of=Match)
    assert isinstance(self, Match)
    assert isinstance(i_o, Con_Int)
    
    # Group 0 in the match is the entire match, so when translating indices, we need to add 1 onto
	# num_captures.
    i = translate_idx(vm, i_o.v, 1 + self.num_caps)
    
    o = self.s.get_slice(vm, int(self.ovect[i * 2]), int(self.ovect[i * 2 + 1]))
    objectmodel.keepalive_until_here(self)
    return o


@con_object_proc
def Match_get_indexes(vm):
    (self, i_o),_ = vm.decode_args(mand="!I", self_of=Match)
    assert isinstance(self, Match)
    assert isinstance(i_o, Con_Int)
    
    i = translate_idx(vm, i_o.v, 1 + self.num_caps)
    
    o = Con_List(vm, [Con_Int(vm, int(self.ovect[i * 2])), Con_Int(vm, int(self.ovect[i * 2 + 1]))])
    objectmodel.keepalive_until_here(self)
    return o


def bootstrap_match_class(vm, mod):
    match_class = Con_Class(vm, Con_String(vm, "Match"), [vm.get_builtin(BUILTIN_OBJECT_CLASS)], mod)
    mod.set_defn(vm, "Match", match_class)

    new_c_con_func_for_class(vm, "get", Match_get, match_class)
    new_c_con_func_for_class(vm, "get_indexes", Match_get_indexes, match_class)
