// Copyright (c) 2003-2006 King's College London, created by Laurence Tratt
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.


#include "Config.h"

#include <string.h>

#include "Core.h"
#include "Misc.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *Con_Module_C_Earley_Parser_init(Con_Obj *thread, Con_Obj *);
Con_Obj *Con_Module_C_Earley_Parser_import(Con_Obj *thread, Con_Obj *);

Con_Obj *_Con_Module_C_Earley_Parser_Parser_parse_func(Con_Obj *);



Con_Obj *Con_Module_C_Earley_Parser_init(Con_Obj *thread, Con_Obj *identifier)
{
	const char* defn_names[] = {"Parser", "parse", NULL};

	return Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("C_Earley_Parser"), defn_names, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



Con_Obj *Con_Module_C_Earley_Parser_import(Con_Obj *thread, Con_Obj *c_earley_parser_mod)
{
	// Array.Array
	
	Con_Obj *parser_class = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Parser"), Con_Builtins_List_Atom_new_va(thread, CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL), c_earley_parser_mod);
	CON_SET_MOD_DEFN(c_earley_parser_mod, "Parser", parser_class);
	
	CON_SET_FIELD(parser_class, "parse", CON_NEW_BOUND_C_FUNC(_Con_Module_C_Earley_Parser_Parser_parse_func, "parse", c_earley_parser_mod, parser_class));

	return c_earley_parser_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// class Parser
//

//
// 'parse(grammar, tokens)'.
//

//
// Generic stuff.
//

typedef struct {
	Con_Int type;
	Con_Obj *token;
} parse_func_Token;

//
// Recogniser stuff.
//

// A single state.

typedef struct {
	Con_Int p, j, f;
} parse_func_State;

// A sequence of states.

typedef struct {
	Con_Int num_states, num_states_allocated;
	parse_func_State *states;
} parse_func_LState;

// The overall state of the parser (ideally this would have been defined earlier in the file,
// but then we would have forward references).

typedef struct {
	Con_Obj *self;
	parse_func_Token *tokens;
	Con_Int num_tokens;
	Con_Int *grammar;
	Con_Int num_productions, num_rules;
	Con_Int *productions;
	Con_Int *alternatives_maps;
	parse_func_LState *lstates;
	Con_Int num_lstates;
	Con_Int num_rule_names;
	Con_Obj **rule_names;
} parse_func_Parser_State;

// Parser.

typedef enum {PARSE_TREE_TOKEN, PARSE_TREE_TREE} parse_func_parse_Tree_Entry_Type;

typedef struct parse_func_parse_Tree parse_func_parse_Tree;

typedef struct {
	parse_func_parse_Tree_Entry_Type type;
	union {
		Con_Obj *token;
		parse_func_parse_Tree *tree;
	} entry;
} parse_func_parse_Tree_Entry;

// A parse tree (constructed from rule 'p'). We don't need to record 'j' or 'f' here.

struct parse_func_parse_Tree {
	Con_Int p;
	Con_Int num_entries, num_entries_allocated;
	parse_func_parse_Tree_Entry *entries;
};

// An alternative, which is a parse tree generated from 'p', 'j', and 'f'.

typedef struct {
	Con_Int lstate, p, j, f;
	parse_func_parse_Tree *tree;
} parse_func_parse_Tree_Alternative;

// A sequence of alternatives.

typedef struct {
	parse_func_parse_Tree_Alternative *entries;
	Con_Int num_entries, num_entries_allocated;
} parse_func_parse_Tree_Alternatives;

// A state which is currently being processed (this stops infinite loops when we are already
// part of the way through processing <p, j, f> in 'lstate' - see later).

typedef struct {
	Con_Int lstate, p, j, f;
} parse_func_State_In_Processing;

// A stack of states currently being processed.

typedef struct {
	Con_Int num_states, num_states_allocated;
	parse_func_State_In_Processing *states;
} parse_func_States_In_Processing;

// An entry which is either a single alternative or a sequence of alternatives.

typedef struct {
	bool is_single;
	union {
		parse_func_parse_Tree_Alternative alternative;
		parse_func_parse_Tree_Alternatives alternatives;
	} entry;
} parse_func_Alternative_Or_Alternatives;

#define _COMPILED_OFFSET_TO_PRODUCTION 0
#define _COMPILED_OFFSET_TO_ALTERNATIVES_MAP 1
#define _COMPILED_OFFSET_TO_RECOGNISER_BRACKETS_MAPS 2
#define _COMPILED_OFFSET_TO_PARSER_BRACKETS_MAPS 3

#define _COMPILED_PRODUCTIONS_NUM 0
#define _COMPILED_PRODUCTIONS_OFFSETS 1

#define _COMPILED_PRODUCTION_PRECEDENCE 0
#define _COMPILED_PRODUCTION_PARENT_RULE 1
#define _COMPILED_PRODUCTION_NUM_SYMBOLS 2
#define _COMPILED_PRODUCTION_SYMBOLS 3

#define _SYMBOL_RULE_REF 0
#define _SYMBOL_TOKEN 1
#define _SYMBOL_OPEN_KLEENE_STAR_GROUP 2
#define _SYMBOL_CLOSE_KLEENE_STAR_GROUP 3
#define _SYMBOL_OPEN_OPTIONAL_GROUP 4
#define _SYMBOL_CLOSE_OPTIONAL_GROUP 5

#define _COMPILED_ALTERNATIVES_MAP_NUM 0
#define _COMPILED_ALTERNATIVES_MAP_OFFSETS 1

#define _ALTERNATIVE_MAP_IS_NULLABLE 0
#define _ALTERNATIVE_MAP_NUM_ENTRIES 1
#define _ALTERNATIVE_MAP_ENTRIES 2

#define _COMPILED_BRACKETS_MAPS_NUM_ENTRIES 0
#define _COMPILED_BRACKETS_MAPS_ENTRIES 1

#define _BRACKET_MAP_NUM_ENTRIES 0
#define _BRACKET_MAP_ENTRIES 1

#define _DEFAULT_NUM_STATES 112
#define _DEFAULT_NUM_STATES_IN_PROCESSING 64
#define _DEFAULT_NUM_STACK_ALTERNATIVES 4
#define _DEFAULT_NUM_MALLOC_ALTERNATIVES 8

#define _GET_PRODUCTION_INT(p, i) (((Con_Int *) ((u_char *) parser->grammar + parser->productions[p]))[i])

void _Con_Module_C_Earley_Parser_Parser_parse_func_recognise(Con_Obj *, parse_func_Parser_State *);
void _Con_Module_C_Earley_Parser_Parser_parse_func_new_lstate(Con_Obj *, parse_func_Parser_State *);
void _Con_Module_C_Earley_Parser_Parser_parse_func_add_state(Con_Obj *, parse_func_Parser_State *, Con_Int, Con_Int, Con_Int, Con_Int);
Con_Int _Con_Module_C_Earley_Parser_Parser_parse_func_find_state(Con_Obj *, parse_func_Parser_State *, Con_Int, Con_Int, Con_Int, Con_Int);
parse_func_Alternative_Or_Alternatives _Con_Module_C_Earley_Parser_Parser_parse_func_states_to_tree(Con_Obj *, parse_func_Parser_State *, Con_Int, Con_Int, Con_Int, Con_Int, parse_func_States_In_Processing *);
parse_func_parse_Tree *_Con_Module_C_Earley_Parser_Parser_parse_func_blank_tree(Con_Obj *, parse_func_Parser_State *, Con_Int);
parse_func_parse_Tree *_Con_Module_C_Earley_Parser_Parser_parse_func_scopy_tree(Con_Obj *, parse_func_Parser_State *, parse_func_parse_Tree *);
void _Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(Con_Obj *, parse_func_Parser_State *, parse_func_parse_Tree *, parse_func_parse_Tree_Entry *);
parse_func_parse_Tree_Alternatives *_Con_Module_C_Earley_Parser_Parser_parse_func_empty_alternatives(Con_Obj *);
void _Con_Module_C_Earley_Parser_Parser_parse_func_add_alternative(Con_Obj *, parse_func_Parser_State *, parse_func_parse_Tree_Alternatives **, parse_func_parse_Tree_Alternatives **, parse_func_parse_Tree_Alternatives **, Con_Int, Con_Int, Con_Int, Con_Int, parse_func_parse_Tree *);
void _Con_Module_C_Earley_Parser_Parser_parse_func_extend_alternatives(Con_Obj *, parse_func_Parser_State *, parse_func_parse_Tree_Alternatives *, parse_func_parse_Tree_Alternatives *);
void _Con_Module_C_Earley_Parser_Parser_parse_func_resolve_tree_ambiguities(Con_Obj *, parse_func_Parser_State *, parse_func_parse_Tree_Alternatives *);
Con_Obj *_Con_Module_C_Earley_Parser_Parser_parse_func_tree_to_list(Con_Obj *, parse_func_Parser_State *, parse_func_parse_Tree *);
Con_Int _Con_Module_C_Earley_Parser_Parser_parse_calc_max_depth(Con_Obj *, parse_func_Parser_State *, parse_func_parse_Tree *);
Con_Int _Con_Module_C_Earley_Parser_Parser_parse_calc_precedence(Con_Obj *, parse_func_Parser_State *, parse_func_parse_Tree *, Con_Int);

Con_Obj *_Con_Module_C_Earley_Parser_Parser_parse_func(Con_Obj *thread)
{
	Con_Obj *grammar_obj, *rule_names, *self, *tokens_map, *tokens_obj;
	CON_UNPACK_ARGS("OSOOO", &self, &grammar_obj, &rule_names, &tokens_map, &tokens_obj);
	
	parse_func_Parser_State *parser = Con_Memory_malloc(thread, sizeof(parse_func_Parser_State), CON_MEMORY_CHUNK_CONSERVATIVE);
	parser->self = self;
	
	// First of all, we "unpack" the tokens list, mapping token type names into integers, and putting
	// them all into an array, so that we can operate much faster upon individual tokens.
	
	Con_Int num_tokens_allocated = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(tokens_obj, "len"));
	parse_func_Token *tokens = Con_Memory_malloc(thread, num_tokens_allocated * sizeof(parse_func_Token), CON_MEMORY_CHUNK_CONSERVATIVE);

	CON_PRE_GET_SLOT_APPLY_PUMP(tokens_obj, "iterate");
	Con_Int i = 0;
	while (1) {
		Con_Obj *token = CON_APPLY_PUMP();
		if (token == NULL)
			break;
		
		if (i > num_tokens_allocated)
			CON_XXX;
		tokens[i].type = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(tokens_map, "get", CON_GET_SLOT(token, "type")));
		tokens[i].token = token;
		i += 1;
	}
	parser->tokens = tokens;
	parser->num_tokens = num_tokens_allocated;
	
	Con_Builtins_String_Atom *grammar_string_atom = CON_GET_ATOM(grammar_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	// Setup a few things for convenience
	
	parser->grammar = (Con_Int *) grammar_string_atom->str;
	parser->productions = (Con_Int *) ((u_char *) parser->grammar + parser->grammar[_COMPILED_OFFSET_TO_PRODUCTION] + _COMPILED_PRODUCTIONS_OFFSETS * sizeof(Con_Int));
	parser->num_productions = parser->grammar[parser->grammar[_COMPILED_OFFSET_TO_PRODUCTION] / sizeof(Con_Int) + _COMPILED_PRODUCTIONS_NUM];
	parser->alternatives_maps = (Con_Int *) ((u_char *) parser->grammar + parser->grammar[_COMPILED_OFFSET_TO_ALTERNATIVES_MAP] + _COMPILED_ALTERNATIVES_MAP_OFFSETS * sizeof(Con_Int));
	parser->num_rules = parser->grammar[parser->grammar[_COMPILED_OFFSET_TO_ALTERNATIVES_MAP] / sizeof(Con_Int) + _COMPILED_ALTERNATIVES_MAP_NUM];
	
	// Unpack rule names
		
	Con_Int num_rule_names_allocated = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(rule_names, "len"));
	parser->rule_names = Con_Memory_malloc(thread, num_rule_names_allocated * sizeof(Con_Obj *), CON_MEMORY_CHUNK_CONSERVATIVE);

	CON_PRE_GET_SLOT_APPLY_PUMP(rule_names, "iterate");
	i = 0;
	while (1) {
		Con_Obj *rule_name = CON_APPLY_PUMP();
		if (rule_name == NULL)
			break;
			
		if (i > num_rule_names_allocated)
			CON_XXX;
		parser->rule_names[i] = rule_name;
		i += 1;
	}
	parser->num_rule_names = i;
	
	// We know in advance that the number of lstates is a maximum of parser->num_tokens + 1
	// (unsuccessful parses may use less than this number of lstates).
	
	parser->lstates = Con_Memory_malloc(thread, (parser->num_tokens + 1) * sizeof(parse_func_LState), CON_MEMORY_CHUNK_CONSERVATIVE);
	parser->num_lstates = 0;
	_Con_Module_C_Earley_Parser_Parser_parse_func_new_lstate(thread, parser);
	
	// Initialize the recogniser with the state "D0 -> . R"
	
	_Con_Module_C_Earley_Parser_Parser_parse_func_add_state(thread, parser, 0, 0, 0, 0);

	_Con_Module_C_Earley_Parser_Parser_parse_func_recognise(thread, parser);
	
	// The final state of the recogniser should contain "D0 -> R .".
	
	Con_Int final_state_pos;
	for (final_state_pos = 0; final_state_pos < parser->lstates[parser->num_lstates - 1].num_states; final_state_pos += 1) {
		parse_func_State *state = &parser->lstates[parser->num_lstates - 1].states[final_state_pos];
		if (state->p == 0 && state->j == 2 && state->f == 0) {
			break;
		}
	}
	if (final_state_pos == parser->lstates[parser->num_lstates - 1].num_states) {
		CON_GET_SLOT_APPLY(self, "error", parser->tokens[parser->num_lstates - 2].token);
	}
	
	// Now construct a parse tree from the states the recogniser has created.
	
	parse_func_States_In_Processing states_in_processing;
	states_in_processing.num_states_allocated = _DEFAULT_NUM_STATES_IN_PROCESSING;
	states_in_processing.states = Con_Memory_malloc(thread, sizeof(parse_func_State_In_Processing) * states_in_processing.num_states_allocated, CON_MEMORY_CHUNK_OPAQUE);
	states_in_processing.num_states = 0;
	parse_func_Alternative_Or_Alternatives alternatives = _Con_Module_C_Earley_Parser_Parser_parse_func_states_to_tree(thread, parser, parser->num_tokens, 0, 2, 0, &states_in_processing);
	assert(alternatives.is_single == 1);
	assert(alternatives.entry.alternative.tree->entries[0].entry.tree != NULL);
	
	// Turn the parse tree into a nested list (removing the D0 node at the very top).
	
	return _Con_Module_C_Earley_Parser_Parser_parse_func_tree_to_list(thread, parser, alternatives.entry.alternative.tree->entries[0].entry.tree);
}



