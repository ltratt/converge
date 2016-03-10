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


import Stdlib_Modules, Target
from Builtins import *




def init(vm):
    return new_c_con_module(vm, "C_Earley_Parser", "C_Earley_Parser", __file__, import_, \
      ["Parser"])


@con_object_proc
def import_(vm):
    (mod,),_ = vm.decode_args("O")

    bootstrap_parser_class(vm, mod)

    return vm.get_builtin(BUILTIN_NULL_OBJ)


################################################################################
# class Parser
#

_COMPILED_OFFSET_TO_PRODUCTIONS = 0
_COMPILED_OFFSET_TO_ALTERNATIVES_MAP = 1
_COMPILED_OFFSET_TO_RECOGNISER_BRACKETS_MAPS = 2
_COMPILED_OFFSET_TO_PARSER_BRACKETS_MAPS = 3

_COMPILED_PRODUCTIONS_NUM = 0
_COMPILED_PRODUCTIONS_OFFSETS = 1

_COMPILED_PRODUCTION_PRECEDENCE = 0
_COMPILED_PRODUCTION_PARENT_RULE = 1
_COMPILED_PRODUCTION_NUM_SYMBOLS = 2
_COMPILED_PRODUCTION_SYMBOLS = 3

_SYMBOL_RULE_REF = 0
_SYMBOL_TOKEN = 1
_SYMBOL_OPEN_KLEENE_STAR_GROUP = 2
_SYMBOL_CLOSE_KLEENE_STAR_GROUP = 3
_SYMBOL_OPEN_OPTIONAL_GROUP = 4
_SYMBOL_CLOSE_OPTIONAL_GROUP = 5

_COMPILED_ALTERNATIVES_MAP_NUM = 0
_COMPILED_ALTERNATIVES_MAP_OFFSETS = 1

_ALTERNATIVE_MAP_IS_NULLABLE = 0
_ALTERNATIVE_MAP_NUM_ENTRIES = 1
_ALTERNATIVE_MAP_ENTRIES = 2

_COMPILED_BRACKETS_MAPS_NUM_ENTRIES = 0
_COMPILED_BRACKETS_MAPS_ENTRIES = 1

_BRACKET_MAP_NUM_ENTRIES = 0
_BRACKET_MAP_ENTRIES = 1


class Parser:
    __slots__ = ("grm", "toks", "items")
    _immutable_fields_ = ("grm", "toks", "items")

    def __init__(self, grm, toks):
        self.grm = grm
        self.toks = toks
        self.items = []


class Alt:
    __slots__ = ("parent_rule", "precedence", "syms")
    _immutable_fields_ = ("parent_rule", "precedence", "syms")
    
    def __init__(self, parent_rule, precedence, syms):
        self.parent_rule = parent_rule
        self.precedence = precedence
        self.syms = syms


class Item:
    __slots__ = ("s", "d", "j", "w")
    # s is the rule number
    # d is the position of the Earley "dot" within the rule we've currently reached
    # j is the position in the token input we've currently reached
    # w is the SPPF tree being built
    _immutable_fields_ = ("s", "d", "j", "w")
    
    def __init__(self, s, d, j, w):
        self.s = s
        self.d = d
        self.j = j
        self.w = w

    def __eq__(self, o):
        if o.s == self.s and o.d == self.d and o.j == self.j and o.w == self.w:
            return 0
        else:
            return -1

    def __repr__(self):
        return "Item(%s, %s, %s, %s)" % (self.s, self.d, self.j, self.w)



class Tree:
    __slots__ = ()


class Tree_Non_Term(Tree):
    __slots__ = ("s", "complete", "j", "i", "precedences", "families", "flattened")
    _immutable_fields_ = ("t", "complete", "j", "i")

    def __init__(self, s, complete, j, i):
        self.s = s
        self.complete = complete
        self.j = j
        self.i = i
        self.precedences = None
        self.families = None
        self.flattened = None


    def __eq__(self, o):
        raise Exception("XXX")


    def pp(self, indent=0, names=None, alts=None):
        out = ["%sNT: %s%s %d %d\n" % (" " * indent, names[alts[self.s].parent_rule], self.s, self.j, self.i)]

        for k in self.flattened:
            out.append(k.pp(indent + 1, names, alts))

        return "".join(out)


    def _seen(self, seen, n):
        for cnd in seen:
            if cnd is n:
                return True
        return False


