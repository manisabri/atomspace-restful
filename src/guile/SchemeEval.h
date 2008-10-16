/*
 * SchemeEval.h
 *
 * Simple scheme expression evaluator
 * Copyright (c) 2008 Linas Vepstas <linas@linas.org>
 */

#ifndef OPENCOG_SCHEME_EVAL_H
#define OPENCOG_SCHEME_EVAL_H
#ifdef HAVE_GUILE

#include <string>
#include <libguile.h>

namespace opencog {

class SchemeSmob;

class SchemeEval
{
	private:
		static bool is_inited;

		// Error handling stuff
		SCM error_string_port;
		SCM captured_stack;
		static SCM preunwind_handler_wrapper(void *, SCM, SCM);
		static SCM catch_handler_wrapper(void *, SCM, SCM);
		SCM preunwind_handler(SCM, SCM);
		SCM catch_handler(SCM, SCM);
		bool caught_error;

		// printfing of basic types
		std::string prt(SCM);

		// output port
		SCM outport;

		SchemeSmob *funcs;

	public:
		SchemeEval(void);
		std::string eval(const std::string &);
};

}

#endif/* HAVE_GUILE */
#endif /* OPENCOG_SCHEME_EVAL_H */