void _Con_Module_C_Earley_Parser_Parser_parse_func_new_lstate(Con_Obj *thread, parse_func_Parser_State *parser)
{
	assert(parser->num_lstates < parser->num_tokens + 1);
	parser->lstates[parser->num_lstates].states = Con_Memory_malloc(thread, _DEFAULT_NUM_STATES * sizeof(parse_func_State), CON_MEMORY_CHUNK_OPAQUE);
	parser->lstates[parser->num_lstates].num_states = 0;
	parser->lstates[parser->num_lstates].num_states_allocated = _DEFAULT_NUM_STATES;
	parser->num_lstates += 1;
}



//
// The recogniser.
//

void _Con_Module_C_Earley_Parser_Parser_parse_func_recognise(Con_Obj *thread, parse_func_Parser_State *parser)
{
	// This is, intentionally, a simple Earley recogniser.
	
	// We only make one small optimisation, which is that each rule is only predicted once in each
	// state. We pack the relevant information this into a bit field. This can give a useful saving
	// if the state tables grow big, as it stops a lot of searching to see whether states have been
	// added.

	unsigned int predicted[parser->num_rules / (sizeof(unsigned int) * 8) + 1];

	for (Con_Int x = 0; x < parser->num_tokens + 1; x += 1) {
		if (x < parser->num_tokens)
			_Con_Module_C_Earley_Parser_Parser_parse_func_new_lstate(thread, parser);
		
		// Reset the list of which rules have been predicted.
		
		bzero(&predicted, (parser->num_rules / (sizeof(unsigned int) * 8) + 1) * sizeof(unsigned int));
		
		Con_Int y = 0;
		while (y < parser->lstates[x].num_states) {
			parse_func_State state = parser->lstates[x].states[y];
			
			Con_Int *production = (Con_Int *) ((u_char *) parser->grammar + parser->productions[state.p]);
			
			if (x < parser->num_tokens && state.j < production[_COMPILED_PRODUCTION_NUM_SYMBOLS] && production[_COMPILED_PRODUCTION_SYMBOLS + state.j] == _SYMBOL_TOKEN && production[_COMPILED_PRODUCTION_SYMBOLS + state.j + 1] == parser->tokens[x].type) {
				// Scanner
				
				_Con_Module_C_Earley_Parser_Parser_parse_func_add_state(thread, parser, x + 1, state.p, state.j + 2, state.f);
			}
			else if (state.j < production[_COMPILED_PRODUCTION_NUM_SYMBOLS] && production[_COMPILED_PRODUCTION_SYMBOLS + state.j] == _SYMBOL_RULE_REF) {
				// Predictor

				Con_Int *alternative_map = (Con_Int *) ((u_char *) parser->grammar + parser->alternatives_maps[production[_COMPILED_PRODUCTION_SYMBOLS + state.j + 1]]);
				
				// We only predict a rule once in each state.
				Con_Int predicted_rule_num = production[_COMPILED_PRODUCTION_SYMBOLS + state.j + 1];
				if ((predicted[predicted_rule_num / (sizeof(unsigned int) * 8)] & (1 << (predicted_rule_num % (sizeof(unsigned int) * 8)))) == 0) {
					predicted[predicted_rule_num / (sizeof(unsigned int) * 8)] |= 1 << (predicted_rule_num % (sizeof(unsigned int) * 8));

					for (Con_Int i = 0; i < alternative_map[_ALTERNATIVE_MAP_NUM_ENTRIES]; i += 1)
						_Con_Module_C_Earley_Parser_Parser_parse_func_add_state(thread, parser, x, alternative_map[_ALTERNATIVE_MAP_ENTRIES + i], 0, x);
				}
				
				// Regardless of whether we've had to predict the rule, we still need to add a state
				// if the rule is nullable (since this state is dependent on state.j).
				
				if (alternative_map[_ALTERNATIVE_MAP_IS_NULLABLE] == 1)
					_Con_Module_C_Earley_Parser_Parser_parse_func_add_state(thread, parser, x, state.p, state.j + 2, state.f);
			}
			else if (state.j == production[_COMPILED_PRODUCTION_NUM_SYMBOLS]) {
				// Completer
				
				assert(state.f < parser->num_lstates);
				for (Con_Int z = 0; z < parser->lstates[state.f].num_states; z += 1) {
					parse_func_State f_state = parser->lstates[state.f].states[z];
					Con_Int *f_production = (Con_Int *) ((u_char *) parser->grammar + parser->productions[f_state.p]);
					if (f_state.j < f_production[_COMPILED_PRODUCTION_NUM_SYMBOLS] && f_production[_COMPILED_PRODUCTION_SYMBOLS + f_state.j] == _SYMBOL_RULE_REF && f_production[_COMPILED_PRODUCTION_SYMBOLS + f_state.j + 1] == production[_COMPILED_PRODUCTION_PARENT_RULE]) {
						_Con_Module_C_Earley_Parser_Parser_parse_func_add_state(thread, parser, x, f_state.p, f_state.j + 2, f_state.f);
					}
				}
			}
			y += 1;
		}
				
		if (x < parser->num_tokens && parser->lstates[x + 1].num_states == 0)
			CON_GET_SLOT_APPLY(parser->self, "error", parser->tokens[x].token);
	}
}



