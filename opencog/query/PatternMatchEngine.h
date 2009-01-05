/*
 * PatternMatchEngine.h
 *
 * Author: Linas Vepstas February 2008
 *
 * Copyright (C) 2008,2009 Linas Vepstas <linasvepstas@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _OPENCOG_PATTERN_MATCH_ENGINE_H
#define _OPENCOG_PATTERN_MATCH_ENGINE_H

#include <map>
#include <stack>

#include <opencog/atomspace/types.h>
#include <opencog/atomspace/Atom.h>
#include <opencog/query/OutgoingTree.h>
#include <opencog/query/PatternMatchCallback.h>

namespace opencog {

typedef std::vector<Handle> RootList;
typedef std::map<Handle, RootList *> RootMap;
typedef std::pair<Handle, RootList *> RootPair;

class PatternMatchEngine
{
	private:
		AtomSpace *atom_space;

		bool prt(Atom *);

		static void prtmsg(const char *, Atom *);
		static void prtmsg(const char *, Handle);

		// -------------------------------------------
		// predicates to be solved.
		std::vector<Handle> normed_predicate;
		std::set<Handle> bound_vars;

		// -------------------------------------------
		// Traversal utilities
		RootMap root_map;
		Handle curr_root;
		bool note_root(Handle);
		
		// -------------------------------------------
		// Examine each candidate for a match, in turn.
		bool do_candidate(Handle);

		// Recurisve tree comparison algorithm.
		bool tree_compare(Atom *, Atom *);
		int depth;  // recursion depth for tree_compare.

		bool pred_up(Handle);
		bool soln_up(Handle);
		OutgoingTree ot;
		Handle curr_soln_handle;
		Handle curr_pred_handle;
		void get_next_unsolved_pred(void);

		// Stack used during recursive exploration
		std::stack<Handle> pred_handle_stack;
		std::stack<Handle> soln_handle_stack;
		std::stack<Handle> root_handle_stack;
		typedef std::map<Handle, Handle> SolnMap;
		std::stack<SolnMap> pred_solutn_stack;

		// -------------------------------------------

		// Result of solving the predicate
		std::map<Handle, Handle> var_solution;
		std::map<Handle, Handle> predicate_solution;

		// callback to report results.
		PatternMatchCallback *pmc;

	public:
		PatternMatchEngine(void);
		void set_atomspace(AtomSpace *);

		void match(PatternMatchCallback *,
		           std::vector<Handle> *clauses,
		           std::vector<Handle> *vars);

		static void print_solution(std::map<Handle, Handle> &preds,
		                           std::map<Handle, Handle> &vars);

};

} // namespace opencog

#endif // _OPENCOG_PATTERN_MATCH_ENGINE_H