class Tree_Term(Tree):
    __slots__ = ("t", "j", "i")
    _immutable_fields_ = ("t", "j", "i")

    def __init__(self, t, j, i):
        self.t = t
        self.j = j
        self.i = i


    def pp(self, indent=0, names=None, alts=None):
        return "%sT: %d %d %d\n" % (" " * indent, self.t, self.j, self.i)


@con_object_proc
def Parser_parse(vm):
    (self, grm_o, rns_o, toksmap_o, toks_o),_ = vm.decode_args("OSOOO")
    assert isinstance(grm_o, Con_String)
    
    toks = [-1] # Tokens: list(int)
    tok_os = [None]
    vm.pre_get_slot_apply_pump(toks_o, "iter")
    while 1:
        e_o = vm.apply_pump()
        if not e_o:
            break
        tok_o = vm.get_slot_apply(toksmap_o, "get", [e_o.get_slot(vm, "type")])
        tok_os.append(e_o)
        toks.append(type_check_int(vm, tok_o).v)

    rn_os = [] # Rule names: list(Con_String)
    vm.pre_get_slot_apply_pump(rns_o, "iter")
    while 1:
        e_o = vm.apply_pump()
        if not e_o:
            break
        rn_os.append(e_o)

    grm = []
    grm_s = grm_o.v
    if len(grm_s) % Target.INTSIZE != 0:
        raise Exception("XXX")
    for i in range(0, len(grm_s), Target.INTSIZE):
        if Target.INTSIZE == 8:
            w = ord(grm_s[i]) + (ord(grm_s[i + 1]) << 8) + (ord(grm_s[i + 2]) << 16) \
                  + (ord(grm_s[i + 3]) << 24) + (ord(grm_s[i + 4]) << 32) + \
                  + (ord(grm_s[i + 5]) << 40) + (ord(grm_s[i + 6]) << 48) + \
                  (ord(grm_s[i + 7]) << 56)
        else:
            w = ord(grm_s[i]) + (ord(grm_s[i + 1]) << 8) + (ord(grm_s[i + 2]) << 16) \
                  + (ord(grm_s[i + 3]) << 24)
        grm.append(w)

    alts_off = grm[_COMPILED_OFFSET_TO_PRODUCTIONS]
    num_alts = grm[alts_off + _COMPILED_PRODUCTIONS_NUM] - 1
    alts = [None] * num_alts
    for i in range(num_alts):
        alt_off = grm[alts_off + _COMPILED_PRODUCTIONS_OFFSETS + i]
        assert alt_off >= 0
        alt_num_syms = grm[alt_off + _COMPILED_PRODUCTION_NUM_SYMBOLS]
        assert alt_num_syms >= 0
        syms = grm[alt_off + _COMPILED_PRODUCTION_SYMBOLS : \
          alt_off + _COMPILED_PRODUCTION_SYMBOLS + alt_num_syms]
        alt = Alt(grm[alt_off + _COMPILED_PRODUCTION_PARENT_RULE], \
          grm[alt_off + _COMPILED_PRODUCTION_PRECEDENCE], syms)
        alts[i] = alt

    b_maps_off = grm[_COMPILED_OFFSET_TO_RECOGNISER_BRACKETS_MAPS]
    num_b_maps = grm[b_maps_off + _COMPILED_BRACKETS_MAPS_NUM_ENTRIES]
    b_maps = [None] * num_b_maps
    for i in range(num_b_maps):
        b_map_off = grm[b_maps_off + _COMPILED_BRACKETS_MAPS_ENTRIES + i]
        assert b_map_off >= 0
        b_map_len = grm[b_map_off + _BRACKET_MAP_NUM_ENTRIES]
        assert b_map_len >= 0
        b_map = grm[b_map_off + _BRACKET_MAP_ENTRIES : b_map_off + _BRACKET_MAP_ENTRIES + b_map_len]
        b_maps[i] = b_map
    
    E = _parse(vm, self, toks, alts, b_maps)

    for k, v in E[len(toks) - 1].items():
        for T in v:
            T_alt = alts[T.s]
            if T.s == 0 and T.d == len(T_alt.syms) and T.j == 0:
                c = T.w
                assert isinstance(c, Tree_Non_Term)
                _resolve_ambiguities(vm, alts, tok_os, rn_os, c)
                int_tree = c.families[0][0]
                src_infos = tok_os[1].get_slot(vm, "src_infos")
                first_src_info = vm.get_slot_apply(src_infos, "get", [Con_Int(vm, 0)])
                src_file = vm.get_slot_apply(first_src_info, "get", [Con_Int(vm, 0)])
                src_off = type_check_int(vm, vm.get_slot_apply(first_src_info, "get", [Con_Int(vm, 1)])).v
                #rn_ss = [type_check_string(vm, x).v for x in rn_os]
                #print int_tree.pp(0, names=rn_ss, alts=alts)
                n, _, _ = _int_tree_to_ptree(vm, alts, tok_os, rn_os, int_tree, src_file, src_off)
                return n

    for i in range(len(tok_os) - 1, -1, -1):
        if len(E[i]) > 0:
            vm.get_slot_apply(self, "error", [tok_os[i]])