//
// Add state <p, j, f> to 'lstate'. If this state already exists in 'lstate', it will not be
// duplicated.
//

void _Con_Module_C_Earley_Parser_Parser_parse_func_add_state(Con_Obj *thread, parse_func_Parser_State *parser, Con_Int lstate, Con_Int p, Con_Int j, Con_Int f)
{
	parse_func_LState *lstate_store = &parser->lstates[lstate];
	
	if (j < _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_NUM_SYMBOLS) && (_GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j) == _SYMBOL_OPEN_KLEENE_STAR_GROUP ||
		 _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j) == _SYMBOL_CLOSE_KLEENE_STAR_GROUP ||
		 _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j) == _SYMBOL_OPEN_OPTIONAL_GROUP ||
		 _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j) == _SYMBOL_CLOSE_OPTIONAL_GROUP)) {
		 
		Con_Int *brackets_map = &parser->grammar[parser->grammar[parser->grammar[_COMPILED_OFFSET_TO_RECOGNISER_BRACKETS_MAPS] / sizeof(Con_Int) + _COMPILED_BRACKETS_MAPS_ENTRIES + _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j + 1)] / sizeof(Con_Int)];
		
		for (Con_Int x = 0; x < brackets_map[_BRACKET_MAP_NUM_ENTRIES]; x += 1) {
			Con_Int new_j = brackets_map[_BRACKET_MAP_ENTRIES + x];
			if (_Con_Module_C_Earley_Parser_Parser_parse_func_find_state(thread, parser, lstate, p, new_j, f) == -1) {
				Con_Memory_make_array_room(thread, (void **) &lstate_store->states, NULL, &lstate_store->num_states_allocated, &lstate_store->num_states, 1, sizeof(parse_func_State));
	
				lstate_store->states[lstate_store->num_states].p = p;
				lstate_store->states[lstate_store->num_states].j = new_j;
				lstate_store->states[lstate_store->num_states].f = f;
				lstate_store->num_states += 1;
			}
		}
	}
	else {
		if (_Con_Module_C_Earley_Parser_Parser_parse_func_find_state(thread, parser, lstate, p, j, f) != -1)
			return;

		Con_Memory_make_array_room(thread, (void **) &lstate_store->states, NULL, &lstate_store->num_states_allocated, &lstate_store->num_states, 1, sizeof(parse_func_State));

		lstate_store->states[lstate_store->num_states].p = p;
		lstate_store->states[lstate_store->num_states].j = j;
		lstate_store->states[lstate_store->num_states].f = f;
		lstate_store->num_states += 1;
	}
}



