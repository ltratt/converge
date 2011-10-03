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
from pypy.rlib import debug, jit
from pypy.rlib.rstacklet import StackletThread
from pypy.rpython.lltypesystem import lltype, llmemory, rffi

from Core import *
import Builtins, Modules, Target



class Return_Exception(Exception): pass

def get_printable_location(bc_off, mod_bc, pc):
    instr = Target.read_word(mod_bc, bc_off)
    it = Target.get_instr(instr)
    return "%s:%s at offset %s. bytecode: %s" % (pc.mod.name, pc.off, bc_off, Target.INSTR_NAMES[it])

jitdriver = jit.JitDriver(greens=["bc_off", "mod_bc", "pc"], reds=["cf", "self"],
                          get_printable_location=get_printable_location)


class VM(object):
    __slots__ = ("builtins", "cf_stack", "mods", "pypy_config", "st")
    _immutable_fields = ("builtins", "cf_stack", "mods")

    def __init__(self): 
        self.builtins = [None] * Builtins.NUM_BUILTINS
        self.mods = {}
        self.cf_stack = []
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


    def apply(self, func, args=None):
        o, _ = self.apply_closure(func, args)
        return o


    @jit.unroll_safe
    def apply_closure(self, func, args=None):
        if args is None: args = []
    
        cf = self._add_continuation_frame(func)
        self._cf_stack_extend(cf, list(args))
        self._cf_stack_append(cf, Builtins.new_con_int(self, len(args)))

        try:
            self.execute(self.st.get_null_handle())
        except Return_Exception:
            pass
        
        self._remove_continuation_frame()
        
        return self._cf_stack_pop(cf), cf.closure[-1]


    def _apply_pump(self, remove_generator_frames=False):
	    # At this point, we're in one of two situations:
	    #
	    #   1) A generator frame has been added but not yet used. So the con stack looks like:
	    #        [..., <generator frame>, <continuation frame>, <argument objects>]
	    #
	    #   2) We need to resume a previous generator. The con stack looks like:
	    #        [..., <generator frame>, <random objects>]
	    #
	    # Fortunately there's an easy way to distinguish the two: if the current continuation on the
	    # stack has a generator frame we're in situation #2, otherwise we're in situation #1.

        cf = self.cf_stack[-1]

        if cf.gfp == -1:
            # We're in case 1) from above.
            ct = self.st.new(switch_hack)
            o = self._cf_stack_pop(cf)
            if cf.returned:
                self._remove_continuation_frame()
                self._remove_generator_frame()
        else:
            # We're in case 2) from above.
            raise Exception("XXX")
        
        if o is None:
            raise Exception("XXX")

        return o


    @jit.unroll_safe
    def decode_args(self, as_):
        cf = self.cf_stack[-1]
        np_o = self._cf_stack_pop(cf)
        assert isinstance(np_o, Builtins.Con_Int)
        np = np_o.v # Num params
        nrmargs = [] # Normal args
        vargs = [] # Var args
        for i in range(len(as_)):
            if as_[i] == "O":
                nrmargs.append(cf.stack[cf.stackpe - np + i])
            elif as_[i] == "v":
                for j in range(i, np - i):
                    vargs.append(cf.stack[cf.stackpe - np + j])
                break
            else:
               raise Exception("XXX")

        for i in range(0, np):
            cf.stack[cf.stackpe - i - 1] = None
        cf.stackpe -= np        
        
        return (nrmargs, vargs)


    def return_(self, obj):
        assert isinstance(obj, Builtins.Con_Object)
        cf = self.cf_stack[-1]
        self._cf_stack_append(cf, obj)
        cf.returned = True
        if cf.ct:
            self.st.switch(cf.ct)
        else:
            raise Return_Exception()


    def execute(self, ct):
        cf = self.cf_stack[-1]
        cf.ct = ct
        pc = cf.pc
        if isinstance(pc, Py_PC):
            pc.f(self)
        else:
            self.bc_loop()
        
        #print "XXX"
        return ct


    def bc_loop(self):
        cf = self.cf_stack[-1]
        pc = cf.pc
        assert isinstance(pc, BC_PC)
        mod_bc = pc.mod.bc
        while 1:
            bc_off = cf.bc_off
            jitdriver.jit_merge_point(bc_off=bc_off, mod_bc=mod_bc, cf=cf, pc=pc, self=self)
            instr = Target.read_word(mod_bc, bc_off)
            it = Target.get_instr(instr)
            #print "%s %s %d [stackpe:%d ffp:%d gfp:%d xfp:%d]" % (str(cf.stack), Target.INSTR_NAMES[instr & 0xFF], bc_off, cf.stackpe, cf.ffp, cf.gfp, cf.xfp)
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
                if self._cf_stack_pop(cf) is self.builtins[Builtins.BUILTIN_FAIL_OBJ]:
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
                print it, cf.stack
                raise Exception("XXX")


    def _instr_var_lookup(self, instr, cf):
        closure_off, var_num = Target.unpack_var_lookup(instr)
        v = cf.closure[closure_off - 1][var_num]
        if v is None:
            raise Exception("XXX")
        self._cf_stack_append(cf, v)
        cf.bc_off += Target.INTSIZE


    def _instr_var_assign(self, instr, cf):
        closure_off, var_num = Target.unpack_var_assign(instr)
        cf.closure[len(cf.closure) - 1 - closure_off][var_num] = cf.stack[cf.stackpe - 1]
        cf.bc_off += Target.INTSIZE


    def _instr_int(self, instr, cf):
        self._cf_stack_append(cf, Builtins.new_con_int(self, Target.unpack_int(instr)))
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
        self._cf_stack_pop(cf)
        cf.bc_off += Target.INTSIZE


    def _instr_apply(self, instr, cf):
        is_fail_up, _ = self._read_failure_frame()
        num_args = Target.unpack_apply(instr)
        fp = cf.stackpe - num_args - 1
        func = cf.stack[fp]
        args = [None] * num_args
        for i in range(0, num_args):
            args[i] = cf.stack[cf.stackpe - i - 1]
            cf.stack[cf.stackpe - i - 1] = None
        cf.stackpe -= num_args

        if is_fail_up:
            gf = Stack_Generator_Frame(cf.gfp, cf.bc_off + Target.INTSIZE)
            cf.stack[fp] = gf
            cf.gfp = fp
            fp += 1
            new_cf = self._add_continuation_frame(func, True)
            self._cf_stack_extend(new_cf, args)
            self._cf_stack_append(new_cf, Builtins.new_con_int(self, num_args))
            o = self._apply_pump()
            if o is None:
                raise Exception("XXX")
        else:
            cf.stack[fp] = None
            cf.stackpe -= 1
            o, _ = self.apply_closure(func, args)
        self._cf_stack_append(cf, o)
        cf.bc_off += Target.INTSIZE


    def _instr_fail_now(self, instr, cf):
        self._fail_now()


    def _instr_func_defn(self, instr, cf):
        is_bound = Target.unpack_func_defn(instr)
        np_o = self._cf_stack_pop(cf)
        assert isinstance(np_o, Builtins.Con_Int)
        nv_o = self._cf_stack_pop(cf)
        assert isinstance(nv_o, Builtins.Con_Int)
        name = self._cf_stack_pop(cf)
        new_pc = BC_PC(cf.pc.mod, cf.bc_off + 2 * Target.INTSIZE)
        container = cf.func.get_slot("container")
        f = Builtins.new_bc_con_func(self, name, is_bound, new_pc, np_o.v, nv_o.v, \
          container, cf.closure)
        self._cf_stack_append(cf, f)
        cf.bc_off += Target.INTSIZE


    def _instr_return(self, instr, cf):
        self.return_(self._cf_stack_pop(cf))
        # Won't get here


    def _instr_import(self, instr, cf):
        mod = self.get_mod(cf.pc.mod.imps[Target.unpack_import(instr)])
        self._cf_stack_append(cf, mod)
        cf.bc_off += Target.INTSIZE


    def _instr_string(self, instr, cf):
        str_start, str_size = Target.unpack_string(instr)
        str_off = cf.bc_off + str_start
        assert str_off > 0 and str_size > 0
        str_ = rffi.charpsize2str(rffi.ptradd(cf.pc.mod.bc, str_off), str_size)
        self._cf_stack_append(cf, Builtins.new_con_string(self, str_))
        cf.bc_off += Target.align(str_start + str_size)


    def _instr_builtin_lookup(self, instr, cf):
        bl = Target.unpack_builtin_lookup(instr)
        assert self.builtins[bl] is not None
        self._cf_stack_append(cf, self.builtins[bl])
        cf.bc_off += Target.INTSIZE


    @jit.unroll_safe
    def _instr_unpack_args(self, instr, cf):
        num_fargs, has_vargs = Target.unpack_unpack_args(instr)
        na_o = self._cf_stack_pop(cf)
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
                        o = self._cf_stack_pop(cf)
                        assert isinstance(o, Builtins.Con_Object)
                    cf.closure[-1][Target.unpack_unpack_args_arg_num(arg_info)] = o

        if has_vargs:
            raise Exception("XXX")
        else:
            cf.bc_off += Target.INTSIZE + num_fargs * Target.INTSIZE


    def _instr_const_get(self, instr, cf):
        const_num = Target.unpack_constant_get(instr)
        if cf.pc.mod.consts[const_num] is not None:
            self._cf_stack_append(cf, cf.pc.mod.consts[const_num])
            cf.bc_off += Target.INTSIZE
        else:
            self._add_failure_frame(False, cf.bc_off)
            cf.bc_off = cf.pc.mod.get_const_create_off(self, const_num)


    def _instr_const_set(self, instr, cf):
        const_num = Target.unpack_constant_set(instr)
        cf.pc.mod.consts[const_num] = self._cf_stack_pop(cf)
        cf.bc_off += Target.INTSIZE


    def _instr_cmp(self, instr, cf):
        rhs = self._cf_stack_pop(cf)
        lhs = self._cf_stack_pop(cf)
        
        it = Target.get_instr(instr)
        if it == Target.CON_INSTR_EQ:
            r = lhs.eq(self, rhs)
        elif it == Target.CON_INSTR_GT:
            r = lhs.gt(self, rhs)
        else:
            raise Exception("XXX")
        
        if r:
            self._cf_stack_append(cf, rhs)
            cf.bc_off += Target.INTSIZE
        else:
            self._fail_now()


    def _instr_sub(self, instr, cf):
        rhs = self._cf_stack_pop(cf)
        lhs = self._cf_stack_pop(cf)
        self._cf_stack_append(cf, lhs.sub(self, rhs))
        cf.bc_off += Target.INTSIZE


    def _instr_module_lookup(self, instr, cf):
        o = self._cf_stack_pop(cf)
        nm_start, nm_size = Target.unpack_mod_lookup(instr)
        nm_off = cf.bc_off + nm_start
        assert nm_off > 0 and nm_size > 0
        nm = rffi.charpsize2str(rffi.ptradd(cf.pc.mod.bc, nm_off), nm_size)
        self._cf_stack_append(cf, o.get_defn(nm))
        cf.bc_off += Target.align(nm_start + nm_size)


    #
    # Stack operations
    #
    
    def _cf_stack_append(self, cf, x):
        cf.stack[cf.stackpe] = x
        cf.stackpe += 1

    def _cf_stack_extend(self, cf, l):
        for x in l:
            cf.stack[cf.stackpe] = x
            cf.stackpe += 1


    def _cf_stack_pop(self, cf):
        cf.stackpe -= 1
        o = cf.stack[cf.stackpe]
        cf.stack[cf.stackpe] = None
        return o


    def _cf_stack_insert(self, cf, i, x):
        for j in range(cf.stackpe, i, -1):
            cf.stack[j] = cf.stack[j - 1]
        cf.stack[i] = x
    

    def _add_continuation_frame(self, func, resumable = False):
        if not isinstance(func, Builtins.Con_Func):
            raise Exception("XXX")

        pc = func.pc
        if isinstance(pc, BC_PC):
            bc_off = pc.off
        else:
            bc_off = -1 

        if func.container_closure is not None:
            closure = func.container_closure + [[None] * func.num_vars]
        else:
            closure = [[None] * func.num_vars]

        cf = Stack_Continuation_Frame(func, pc, bc_off, closure, resumable)
        self.cf_stack.append(cf)
        
        return cf


    def _remove_continuation_frame(self):
        del self.cf_stack[-1]


    def _remove_generator_frame(self):
        cf = self.cf_stack[-1]
        
        gf = cf.stack[cf.gfp]
        assert isinstance(gf, Stack_Generator_Frame)
        
        for i in range(cf.gfp, cf.stackpe):
            cf.stack[i] = None
        cf.stackpe = cf.gfp
        cf.gfp = gf.prev_gfp


    def _add_failure_frame(self, is_fail_up, new_off=-1):
        cf = self.cf_stack[-1]

        if cf.ff_stack[cf.ffp + 1] is None:
            ff = Stack_Failure_Frame()
            cf.ff_stack[cf.ffp + 1] = ff

        ff = cf.ff_stack[cf.ffp + 1]
        ff.stackpe = cf.stackpe
        ff.is_fail_up = is_fail_up
        ff.prev_gfp = cf.gfp
        ff.fail_to_off = new_off

        cf.gfp = -1
        cf.ffp += 1
        

    def _remove_failure_frame(self):
        cf = self.cf_stack[-1]
        ff = cf.ff_stack[cf.ffp]
        for i in range(ff.stackpe, cf.stackpe):
            cf.stack[i] = None
        cf.stackpe = ff.stackpe
        cf.ffp -= 1


    def _read_failure_frame(self):
        cf = self.cf_stack[-1]
        ff = cf.ff_stack[cf.ffp]
        return (ff.is_fail_up, ff.fail_to_off)


    @jit.unroll_safe
    def _fail_now(self):
        cf = self.cf_stack[-1]
        while 1:
            is_fail_up, fail_to_off = self._read_failure_frame()
            if is_fail_up:
                if cf.gfp > -1:
                    raise Exception("XXX")
                else:
                    self._remove_failure_frame()
                    cf.bc_off = fail_to_off
            else:
                cf.bc_off = fail_to_off
                self._remove_failure_frame()
                break