#
# This is an extension of the Earley parsing algorithm described in:
#   Recognition is not parsing - SPPF-style parsing from cubic recognisers
#   Elizabeth Scott, Adrian Johnstone
#   Science of Computer Programming 75 (2010) 55-70
# In particular we extend this with the Kleene star ()* and optional ()? operators.
#
# "line X" is a reference to the line numbered X (starting from 1) in the Scott / Johnstone
# algorithm.
#

def _parse(vm, self, toks, alts, b_maps):
    E = [{} for x in range(len(toks))]
    _add_to_e_set(E[0], Item(0, 0, 0, None))
    Qd = {}
    V = {}
    for i in range(0, len(toks)):
        H = {}
        R = _clone_e_set(E[i])
        Q = Qd
        Qd = {}
        while len(R) > 0:
            Lam = _pop_e_set(R)
            B_alt = alts[Lam.s]
            if Lam.d < len(B_alt.syms) and B_alt.syms[Lam.d] == _SYMBOL_RULE_REF: # line 9
                C = B_alt.syms[Lam.d + 1]
                for C_cnd in range(len(alts)): # line 10
                    C_cnd_alt = alts[C_cnd]
                    if C_cnd_alt.parent_rule != C: # line 10
                        continue
                    for p in _get_all_pos(alts, b_maps, C_cnd, 0):
                        e = Item(C_cnd, p, i, None)
                        if _sigma_d_at(alts, toks, C_cnd, p):
                            # line 11
                            if not _is_in_e_set(E[i], e):
                                _add_to_e_set(E[i], e)
                                _add_to_e_set(R, e)
                        elif _tok_match(alts, toks, C_cnd, p, i + 1):
                            _add_to_e_set(Q, Item(C_cnd, p, i, None))
                if C in H:
                    for p in _get_all_pos(alts, b_maps, Lam.s, Lam.d + 2):
                        y = _make_node(alts, toks, b_maps, Lam.s, p, Lam.j, i, Lam.w, H[C], V) # line 15
                        e = Item(Lam.s, p, Lam.j, y)
                        if _sigma_d_at(alts, toks, Lam.s, p):
                            # line 16
                            if not _is_in_e_set(E[i], e):
                                _add_to_e_set(E[i], e)
                                _add_to_e_set(R, e)
                        elif _tok_match(alts, toks, Lam.s, p, i + 1):
                            _add_to_e_set(Q, e)
            elif Lam.d == len(B_alt.syms): # line 19
                w = Lam.w
                if w is None: # line 20
                    D = (B_alt.parent_rule, -1, i, i)
                    w = V.get(D, None)
                    if w is None:
                        # line 21
                        w = Tree_Non_Term(Lam.s, True, i, i)
                        V[D] = w
                if Lam.j == i: # line 24
                    H[Lam.s] = w
                for es in E[Lam.j].values(): # line 25
                    for A in es:
                        A_alt = alts[A.s]
                        if A.d == len(A_alt.syms) or A_alt.syms[A.d] != _SYMBOL_RULE_REF \
                          or A_alt.syms[A.d + 1] != B_alt.parent_rule:
                            continue
                        for p in _get_all_pos(alts, b_maps, A.s, A.d + 2):
                            y = _make_node(alts, toks, b_maps, A.s, p, A.j, i, A.w, w, V) # line 26
                            # line 27
                            if _sigma_d_at(alts, toks, A.s, p):
                                e = Item(A.s, p, A.j, y)
                                if not _is_in_e_set(E[i], e):
                                    _add_to_e_set(E[i], e)
                                    _add_to_e_set(R, e)
                            elif _tok_match(alts, toks, A.s, p, i + 1): # line 29
                                _add_to_e_set(Q, Item(A.s, p, A.j, y))

        V = {}
        if i < len(toks) - 1:
            v = Tree_Term(toks[i + 1], i, i + 1)
            while len(Q) > 0:
                Lam = _pop_e_set(Q)
                B_alt = alts[Lam.s]
                assert Lam.d < len(B_alt.syms) and B_alt.syms[Lam.d] == _SYMBOL_TOKEN \
                  and B_alt.syms[Lam.d + 1] == toks[i + 1]
                for p in _get_all_pos(alts, b_maps, Lam.s, Lam.d + 2):
                    y = _make_node(alts, toks, b_maps, Lam.s, p, Lam.j, i + 1, Lam.w, v, V)
                    e = Item(Lam.s, p, Lam.j, y)
                    if _sigma_d_at(alts, toks, Lam.s, p):
                        # line 36
                        _add_to_e_set(E[i + 1], e)
                    elif _tok_match(alts, toks, Lam.s, p, i + 2):
                        # line 37
                        _add_to_e_set(Qd, e)

    return E