//
// Return the position that the state <p, j, f> is found in 'lstate' or -1 if the state is not found.
//

Con_Int _Con_Module_C_Earley_Parser_Parser_parse_func_find_state(Con_Obj *thread, parse_func_Parser_State *parser, Con_Int lstate, Con_Int p, Con_Int j, Con_Int f)
{
	parse_func_LState *lstate_store = &parser->lstates[lstate];

	for (Con_Int i = 0; i < lstate_store->num_states; i += 1) {
		parse_func_State *candidate_state = &lstate_store->states[i];
		if (candidate_state->p == p && candidate_state->j == j && candidate_state->f == f)
			return i;
	}
	
	return -1;
}



//
// Create a parse tree for the state <start_p, start_j, start_f> in 'start_lstate'. Returns one or
// more alternative parse trees.
//
// 'states_in_processing' should be a pointer to memory with a list of states that are currently
// being processed; this is to stop this function going into an infinite loop.
//
// Note that entries in each node of the tree are in reverse order. This is because we have to build
// the parse tree from the bottom up; storing the entries in reverse order prevents us having to
// continually do the equivalent of 'insert(0, ...)'.
//

parse_func_Alternative_Or_Alternatives _Con_Module_C_Earley_Parser_Parser_parse_func_states_to_tree(Con_Obj *thread, parse_func_Parser_State *parser, Con_Int start_lstate, Con_Int start_p, Con_Int start_j, Con_Int start_f, parse_func_States_In_Processing *states_in_processing)
{
	// This function, and many of its helpers, have been optimised to not allocate memory whenever
	// possible. The main cost of executing this function is its tendency to allocate large chunks
	// of memory for alternatives that are either never created, or which are removed as
	// possibilities almost as soon as they are created.
	//
	// To this end, we try and do as much as possible in stack space before falling back on malloc'd
	// memory if necessary.
	//
	// This does have the unfortunate consequence of making the following code harder to read.

	// There are three important variables to do with the current set of alternatives:
	//   stack_alternatives : pointer to chunk of memory on the stack
	//   malloc_alternatives: pointer to chunk of memory on the heap (or NULL if not yet allocated)
	//   alternatives       : points to either stack_alternatives or malloc_alternatives depending
	//                        on which is currently in use.

	parse_func_parse_Tree_Alternatives stack_alternatives_mem;
	parse_func_parse_Tree_Alternative stack_alternatives_alternative[_DEFAULT_NUM_STACK_ALTERNATIVES];
	stack_alternatives_mem.entries = stack_alternatives_alternative;
	stack_alternatives_mem.num_entries = 0;
	stack_alternatives_mem.num_entries_allocated = _DEFAULT_NUM_STACK_ALTERNATIVES;
	
	parse_func_parse_Tree_Alternatives *stack_alternatives = &stack_alternatives_mem;
	parse_func_parse_Tree_Alternatives *malloc_alternatives = NULL;
	parse_func_parse_Tree_Alternatives *alternatives = stack_alternatives;

	// XXX: This variable should be local to several parts of below code. Unfortunately doing so
	// appears to trigger a bug in GCC 3.3.5 on x86_64; using this variable here, once, appears to
	// fix the problem and whilst ugly is safe. One day maybe it can move back to those places where
	// it came from.

	parse_func_parse_Tree_Entry new_entry;

	if (start_j > 0 && (_GET_PRODUCTION_INT(start_p, _COMPILED_PRODUCTION_SYMBOLS + start_j - 2) == _SYMBOL_CLOSE_KLEENE_STAR_GROUP || _GET_PRODUCTION_INT(start_p, _COMPILED_PRODUCTION_SYMBOLS + start_j - 2) == _SYMBOL_CLOSE_OPTIONAL_GROUP)) {
		// If start_j points to a close grouping (by definition 'start_j' can't point to an open
		// grouping) then we have to immediately start with multiple alternatives.
		
		Con_Int *brackets_map = &parser->grammar[parser->grammar[parser->grammar[_COMPILED_OFFSET_TO_PARSER_BRACKETS_MAPS] / sizeof(Con_Int) + _COMPILED_BRACKETS_MAPS_ENTRIES + _GET_PRODUCTION_INT(start_p, _COMPILED_PRODUCTION_SYMBOLS + start_j - 2 + 1)] / sizeof(Con_Int)];
		for (Con_Int x = 0; x < brackets_map[_BRACKET_MAP_NUM_ENTRIES]; x += 1) {
			Con_Int new_j = brackets_map[_BRACKET_MAP_ENTRIES + x];
			if (new_j < 0 && start_lstate > start_f)
				continue;
			_Con_Module_C_Earley_Parser_Parser_parse_func_add_alternative(thread, parser, &alternatives, &stack_alternatives, &malloc_alternatives, start_lstate, start_p, new_j + 2, start_f, _Con_Module_C_Earley_Parser_Parser_parse_func_blank_tree(thread, parser, start_p));
		}
	}
	else {
		_Con_Module_C_Earley_Parser_Parser_parse_func_add_alternative(thread, parser, &alternatives, &stack_alternatives, &malloc_alternatives, start_lstate, start_p, start_j, start_f, _Con_Module_C_Earley_Parser_Parser_parse_func_blank_tree(thread, parser, start_p));
	}
	
	Con_Int x = 0;
	while (x < alternatives->num_entries) {
		Con_Int lstate = alternatives->entries[x].lstate;
		Con_Int p = alternatives->entries[x].p;
		Con_Int j = alternatives->entries[x].j;
		Con_Int f = alternatives->entries[x].f;
		parse_func_parse_Tree *tree = alternatives->entries[x].tree;
		
		j -= 2;
		while (j >= 0) {
			assert(lstate >= f && f < parser->num_lstates);
			assert(lstate >= 0 && lstate < parser->num_lstates);
			if (_GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j) == _SYMBOL_RULE_REF) {
				// This part of the parser is complicated by the fact that we're trying desparately
				// to optimise for the common case i.e. when there are no ambiguities. The main cost
				// in catering for ambiguities is the needless allocation of memory, so we try
				// not to malloc memory for multiple alternatives unless we find we really have to.
			
				bool found_possible_alternative = false;
				parse_func_parse_Tree_Alternatives *possible_alternatives = NULL;
				parse_func_parse_Tree_Alternative possible_alternative; 
				for (Con_Int y = 0; y < parser->lstates[lstate].num_states; y += 1) {
					Con_Int candidate_p = parser->lstates[lstate].states[y].p;
					Con_Int candidate_j = parser->lstates[lstate].states[y].j;
					Con_Int candidate_f = parser->lstates[lstate].states[y].f;
						
					// A couple of quick checks allow us to rule many candidate states out
					// straight away.
					//
					// It would be possible to narrow this down a bit further if we knew the
					// minimum number of symbols that have to appear to the left of this one.
					
					if (candidate_f < f || (j == 0 && candidate_f != f))
						continue;

					// Now we need to check to see if we're already part of the way through
					// processing this state - if we are, we don't start processing it again,
					// to avoid infinite loops.
					
					Con_Int z;
					for (z = 0; z < states_in_processing->num_states; z += 1) {
						if (states_in_processing->states[z].lstate == lstate &&
							states_in_processing->states[z].p == candidate_p &&
							states_in_processing->states[z].j == candidate_j &&
							states_in_processing->states[z].f == candidate_f)
							break;
					}
					if (z < states_in_processing->num_states) {
						// This state is already in processing.
						continue;
					}
					
					if (_GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j + 1) == _GET_PRODUCTION_INT(candidate_p, _COMPILED_PRODUCTION_PARENT_RULE) &&
						candidate_j == _GET_PRODUCTION_INT(candidate_p, _COMPILED_PRODUCTION_NUM_SYMBOLS) &&
						_Con_Module_C_Earley_Parser_Parser_parse_func_find_state(thread, parser, candidate_f, p, j, f) != -1) {
						if (!found_possible_alternative) {
							// This is the first possible alternative we've found so far.
							
							possible_alternative.lstate = lstate;
							possible_alternative.p = candidate_p;
							possible_alternative.j = candidate_j;
							possible_alternative.f = candidate_f;
							found_possible_alternative = true;
						}
						else {
							// We've already found at least one possible alternative.
						
							if (possible_alternatives == NULL) {
								// So far we've only found one possible alternative, so we need to
								// malloc memory for multiple possilble alternatives and push the one
								// we've found so far onto this new stack. From hereon we'll continue
								// to use this malloc'd memory.
								
								possible_alternatives = _Con_Module_C_Earley_Parser_Parser_parse_func_empty_alternatives(thread);
								_Con_Module_C_Earley_Parser_Parser_parse_func_add_alternative(thread, parser, NULL, NULL, &possible_alternatives, possible_alternative.lstate, possible_alternative.p, possible_alternative.j, possible_alternative.f, NULL);
							}
							_Con_Module_C_Earley_Parser_Parser_parse_func_add_alternative(thread, parser, NULL, NULL, &possible_alternatives, lstate, candidate_p, candidate_j, candidate_f, NULL);
						}
					}
				}
				
				if (!found_possible_alternative)
					goto remove_alternative;
				
				Con_Memory_make_array_room(thread, (void **) &states_in_processing->states, NULL, &states_in_processing->num_states_allocated, &states_in_processing->num_states, 1, sizeof(parse_func_State_In_Processing));
				states_in_processing->states[states_in_processing->num_states].lstate = start_lstate;
				states_in_processing->states[states_in_processing->num_states].p = start_p;
				states_in_processing->states[states_in_processing->num_states].j = start_j;
				states_in_processing->states[states_in_processing->num_states].f = start_f;
				states_in_processing->num_states += 1;
				
				parse_func_parse_Tree_Alternatives *definite_alternatives = NULL;
				parse_func_parse_Tree_Alternative definite_alternative;
				if (possible_alternatives == NULL) {
					// We're only dealing with a single alternative at this point - note however
					// that it is possible for this single alternative to generate multiple
					// alternatives itself.
					
					parse_func_Alternative_Or_Alternatives new_alternatives = _Con_Module_C_Earley_Parser_Parser_parse_func_states_to_tree(thread, parser, lstate, possible_alternative.p, possible_alternative.j, possible_alternative.f, states_in_processing);
					states_in_processing->num_states -= 1;
					
					if (new_alternatives.is_single) {
						// We only got back a single entry - easy stuff.
						
						definite_alternative = new_alternatives.entry.alternative;
					}
					else if (new_alternatives.entry.alternatives.num_entries == 0)
						goto remove_alternative;
					else {
						// We got back multiple alternatives. We straight away see if we can resolve
						// them down to a single alternative.
						
						parse_func_parse_Tree_Alternatives *real_new_alternatives = &new_alternatives.entry.alternatives;
						_Con_Module_C_Earley_Parser_Parser_parse_func_resolve_tree_ambiguities(thread, parser, real_new_alternatives);

						if (real_new_alternatives->num_entries == 1) {
							// Thankfully we resolved the alternatives down to a single entry.
							definite_alternative = real_new_alternatives->entries[0];
						}
						else
							definite_alternatives = real_new_alternatives;
					}
				}
				else {
					// The slow case: we have multiple alternatives that we haven't yet been able to
					// resolve, so we have to build trees for each one.
				
					definite_alternatives = _Con_Module_C_Earley_Parser_Parser_parse_func_empty_alternatives(thread);
					for (Con_Int y = 0; y < possible_alternatives->num_entries; y += 1) {
						parse_func_Alternative_Or_Alternatives new_alternatives = _Con_Module_C_Earley_Parser_Parser_parse_func_states_to_tree(thread, parser, lstate, possible_alternatives->entries[y].p, possible_alternatives->entries[y].j, possible_alternatives->entries[y].f, states_in_processing);
						if (new_alternatives.is_single) {
							parse_func_parse_Tree_Alternative *real_new_alternative = &new_alternatives.entry.alternative;
							_Con_Module_C_Earley_Parser_Parser_parse_func_add_alternative(thread, parser, NULL, NULL, &definite_alternatives, real_new_alternative->lstate, real_new_alternative->p, real_new_alternative->j, real_new_alternative->f, real_new_alternative->tree);
						}
						else
							_Con_Module_C_Earley_Parser_Parser_parse_func_extend_alternatives(thread, parser, definite_alternatives, &new_alternatives.entry.alternatives);
					}
					states_in_processing->num_states -= 1;

					if (definite_alternatives->num_entries == 0)
						goto remove_alternative;
					else if (definite_alternatives->num_entries == 1) {
						definite_alternative = definite_alternatives->entries[0];
						definite_alternatives = NULL;
					}
					else {
						_Con_Module_C_Earley_Parser_Parser_parse_func_resolve_tree_ambiguities(thread, parser, definite_alternatives);
						if (definite_alternatives->num_entries == 1) {
							definite_alternative = definite_alternatives->entries[0];
							definite_alternatives = NULL;
						}
					}
				}
				
				if (definite_alternatives == NULL) {
					// We're in the relatively simple case where we either only had one possible
					// alternative, or we were able to resolve a conflict down to a single
					// alternative.
					
					if (j == 0 || _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j - 2) == _SYMBOL_RULE_REF || _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j - 2) == _SYMBOL_TOKEN) {
						// We're in the simple case where we're processing one of these types of
						// states:
						//
						//   A -> B . y
						//   A -> x B . y
						//
						// where B is either a terminal or non-terminal.
						
						lstate = definite_alternative.lstate;
						new_entry.type = PARSE_TREE_TREE;
						new_entry.entry.tree = definite_alternative.tree;
						_Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(thread, parser, tree, &new_entry);
						
						alternatives->entries[x].lstate = lstate;
						alternatives->entries[x].j = j;
						j -= 2;
					}
					else {
						lstate = definite_alternative.lstate;
						
						Con_Int *brackets_map = &parser->grammar[parser->grammar[parser->grammar[_COMPILED_OFFSET_TO_PARSER_BRACKETS_MAPS] / sizeof(Con_Int) + _COMPILED_BRACKETS_MAPS_ENTRIES + _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j - 2 + 1)] / sizeof(Con_Int)];
						for (Con_Int y = 1; y < brackets_map[_BRACKET_MAP_NUM_ENTRIES]; y += 1) {
							parse_func_parse_Tree *new_tree = _Con_Module_C_Earley_Parser_Parser_parse_func_scopy_tree(thread, parser, tree);
							new_entry.type = PARSE_TREE_TREE;
							new_entry.entry.tree = definite_alternative.tree;
							_Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(thread, parser, new_tree, &new_entry);
							_Con_Module_C_Earley_Parser_Parser_parse_func_add_alternative(thread, parser, &alternatives, &stack_alternatives, &malloc_alternatives, lstate, p, brackets_map[_BRACKET_MAP_ENTRIES + y] + 2, f, new_tree);
						}
						
						new_entry.type = PARSE_TREE_TREE;
						new_entry.entry.tree = definite_alternative.tree;
						_Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(thread, parser, tree, &new_entry);
						
						alternatives->entries[x].lstate = lstate;
						alternatives->entries[x].j = brackets_map[_BRACKET_MAP_ENTRIES + 0] + 2;
						j = brackets_map[_BRACKET_MAP_ENTRIES + 0];
					}
				}
				else {
					// Bugger; we weren't able to choose one of the alternatives because
					// they're not finished processing yet. Therefore we have to push all of them
					// onto the stack.
					
					if (j == 0 || _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j - 2) == _SYMBOL_RULE_REF || _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j - 2) == _SYMBOL_TOKEN) {
						// We're in the simple case where we're processing one of these types of
						// states:
						//
						//   A -> B . y
						//   A -> x B . y
						//
						// where B is either a terminal or non-terminal.
						//
						// However we do have more than one alternative to push onto the stack.
						
						for (Con_Int y = 1; y < definite_alternatives->num_entries; y += 1) {
							parse_func_parse_Tree *new_tree = _Con_Module_C_Earley_Parser_Parser_parse_func_scopy_tree(thread, parser, alternatives->entries[x].tree);
							new_entry.type = PARSE_TREE_TREE;
							new_entry.entry.tree = definite_alternatives->entries[y].tree;
							_Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(thread, parser, new_tree, &new_entry);
							_Con_Module_C_Earley_Parser_Parser_parse_func_add_alternative(thread, parser, &alternatives, &stack_alternatives, &malloc_alternatives, definite_alternatives->entries[y].lstate, p, j, f, new_tree);
						}

						lstate = definite_alternatives->entries[0].lstate;
						new_entry.type = PARSE_TREE_TREE;
						new_entry.entry.tree = definite_alternatives->entries[0].tree;
						_Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(thread, parser, tree, &new_entry);
						
						alternatives->entries[x].lstate = lstate;
						alternatives->entries[x].j = j;
						j -= 2;
					}
					else {
						// We're in the more complex case where the production entry preceeding the
						// token is a grouping. That means that we will have to add multiple
						// alternatives of the multiple alternatives.
						//
						// This isn't quite as bad as it may first seem: we still know (as in the
						// first branch) that we're effectively moving the dot over a terminal or
						// non-terminal.

						Con_Int *brackets_map = &parser->grammar[parser->grammar[parser->grammar[_COMPILED_OFFSET_TO_PARSER_BRACKETS_MAPS] / sizeof(Con_Int) + _COMPILED_BRACKETS_MAPS_ENTRIES + _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j - 2 + 1)] / sizeof(Con_Int)];

						// First of all we take the current working alternative and add all but
						// the first bracket alternatives into the alternatives.

						for (Con_Int y = 1; y < brackets_map[_BRACKET_MAP_NUM_ENTRIES]; y += 1) {
							parse_func_parse_Tree *new_tree = _Con_Module_C_Earley_Parser_Parser_parse_func_scopy_tree(thread, parser, tree);
							new_entry.type = PARSE_TREE_TREE;
							new_entry.entry.tree = definite_alternatives->entries[0].tree;
							_Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(thread, parser, new_tree, &new_entry);
							_Con_Module_C_Earley_Parser_Parser_parse_func_add_alternative(thread, parser, &alternatives, &stack_alternatives, &malloc_alternatives, definite_alternatives->entries[0].lstate, p, brackets_map[_BRACKET_MAP_ENTRIES + y] + 2, f, new_tree);
						}

						// Now we copy all the other alternatives (note that for all of these we
						// copy *all* of the bracket alternatives).

						for (Con_Int y = 1; y < definite_alternatives->num_entries; y += 1) {
							for (Con_Int z = 0; z < brackets_map[_BRACKET_MAP_NUM_ENTRIES]; z += 1) {
								parse_func_parse_Tree *new_tree = _Con_Module_C_Earley_Parser_Parser_parse_func_scopy_tree(thread, parser, tree);
								new_entry.type = PARSE_TREE_TREE;
								new_entry.entry.tree = definite_alternatives->entries[y].tree;
								_Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(thread, parser, new_tree, &new_entry);
								_Con_Module_C_Earley_Parser_Parser_parse_func_add_alternative(thread, parser, &alternatives, &stack_alternatives, &malloc_alternatives, definite_alternatives->entries[y].lstate, p, brackets_map[_BRACKET_MAP_ENTRIES + z] + 2, f, new_tree);
							}
						}

						// Update the current working alternative.

						lstate = definite_alternatives->entries[0].lstate;
						new_entry.type = PARSE_TREE_TREE;
						new_entry.entry.tree = definite_alternatives->entries[0].tree;
						_Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(thread, parser, tree, &new_entry);
						alternatives->entries[x].lstate = lstate;
						alternatives->entries[x].j = brackets_map[_BRACKET_MAP_ENTRIES + 0] + 2;
						j = brackets_map[_BRACKET_MAP_ENTRIES + 0];
					}
				}
			}
			else {
				assert(_GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j) == _SYMBOL_TOKEN);
				if (_GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j + 1) != parser->tokens[lstate - 1].type)
					goto remove_alternative;
				
				if (j == 0 || _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j - 2) == _SYMBOL_RULE_REF || _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j - 2) == _SYMBOL_TOKEN) {
					// We're in the simple case where we're processing one of these types of
					// states:
					//
					//   A -> . y z
					//   A -> x . y z
					//
					// where x is a terminal.
					
					lstate -= 1;
					
					if (_Con_Module_C_Earley_Parser_Parser_parse_func_find_state(thread, parser, lstate, p, j, f) == -1)
						goto remove_alternative;
					
					new_entry.type = PARSE_TREE_TOKEN;
					new_entry.entry.token = parser->tokens[lstate].token;
					_Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(thread, parser, tree, &new_entry);
					alternatives->entries[x].lstate = lstate;
					alternatives->entries[x].j = j;
					j -= 2;
				}
				else {
					// We're in the more complex case where the production entry preceeding the token
					// is a grouping. That means that we will have to add multiple alternatives.

					lstate -= 1;
					
					Con_Int *brackets_map = &parser->grammar[parser->grammar[parser->grammar[_COMPILED_OFFSET_TO_PARSER_BRACKETS_MAPS] / sizeof(Con_Int) + _COMPILED_BRACKETS_MAPS_ENTRIES + _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_SYMBOLS + j - 2 + 1)] / sizeof(Con_Int)];
					for (Con_Int y = 1; y < brackets_map[_BRACKET_MAP_NUM_ENTRIES]; y += 1) {
						parse_func_parse_Tree *new_tree = _Con_Module_C_Earley_Parser_Parser_parse_func_scopy_tree(thread, parser, tree);
						new_entry.type = PARSE_TREE_TOKEN;
						new_entry.entry.token = parser->tokens[lstate].token;
						_Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(thread, parser, new_tree, &new_entry);
						_Con_Module_C_Earley_Parser_Parser_parse_func_add_alternative(thread, parser, &alternatives, &stack_alternatives, &malloc_alternatives, lstate, p, brackets_map[_BRACKET_MAP_ENTRIES + y] + 2, f, new_tree);
					}
					
					new_entry.type = PARSE_TREE_TOKEN;
					new_entry.entry.token = parser->tokens[lstate].token;
					_Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(thread, parser, tree, &new_entry);
					alternatives->entries[x].lstate = lstate;
					alternatives->entries[x].j = brackets_map[_BRACKET_MAP_ENTRIES + 0] + 2;
					j = brackets_map[_BRACKET_MAP_ENTRIES + 0];
				}
			}
		}
		
		if (lstate > start_f)
			goto remove_alternative;
		
		x += 1;
		continue;