global_vm = VM()

def new_vm():
    global_vm.init()
    return global_vm


class Stack_Continuation_Frame(Con_Thingy):
    __slots__ = ("stack", "stackpe", "ff_stack", "func", "pc", "bc_off", "closure", "ct", "ffp",
      "gfp", "xfp", "resumable", "returned")
    _immutable_fields_ = ("stack", "ff_stack", "func", "closure", "pc", "resumable")

    def __init__(self, func, pc, bc_off, closure, resumable):
        self.stack = [None] * 64
        debug.make_sure_not_resized(self.stack)
        self.ff_stack = [None] * 32
        debug.make_sure_not_resized(self.ff_stack)
        self.func = func
        self.pc = pc
        self.bc_off = bc_off # -1 for Py modules
        self.closure = closure
        self.resumable = resumable
        self.returned = False

        # stackpe always points to the element *after* the end of the stack (this makes a lot of
        # stack-based operations quicker)
        self.stackpe = 0
        # ffp, gfp, and xfp all point *to* the frame they refer to
        self.ffp = self.gfp = self.xfp = -1


class Stack_Failure_Frame(Con_Thingy):
    __slots__ = ("stackpe", "is_fail_up", "prev_gfp", "fail_to_off")


class Stack_Generator_Frame(Con_Thingy):
    __slots__ = ("prev_gfp", "resume_bc_off", "returned")
    _immutable_fields_ = ("prev_gfp", "resume_bc_off")
    
    def __init__(self, prev_gfp, resume_bc_off):
        self.prev_gfp = prev_gfp
        self.resume_bc_off = resume_bc_off
        self.returned = False


def switch_hack(ct, arg):
    global_vm.execute(ct)
    return ct