def _sigma_d_at(alts, toks, s, d):
    alt = alts[s]
    if d == len(alt.syms) or alt.syms[d] == _SYMBOL_RULE_REF:
        return True
    return False


def _tok_match(alts, toks, s, d, tok_i):
    alt = alts[s]
    if tok_i < len(toks) and d < len(alt.syms) and alt.syms[d] == _SYMBOL_TOKEN \
      and alt.syms[d + 1] == toks[tok_i]:
        return True
    return False


def _get_all_pos(alts, b_maps, s, d):
    alt = alts[s]
    if d < len(alt.syms) and alt.syms[d] in \
      (_SYMBOL_OPEN_KLEENE_STAR_GROUP, _SYMBOL_CLOSE_KLEENE_STAR_GROUP, \
      _SYMBOL_OPEN_OPTIONAL_GROUP, _SYMBOL_CLOSE_OPTIONAL_GROUP):
        return b_maps[alt.syms[d + 1]]
    else:
        return [d]


def _is_in_e_set(s, e):
    cnds = s.get((e.s, e.d, e.j), None)
    if cnds is None:
        return False
    for cnd_e in cnds:
        if e.w is cnd_e.w:
            return True
    return False


def _add_to_e_set(s, e):
    lab = (e.s, e.d, e.j)
    cnds = s.get(lab, None)
    if cnds is None:
        s[lab] = [e]
    else:
        for cnd_e in cnds:
            if e.w is cnd_e.w:
                return
        else:
            cnds.append(e)


def _clone_e_set(s):
    n = {}
    for k, v in s.items():
        n[k] = v[:]
    return n


def _pop_e_set(s):
    assert len(s) > 0
    for k, v in s.items():
        e = v.pop()
        if len(v) == 0:
            del s[k]
        return e
    raise Exception("Shouldn't get here")