remove_alternative:
		// As we've been building the current parse tree, it's clear that it isn't correct, so we
		// remove it from the list of alternatives that we're building.
		
		memmove(alternatives->entries + x, alternatives->entries + x + 1, (alternatives->num_entries - (x + 1)) * sizeof(parse_func_parse_Tree_Alternative));
		alternatives->num_entries -= 1;
		bzero(alternatives->entries + alternatives->num_entries, sizeof(parse_func_parse_Tree_Alternative));
	}

	if (alternatives->num_entries == 1) {
		parse_func_Alternative_Or_Alternatives rtn;
		rtn.is_single = true;
		rtn.entry.alternative = alternatives->entries[0];
		return rtn;
	}
	else {
		if (alternatives == stack_alternatives) {
			malloc_alternatives = _Con_Module_C_Earley_Parser_Parser_parse_func_empty_alternatives(thread);
			_Con_Module_C_Earley_Parser_Parser_parse_func_extend_alternatives(thread, parser, malloc_alternatives, stack_alternatives);
			alternatives = malloc_alternatives;
		}
	
		parse_func_Alternative_Or_Alternatives rtn;
		rtn.is_single = false;
		rtn.entry.alternatives = *alternatives;
		return rtn;
	}
}



//
// Return a parse tree with no entries.
//

