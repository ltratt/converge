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


from pypy.config.pypyoption import get_pypy_config
from pypy.rlib import jit
from pypy.rlib.rstacklet import StackletThread
from pypy.rpython.lltypesystem import lltype, llmemory, rffi

from Core import *
import Builtins, Modules, Target




class Return_Exception(Exception): pass

def get_printable_location(bc_off, mod_bc, pc):
    instr = Target.read_word(mod_bc, bc_off)
    it = Target.get_instr(instr)
    return "%s:%s at offset %s. bytecode: %s" % (pc.mod.name, pc.off, bc_off, it)

jitdriver = jit.JitDriver(greens=["bc_off", "mod_bc", "pc"], reds=["cf", "self"],
                          get_printable_location=get_printable_location)


class VM(object):
    __slots__ = ("builtins", "cfp", "ff_stack", "ffp", "mods", "pypy_config", "st", "stack")
    _immutable_fields = ("builtins", "mods", "stack", "ff_stack")

    def __init__(self): 
        self.builtins = [None] * Builtins.NUM_BUILTINS
        self.mods = {}
        self.stack = []
        # Failure frames are a real nusciance in an Icon-based system. We take the general idea
        # from the "Experiences with an Icon-like Expression Evaluation System" paper and try
        # to avoid continually putting these on the main stack.
        self.ff_stack = []
        self.ffp = -1
        self.cfp = -1
        self.pypy_config = None


    def init(self):
        self.st = StackletThread(self.pypy_config)
        
        for init_func in Modules.BUILTIN_MODULES:
            self.set_mod(init_func(self))


    def set_mod(self, mod):
        self.mods[mod.id_] = mod


    def get_mod(self, mod_id):
        m = self.mods[mod_id]
        if not m.initialized:
            m.import_(self)
        return m


    def apply(self, f, *args):
        o, _ = self.apply_closure(f, *args)
        return o


    @jit.unroll_safe
    def apply_closure(self, f, *args):
        self.stack.append(f)
        for a in list(args):
            self.stack.append(a)

        return self._apply_closure_on_stack(len(args))
        
        
    def _apply_closure_on_stack(self, num_args):
        cf = self._add_continuation_frame(num_args)
        try:
            self.execute(self.st.get_null_handle())
        except Return_Exception:
            pass
        
        o = self.stack.pop()
        self._remove_continuation_frame()
        
        return o, cf.closure[-1]


    def _apply_pump(self):
        ct = self.st.new(switch_hack)
        self.st.destroy(ct)
        o = self.stack.pop()
        self._remove_continuation_frame()
        return o


    @jit.unroll_safe
    def decode_args(self, as_):
        np_o = self.stack.pop()
        assert isinstance(np_o, Builtins.Con_Int)
        np = np_o.v # Num params
        nrmargs = [] # Normal args
        vargs = [] # Var args
        for i in range(len(as_)):
            if as_[i] == "O":
                nrmargs.append(self.stack[len(self.stack) - np + i])
            elif as_[i] == "v":
                for j in range(i, np - i):
                    vargs.append(self.stack[len(self.stack) - np + j])
                break
            else:
               raise Exception("XXX")
        
        s = len(self.stack) - np
        assert s > 0
        del self.stack[s : ]
        
        return (nrmargs, vargs)


    def return_(self, obj):
        assert isinstance(obj, Builtins.Con_Object)
        self.stack.append(obj)
                
        cf = self.stack[self.cfp]
        assert isinstance(cf, Stack_Continuation_Frame)
        if cf.ct:
            self.st.switch(cf.ct)
        else:
            raise Return_Exception()


    def execute(self, ct):
        cf = self.stack[self.cfp]
        assert isinstance(cf, Stack_Continuation_Frame)
        cf.ct = ct
        pc = cf.pc
        if isinstance(pc, Py_PC):
            pc.f(self)
        else:
            self.bc_loop()
        
        #print "XXX"
        return ct


    def bc_loop(self):
        cf = self.stack[self.cfp]
        assert isinstance(cf, Stack_Continuation_Frame)
        pc = cf.pc
        assert isinstance(pc, BC_PC)
        mod_bc = pc.mod.bc
        while 1:
            bc_off = cf.bc_off
            jitdriver.jit_merge_point(bc_off=bc_off, mod_bc=mod_bc, cf=cf, pc=pc, self=self)
            instr = Target.read_word(mod_bc, bc_off)
            it = Target.get_instr(instr)
            #print "%s %s %d [%d %d %d]" % (str(self.stack), Target.INSTR_NAMES[instr & 0xFF], bc_off, self.cfp, self.ffp, cf.xfp)
            if it == Target.CON_INSTR_VAR_LOOKUP:
                self._instr_var_lookup(instr, cf)
            elif it == Target.CON_INSTR_VAR_ASSIGN:
                self._instr_var_assign(instr, cf)
            elif it == Target.CON_INSTR_INT:
                self._instr_int(instr, cf)
            elif it == Target.CON_INSTR_ADD_FAILURE_FRAME:
                self._instr_add_failure_frame(instr, cf)
            elif it == Target.CON_INSTR_ADD_FAIL_UP_FRAME:
                self._instr_add_fail_up_frame(instr, cf)
            elif it == Target.CON_INSTR_REMOVE_FAILURE_FRAME:
                self._instr_remove_failure_frame(instr, cf)
            elif it == Target.CON_INSTR_FAIL_NOW:
                self._instr_fail_now(instr, cf)
            elif it == Target.CON_INSTR_POP:
                self._instr_pop(instr, cf)
            elif it == Target.CON_INSTR_APPLY:
                self._instr_apply(instr, cf)
            elif it == Target.CON_INSTR_FUNC_DEFN:
                self._instr_func_defn(instr, cf)
            elif it == Target.CON_INSTR_RETURN:
                self._instr_return(instr, cf)
            elif it == Target.CON_INSTR_BRANCH:
                j = Target.unpack_branch(instr)
                cf.bc_off += j
                if j < 0:
                    bc_off=cf.bc_off
                    jitdriver.can_enter_jit(bc_off=bc_off, mod_bc=mod_bc, cf=cf, pc=pc, self=self)
            elif it == Target.CON_INSTR_IMPORT:
                self._instr_import(instr, cf)
            elif it == Target.CON_INSTR_STRING:
                self._instr_string(instr, cf)
            elif it == Target.CON_INSTR_BUILTIN_LOOKUP:
                self._instr_builtin_lookup(instr, cf)
            elif it == Target.CON_INSTR_UNPACK_ARGS:
                self._instr_unpack_args(instr, cf)
            elif it == Target.CON_INSTR_CONST_GET:
                self._instr_const_get(instr, cf)
            elif it == Target.CON_INSTR_CONST_SET:
                self._instr_const_set(instr, cf)
            elif it == Target.CON_INSTR_BRANCH_IF_NOT_FAIL:
                if self.stack.pop() is self.builtins[Builtins.BUILTIN_FAIL_OBJ]:
                    cf.bc_off += Target.INTSIZE
                else:
                    j = Target.unpack_branch_if_not_fail(instr)
                    cf.bc_off += j
                    if j < 0:
                        bc_off=cf.bc_off
                        jitdriver.can_enter_jit(bc_off=bc_off, mod_bc=mod_bc, cf=cf, pc=pc, self=self)
            elif it == Target.CON_INSTR_EQ or it == Target.CON_INSTR_GT:
                self._instr_cmp(instr, cf)
            elif it == Target.CON_INSTR_SUBTRACT:
                self._instr_sub(instr, cf)
            elif it == Target.CON_INSTR_MODULE_LOOKUP:
                self._instr_module_lookup(instr, cf)
            else:
                print it, self.stack
                raise Exception("XXX")


    def _instr_var_lookup(self, instr, cf):
        closure_off, var_num = Target.unpack_var_lookup(instr)
        v = cf.closure[closure_off - 1][var_num]
        if v is None:
            raise Exception("XXX")
        self.stack.append(v)
        cf.bc_off += Target.INTSIZE


    def _instr_var_assign(self, instr, cf):
        closure_off, var_num = Target.unpack_var_assign(instr)
        cf.closure[len(cf.closure) - 1 - closure_off][var_num] = self.stack[-1]
        cf.bc_off += Target.INTSIZE


    def _instr_int(self, instr, cf):
        self.stack.append(Builtins.new_con_int(self, Target.unpack_int(instr)))
        cf.bc_off += Target.INTSIZE


    def _instr_add_failure_frame(self, instr, cf):
        off = Target.unpack_add_failure_frame(instr)
        self._add_failure_frame(False, cf.bc_off + off)
        cf.bc_off += Target.INTSIZE


    def _instr_add_fail_up_frame(self, instr, cf):
        self._add_failure_frame(True)
        cf.bc_off += Target.INTSIZE


    def _instr_remove_failure_frame(self, instr, cf):
        self._remove_failure_frame()
        cf.bc_off += Target.INTSIZE


    def _instr_pop(self, instr, cf):
        assert isinstance(self.stack[-1], Builtins.Con_Object)
        del self.stack[-1]
        cf.bc_off += Target.INTSIZE


    def _instr_apply(self, instr, cf):
        is_fail_up, _ = self._read_failure_frame()
        num_args = Target.unpack_apply(instr)

        if is_fail_up:
            self._add_continuation_frame(num_args, cf.bc_off + Target.INTSIZE)
            o = self._apply_pump()
            if o is None:
                raise Exception("XXX")
        else:
            o, _ = self._apply_closure_on_stack(num_args)
        self.stack.append(o)
        cf.bc_off += Target.INTSIZE


    def _instr_fail_now(self, instr, cf):
        self._fail_now()


    def _instr_func_defn(self, instr, cf):
        is_bound = Target.unpack_func_defn(instr)
        np_o = self.stack.pop()
        assert isinstance(np_o, Builtins.Con_Int)
        nv_o = self.stack.pop()
        assert isinstance(nv_o, Builtins.Con_Int)
        name = self.stack.pop()
        new_pc = BC_PC(cf.pc.mod, cf.bc_off + 2 * Target.INTSIZE)
        container = cf.func.get_slot("container")
        f = Builtins.new_bc_con_func(self, name, is_bound, new_pc, np_o.v, nv_o.v, \
          container, cf.closure)
        self.stack.append(f)
        cf.bc_off += Target.INTSIZE


    def _instr_return(self, instr, cf):
        self.return_(self.stack.pop())
        # Won't get here


    def _instr_import(self, instr, cf):
        mod = self.get_mod(cf.pc.mod.imps[Target.unpack_import(instr)])
        self.stack.append(mod)
        cf.bc_off += Target.INTSIZE


    def _instr_string(self, instr, cf):
        str_start, str_size = Target.unpack_string(instr)
        str_off = cf.bc_off + str_start
        assert str_off > 0 and str_size > 0
        str_ = rffi.charpsize2str(rffi.ptradd(cf.pc.mod.bc, str_off), str_size)
        self.stack.append(Builtins.new_con_string(self, str_))
        cf.bc_off += Target.align(str_start + str_size)


    def _instr_builtin_lookup(self, instr, cf):
        bl = Target.unpack_builtin_lookup(instr)
        assert self.builtins[bl] is not None
        self.stack.append(self.builtins[bl])
        cf.bc_off += Target.INTSIZE


    @jit.unroll_safe
    def _instr_unpack_args(self, instr, cf):
        num_fargs, has_vargs = Target.unpack_unpack_args(instr)
        na_o = self.stack.pop()
        assert isinstance(na_o, Builtins.Con_Int)
        num_params = na_o.v
        if num_params > num_fargs and not has_vargs:
            raise Exception("XXX")

        if num_fargs > 0:
            arg_offset = cf.bc_off + Target.INTSIZE + num_fargs * Target.INTSIZE
            for i in range(num_fargs - 1, -1, -1):
                arg_offset -= Target.INTSIZE
                arg_info = Target.read_word(cf.pc.mod.bc, arg_offset)
                if i > num_params:
                    if Target.unpack_unpack_args_is_mandatory(arg_info):
                        raise Exception("XXX")
                else:
                    if num_params > num_fargs:
                        raise Exception("XXX")
                    else:
                        o = self.stack.pop()
                        assert isinstance(o, Builtins.Con_Object)
                    cf.closure[-1][Target.unpack_unpack_args_arg_num(arg_info)] = o

        if has_vargs:
            raise Exception("XXX")
        else:
            cf.bc_off += Target.INTSIZE + num_fargs * Target.INTSIZE


    def _instr_const_get(self, instr, cf):
        const_num = Target.unpack_constant_get(instr)
        if cf.pc.mod.consts[const_num] is not None:
            self.stack.append(cf.pc.mod.consts[const_num])
            cf.bc_off += Target.INTSIZE
        else:
            self._add_failure_frame(False, cf.bc_off)
            cf.bc_off = cf.pc.mod.get_const_create_off(self, const_num)


    def _instr_const_set(self, instr, cf):
        const_num = Target.unpack_constant_set(instr)
        cf.pc.mod.consts[const_num] = self.stack.pop()
        cf.bc_off += Target.INTSIZE


    def _instr_cmp(self, instr, cf):
        rhs = self.stack.pop()
        lhs = self.stack.pop()
        
        it = Target.get_instr(instr)
        if it == Target.CON_INSTR_EQ:
            r = lhs.eq(self, rhs)
        elif it == Target.CON_INSTR_GT:
            r = lhs.gt(self, rhs)
        else:
            raise Exception("XXX")
        
        if r:
            self.stack.append(rhs)
            cf.bc_off += Target.INTSIZE
        else:
            self._fail_now()


    def _instr_sub(self, instr, cf):
        rhs = self.stack.pop()
        lhs = self.stack.pop()
        self.stack.append(lhs.sub(self, rhs))
        cf.bc_off += Target.INTSIZE


    def _instr_module_lookup(self, instr, cf):
        o = self.stack.pop()
        nm_start, nm_size = Target.unpack_mod_lookup(instr)
        nm_off = cf.bc_off + nm_start
        assert nm_off > 0 and nm_size > 0
        nm = rffi.charpsize2str(rffi.ptradd(cf.pc.mod.bc, nm_off), nm_size)
        self.stack.append(o.get_defn(nm))
        cf.bc_off += Target.align(nm_start + nm_size)


    #
    # Stack operations
    #

    def _add_continuation_frame(self, num_args, resumption_bc_off = -1):
        fp = len(self.stack) - 1 - num_args
        f = self.stack[fp]
        if not isinstance(f, Builtins.Con_Func):
            raise Exception("XXX")

        pc = f.pc
        if isinstance(pc, BC_PC):
            bc_off = pc.off
        else:
            bc_off = -1 

        if f.container_closure is not None:
            closure = f.container_closure + [[None] * f.num_vars]
        else:
            closure = [[None] * f.num_vars]

        cf = Stack_Continuation_Frame(self.cfp, self.ffp, f, pc, bc_off, closure, resumption_bc_off)

        self.cfp = fp
        self.stack[fp] = cf
        self.stack.append(Builtins.new_con_int(self, num_args))
        
        return cf


    def _remove_continuation_frame(self):
        old_cfp = self.cfp
        assert old_cfp >= 0
        cf = self.stack[old_cfp]
        assert isinstance(cf, Stack_Continuation_Frame)
        self.cfp = cf.prev_cfp
        del self.stack[old_cfp:]
        self.ffp = cf.prev_ffp


    def _add_failure_frame(self, is_fail_up, new_off=-1):
        cf = self.stack[self.cfp]
        assert isinstance(cf, Stack_Continuation_Frame)

        if self.ffp + 1 == len(self.ff_stack):
            ff = Stack_Failure_Frame()
            self.ff_stack.append(ff)

        ff = self.ff_stack[self.ffp + 1]
        ff.stackp = len(self.stack)
        ff.is_fail_up = is_fail_up
        ff.prev_gfp = cf.gfp
        ff.fail_to_off = new_off
        cf.gfp = -1
        self.ffp += 1
        

    def _remove_failure_frame(self):
        cf = self.stack[self.cfp]
        assert isinstance(cf, Stack_Continuation_Frame)
        
        ff = self.ff_stack[self.ffp]
        del self.stack[ff.stackp:]
        cf.gfp = ff.prev_gfp

        self.ffp -= 1


    def _read_failure_frame(self):
        cf = self.stack[self.cfp]
        assert isinstance(cf, Stack_Continuation_Frame)
        
        ff = self.ff_stack[self.ffp]

        return (ff.is_fail_up, ff.fail_to_off)


    @jit.unroll_safe
    def _fail_now(self):
        cf = self.stack[self.cfp]
        assert isinstance(cf, Stack_Continuation_Frame)
        while 1:
            is_fail_up, fail_to_off = self._read_failure_frame()
            if is_fail_up:
                raise Exception("XXX")
            else:
                cf.bc_off = fail_to_off
                self._remove_failure_frame()
                break



global_vm = VM()

def new_vm():
    global_vm.init()
    return global_vm


class Stack_Continuation_Frame(Con_Thingy):
    __slots__ = ("prev_cfp", "prev_ffp", "func", "pc", "bc_off", "closure", "ct", "gfp", "xfp",
      "resumption_bc_off")
    _immutable_fields_ = ("prev_cfp", "prev_ffp", "closure", "pc", "resumption_bc_off")

    def __init__(self, prev_cfp, prev_ffp, func, pc, bc_off, closure, resumption_bc_off):
        self.prev_cfp = prev_cfp
        self.prev_ffp = prev_ffp
        self.func = func
        self.pc = pc
        self.bc_off = bc_off # -1 for Py modules
        self.closure = closure
        self.resumption_bc_off = resumption_bc_off

        self.gfp = self.xfp = -1


class Stack_Failure_Frame(Con_Thingy):
    __slots__ = ("stackp", "is_fail_up", "prev_gfp", "prev_ffp", "fail_to_off")


class Stack_Generator_Frame(Con_Thingy):
    pass


def switch_hack(ct, arg):
    global_vm.execute(ct)
    return ct