def _make_node(alts, toks, b_maps, B, d, j, i, w, v, V):
    B_alt = alts[B]
    if d == len(B_alt.syms):
        lab = (B_alt.parent_rule, -1, j, i)
    else:
        lab = (B, d, j, i)

    # The Kleene star and optional brackets complicate the case presented in line 46
    # since we have no obvious \alpha to compare to the empty string. What we note is that if
    # this label has not previously been seen, we can search leftwards in the alternative to
    # see if a rule / token must have been found (in other words, we skip over brackets, which
    # we can't be sure of either way); if it has, we know \alpha \neq \epsilon, and can move
    # on accordingly.
    y = V.get(lab, None)
    if y is None:
        if d < len(B_alt.syms):
            p = d
            found = False
            while p >= 0:
                if B_alt.syms[p] in (_SYMBOL_RULE_REF, _SYMBOL_TOKEN):
                    found = True
                elif B_alt.syms[p] in (_SYMBOL_CLOSE_KLEENE_STAR_GROUP, _SYMBOL_CLOSE_OPTIONAL_GROUP):
                    p -= 2
                    while B_alt.syms[p] not in (_SYMBOL_OPEN_KLEENE_STAR_GROUP, _SYMBOL_CLOSE_KLEENE_STAR_GROUP):
                        p -= 2
                p -= 2
            if not found:
                return v
        if lab[1] == -1:
            y = Tree_Non_Term(B, True, j, i)
        else:
            y = Tree_Non_Term(B, False, j, i)
        V[lab] = y

    if w is None:
        kids = [v]
    else:
        kids = [w, v]

    if y.families is None:
        y.families = [kids]
        y.precedences = [B_alt.precedence]
    else:
        for cnd_kids in y.families:
            if len(kids) != len(cnd_kids) or kids[0] is not cnd_kids[0]:
                continue
            if len(kids) == 2 and kids[1] is not cnd_kids[1]:
                continue
            break
        else:
            y.families.append(kids)
            y.precedences.append(B_alt.precedence)

    return y


def _resolve_ambiguities(vm, alts, tok_os, rn_os, n):
    # This is a fairly lazy ambiguity resolution scheme - it only looks 2 levels deep. That's enough
    # for current purposes.
    if isinstance(n, Tree_Term):
        return
    assert isinstance(n, Tree_Non_Term)
    if n.families is not None:
        # The basic approach is to resolve all ambiguities in children first, before trying to
        # resolve ambiguity in 'n' itself.
        
        for kids in n.families:
            for c in kids:
                _resolve_ambiguities(vm, alts, tok_os, rn_os, c)

        if len(n.families) == 1:
            n.flattened = _flatten_kids(n.families[0])
            return
        
        if 0 not in n.precedences:
            lp = hp = n.precedences[0]
            for p in n.precedences:
                lp = min(lp, p)
                hp = max(hp, p)
            if lp != hp:
                j = 0
                while j < len(n.families):
                    if n.precedences[j] != lp:
                        del n.precedences[j]
                        del n.families[j]
                    else:
                        j += 1
                if len(n.families) == 1:
                    n.flattened = _flatten_kids(n.families[0])
                    return

        ffamilies = [_flatten_kids(k) for k in n.families]
        for fkids in ffamilies:
            if len(fkids) != len(ffamilies[0]):
                break
        
        if len(ffamilies) > 1:
            # We still have ambiguities left, so, as a sensible default, we prefer
            # left-associative parses.
            i = 0
            while i < len(ffamilies) - 1:
                len1 = len(ffamilies[i])
                len2 = len(ffamilies[i + 1])
                if len1 < len2:
                    del ffamilies[i]
                elif len2 < len1:
                    del ffamilies[i + 1]
                else:
                    i += 1
            if len(ffamilies) > 1:
                for i in range(len(ffamilies[0])):
                    j = 0
                    while j < len(ffamilies) - 1:
                        len1 = _max_depth(ffamilies[j][i])
                        len2 = _max_depth(ffamilies[j + 1][i])
                        if len1 < len2:
                            del ffamilies[j]
                        elif len2 < len1:
                            del ffamilies[j + 1]
                        else:
                            j += 1
                    if len(ffamilies) == 1:
                        break
                
        n.flattened = ffamilies[0]
    else:
        n.flattened = []