parse_func_parse_Tree *_Con_Module_C_Earley_Parser_Parser_parse_func_blank_tree(Con_Obj *thread, parse_func_Parser_State *parser, Con_Int p)
{
	parse_func_parse_Tree *tree = Con_Memory_malloc(thread, sizeof(parse_func_parse_Tree), CON_MEMORY_CHUNK_CONSERVATIVE);
	tree->p = p;
	tree->num_entries_allocated = _GET_PRODUCTION_INT(p, _COMPILED_PRODUCTION_NUM_SYMBOLS);
	tree->entries = Con_Memory_malloc(thread, sizeof(parse_func_parse_Tree_Entry) * tree->num_entries_allocated, CON_MEMORY_CHUNK_CONSERVATIVE);
	tree->num_entries = 0;
	
	return tree;
}



//
// Return a shallow copy of the parse tree 'tree'.
//

parse_func_parse_Tree *_Con_Module_C_Earley_Parser_Parser_parse_func_scopy_tree(Con_Obj *thread, parse_func_Parser_State *parser, parse_func_parse_Tree *tree)
{
	parse_func_parse_Tree *new_tree = Con_Memory_malloc(thread, sizeof(parse_func_parse_Tree), CON_MEMORY_CHUNK_CONSERVATIVE);
	new_tree->p = tree->p;
	new_tree->num_entries_allocated = tree->num_entries_allocated;
	new_tree->entries = Con_Memory_malloc(thread, new_tree->num_entries_allocated * sizeof(parse_func_parse_Tree_Entry), CON_MEMORY_CHUNK_CONSERVATIVE);
	memmove(new_tree->entries, tree->entries, tree->num_entries * sizeof(parse_func_parse_Tree_Entry));
	new_tree->num_entries = tree->num_entries;
	
	return new_tree;
}



//
// Add 'new_entry' to the parse tree 'tree'.
//

void _Con_Module_C_Earley_Parser_Parser_parse_func_add_tree_element(Con_Obj *thread, parse_func_Parser_State *parser, parse_func_parse_Tree *tree, parse_func_parse_Tree_Entry *new_entry)
{
	Con_Memory_make_array_room(thread, (void **) &tree->entries, NULL, &tree->num_entries_allocated, &tree->num_entries, 1, sizeof(parse_func_parse_Tree_Entry));
	tree->entries[tree->num_entries++] = *new_entry;
}



//
// Return a new list of alternatives with no entries.
//

parse_func_parse_Tree_Alternatives *_Con_Module_C_Earley_Parser_Parser_parse_func_empty_alternatives(Con_Obj *thread)
{
	parse_func_parse_Tree_Alternatives *alternatives = Con_Memory_malloc(thread, sizeof(parse_func_parse_Tree_Alternatives), CON_MEMORY_CHUNK_CONSERVATIVE);
	alternatives->num_entries_allocated = _DEFAULT_NUM_MALLOC_ALTERNATIVES;
	alternatives->entries = Con_Memory_malloc(thread, alternatives->num_entries_allocated * sizeof(parse_func_parse_Tree_Alternative), CON_MEMORY_CHUNK_CONSERVATIVE);
	alternatives->num_entries = 0;
	
	return alternatives;
}



//
// Add the alternative '<p, j, f, tree>' to either '*stack_alternatives' or '*malloc_alternatives'.
//
// If 'stack_alternatives' or '*stack_alternatives' is NULL, entries are added straight away to
// '*malloc_alternatives'. Otherwise, this function attempts to add entries to '*stack_alternatives';
// if that is full, it sets up '*malloc_alternatives', makes '*alternatives' == '*stack_alternatives'
// and then adds the new entry to '*malloc_alternatives'.
//

void _Con_Module_C_Earley_Parser_Parser_parse_func_add_alternative(Con_Obj *thread, parse_func_Parser_State *parser, parse_func_parse_Tree_Alternatives **alternatives, parse_func_parse_Tree_Alternatives **stack_alternatives, parse_func_parse_Tree_Alternatives **malloc_alternatives, Con_Int lstate, Con_Int p, Con_Int j, Con_Int f, parse_func_parse_Tree *tree)
{
	if (stack_alternatives != NULL && *stack_alternatives != NULL) {
		if ((*stack_alternatives)->num_entries < (*stack_alternatives)->num_entries_allocated) {
			parse_func_parse_Tree_Alternative *new_alternative = &(*stack_alternatives)->entries[(*stack_alternatives)->num_entries];

			new_alternative->lstate = lstate;
			new_alternative->p = p;
			new_alternative->j = j;
			new_alternative->f = f;
			new_alternative->tree = tree;

			(*stack_alternatives)->num_entries += 1;
			return;
		}
		
		*malloc_alternatives = _Con_Module_C_Earley_Parser_Parser_parse_func_empty_alternatives(thread);
		_Con_Module_C_Earley_Parser_Parser_parse_func_extend_alternatives(thread, parser, *malloc_alternatives, *stack_alternatives);
		*stack_alternatives = NULL;
		*alternatives = *malloc_alternatives;
	}
	
	Con_Memory_make_array_room(thread, (void **) &(*malloc_alternatives)->entries, NULL, &(*malloc_alternatives)->num_entries_allocated, &(*malloc_alternatives)->num_entries, 1, sizeof(parse_func_parse_Tree_Alternative));
	parse_func_parse_Tree_Alternative *new_alternative = &(*malloc_alternatives)->entries[(*malloc_alternatives)->num_entries];
	
	new_alternative->lstate = lstate;
	new_alternative->p = p;
	new_alternative->j = j;
	new_alternative->f = f;
	new_alternative->tree = tree;
	
	(*malloc_alternatives)->num_entries += 1;
}