def _int_tree_to_ptree(vm, alts, tok_os, rn_os, n, src_file, src_off):
    if isinstance(n, Tree_Non_Term):
        tree_mod = vm.import_stdlib_mod(Stdlib_Modules.STDLIB_CPK_TREE)
        non_term_class = tree_mod.get_defn(vm, "Non_Term")

        name = rn_os[alts[n.s].parent_rule]
        kids, new_src_off, new_src_len = _int_tree_to_ptree_kids(vm, alts, tok_os, rn_os, n, src_file, src_off)
        
        src_infos = Con_List(vm, [Con_List(vm, [src_file, Con_Int(vm, new_src_off), Con_Int(vm, new_src_len)])])
        rn = vm.get_slot_apply(non_term_class, "new", [name, Con_List(vm, kids), src_infos])
    else:
        assert isinstance(n, Tree_Term)
        rn = tok_os[n.j + 1]
        new_src_off, new_src_len = _term_off_len(vm, rn)

    return rn, new_src_off, new_src_len


def _int_tree_to_ptree_kids(vm, alts, tok_os, rn_os, n, src_file, src_off):
    kids = []
    new_src_off = cur_src_off = src_off
    new_src_len = 0
    i = 0
    for c in n.flattened:
        if isinstance(c, Tree_Non_Term):
            c, newer_src_off, newer_src_len = \
              _int_tree_to_ptree(vm, alts, tok_os, rn_os, c, src_file, cur_src_off)
            kids.append(c)
        else:
            assert isinstance(c, Tree_Term)
            tok_o = tok_os[c.j + 1]
            newer_src_off, newer_src_len = _term_off_len(vm, tok_o)
            src_end = new_src_off + new_src_len
            kids.append(tok_o)

        if i == 0:
            new_src_off = newer_src_off
        i += 1
        cur_src_off = newer_src_off + newer_src_len
        new_src_len = (newer_src_off - new_src_off) + newer_src_len

    return kids, new_src_off, new_src_len


def _term_off_len(vm, n):
    src_infos = n.get_slot(vm, "src_infos")
    first_src_info = vm.get_slot_apply(src_infos, "get", [Con_Int(vm, 0)])
    src_off = type_check_int(vm, vm.get_slot_apply(first_src_info, "get", [Con_Int(vm, 1)])).v
    src_len = type_check_int(vm, vm.get_slot_apply(first_src_info, "get", [Con_Int(vm, 2)])).v
    return src_off, src_len


def _max_depth(n):
    if isinstance(n, Tree_Term):
        return 0
    assert isinstance(n, Tree_Non_Term)
    md = 0
    for c in n.flattened:
        md = max(md, 1 + _max_depth(c))
    return md


def _flatten_kids(kids):
    fkids = []
    for c in kids:
        if isinstance(c, Tree_Non_Term):
            _flatten_non_term(c)
            if c.complete:
                fkids.append(c)
            elif len(c.flattened) > 0:
                fkids.extend(c.flattened)
        else:
            assert isinstance(c, Tree_Term)
            fkids.append(c)
    return fkids


def _flatten_non_term(n):
    if n.flattened is not None:
        return
    if n.families is None:
        n.flattened = []
        return
    fkids = []
    for c in n.families[0]:
        if isinstance(c, Tree_Non_Term):
            _flatten_non_term(c)
            if c.complete:
                fkids.append(c)
            elif len(c.flattened) > 0:
                fkids.extend(c.flattened)
        else:
            assert isinstance(c, Tree_Term)
            fkids.append(c)
    n.flattened = fkids


def bootstrap_parser_class(vm, mod):
    parser_class = Con_Class(vm, Con_String(vm, "Parser"), \
      [vm.get_builtin(BUILTIN_OBJECT_CLASS)], mod)
    mod.set_defn(vm, "Parser", parser_class)

    new_c_con_func_for_class(vm, "parse", Parser_parse, parser_class)