//
// Append each alternative in 'new_alternatives' in turn to 'dst_alternatives'.
//
// Note that 'dst_alternatives' must NOT be stack allocated alternatives.
//

void _Con_Module_C_Earley_Parser_Parser_parse_func_extend_alternatives(Con_Obj *thread, parse_func_Parser_State *parser, parse_func_parse_Tree_Alternatives *dst_alternatives, parse_func_parse_Tree_Alternatives *new_alternatives)
{
	Con_Memory_make_array_room(thread, (void **) &dst_alternatives->entries, NULL, &dst_alternatives->num_entries_allocated, &dst_alternatives->num_entries, new_alternatives->num_entries, sizeof(parse_func_parse_Tree_Alternative));
	memmove(dst_alternatives->entries + dst_alternatives->num_entries, new_alternatives->entries, new_alternatives->num_entries * sizeof(parse_func_parse_Tree_Alternative));
	dst_alternatives->num_entries += new_alternatives->num_entries;
}



//
// Attempt to resolve ambiguities in 'alternatives', possibly by inspecting parse trees.
//
// Note that this function will either not effect 'alternatives' at all, or it will alter it so that
// only a single alternative remains.
//

void _Con_Module_C_Earley_Parser_Parser_parse_func_resolve_tree_ambiguities(Con_Obj *thread, parse_func_Parser_State *parser, parse_func_parse_Tree_Alternatives *alternatives)
{
	assert(alternatives->num_entries > 0);

	// First of all we check to see if each alternative has consumed the same number of tokens.
	//
	// At the same time, to avoid iterating over the list more than once, we also calculate
	// the positions of the alternatives with the highest and lowest precedences. If, after
	// this loop has completed, lowest_precedence_pos == highest_precedence_pos then all
	// alternatives have the same precedence.
	
	Con_Int start_lstate = alternatives->entries[0].lstate;
	for (Con_Int x = 1; x < alternatives->num_entries; x += 1) {
		if (alternatives->entries[x].lstate != start_lstate) {
			// If any of the alternatives have consumed differing numbers of tokens then we
			// can't yet distinguish between the alternatives.
			
			return;
		}
	}
	
	// At this point, we know that all of the alternatives have consumed the same number of
	// tokens. Therefore we're in a position to choose one of the alternatives as the one we
	// wish to work on.

	// We need to know the depth of the deepest tree.

	Con_Int max_depth = 0;
	for (Con_Int x = 0; x < alternatives->num_entries; x += 1) {
		Con_Int new_depth = _Con_Module_C_Earley_Parser_Parser_parse_calc_max_depth(thread, parser, alternatives->entries[0].tree);
		if (new_depth > max_depth)
			max_depth = new_depth;
	}
	
	for (Con_Int depth = 0; depth <= max_depth + 1; depth += 1) {
		Con_Int lowest_precedence = -1, highest_precedence = -1;
		for (Con_Int x = 0; x < alternatives->num_entries; x += 1) {
			Con_Int new_precedence = _Con_Module_C_Earley_Parser_Parser_parse_calc_precedence(thread, parser, alternatives->entries[x].tree, depth);
			if (new_precedence == 0)
				continue;
			
			if (lowest_precedence == -1)
				lowest_precedence = highest_precedence = new_precedence;
			else if (new_precedence < lowest_precedence)
				lowest_precedence = new_precedence;
			else if (new_precedence > highest_precedence)
				highest_precedence = new_precedence;
		}

		if (lowest_precedence == -1)
			continue;
		
		if (lowest_precedence == highest_precedence)
			break;

		Con_Int i = 0;
		while (i < alternatives->num_entries) {
			Con_Int precedence = _Con_Module_C_Earley_Parser_Parser_parse_calc_precedence(thread, parser, alternatives->entries[i].tree, depth);
			if (precedence == lowest_precedence) {
				i += 1;
				continue;
			}

			memmove(alternatives->entries + i, alternatives->entries + i + 1, (alternatives->num_entries - (i + 1)) * sizeof(parse_func_parse_Tree_Alternative));
			alternatives->num_entries -= 1;
		}
		
		return;
	}
	
	alternatives->num_entries = 1;
}



//
// Return the maximum depth of 'tree'.
//

Con_Int _Con_Module_C_Earley_Parser_Parser_parse_calc_max_depth(Con_Obj *thread, parse_func_Parser_State *parser, parse_func_parse_Tree *tree)
{
	Con_Int max_depth = 0;
	for (Con_Int x = 0; x < tree->num_entries; x += 1) {
		if (tree->entries[x].type == PARSE_TREE_TREE) {
			Con_Int new_depth = 1 + _Con_Module_C_Earley_Parser_Parser_parse_calc_max_depth(thread, parser, tree->entries[x].entry.tree);
			if (new_depth > max_depth)
				max_depth = new_depth;
		}
	}
	
	return max_depth;
}



//
// Return the maximum depth of any sub-tree of 'tree' at 'depth' levels in.
//

Con_Int _Con_Module_C_Earley_Parser_Parser_parse_calc_precedence(Con_Obj *thread, parse_func_Parser_State *parser, parse_func_parse_Tree *tree, Con_Int depth)
{
	if (depth == 0)
		return _GET_PRODUCTION_INT(tree->p, _COMPILED_PRODUCTION_PRECEDENCE);

	Con_Int highest_precedence = 0;
	for (Con_Int x = 0; x < tree->num_entries; x += 1) {
		if (tree->entries[x].type == PARSE_TREE_TREE) {
			Con_Int new_precedence = _Con_Module_C_Earley_Parser_Parser_parse_calc_precedence(thread, parser, tree->entries[x].entry.tree, depth - 1);
			
			if (new_precedence > highest_precedence)
				highest_precedence = new_precedence;
		}
	}
	
	return highest_precedence;
}



//
// Convert a parse tree to a more friendly Converge nested list.
//

Con_Obj *_Con_Module_C_Earley_Parser_Parser_parse_func_tree_to_list(Con_Obj *thread, parse_func_Parser_State *parser, parse_func_parse_Tree *tree)
{
	Con_Obj *list = Con_Builtins_List_Atom_new_sized(thread, tree->num_entries + 1);
	CON_GET_SLOT_APPLY(list, "append", parser->rule_names[_GET_PRODUCTION_INT(tree->p, _COMPILED_PRODUCTION_PARENT_RULE)]);
	
	// Note that entries in parse tree nodes are stored in reverse order, so we have to unpack them
	// in reverse order.
	
	for (Con_Int x = tree->num_entries - 1; x >= 0; x -= 1) {
		parse_func_parse_Tree_Entry *entry = &tree->entries[x];
		if (entry->type == PARSE_TREE_TOKEN)
			CON_GET_SLOT_APPLY(list, "append", entry->entry.token);
		else
			CON_GET_SLOT_APPLY(list, "append", _Con_Module_C_Earley_Parser_Parser_parse_func_tree_to_list(thread, parser, entry->entry.tree));
	}
	
	return list;
}
