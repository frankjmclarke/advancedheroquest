/*****************************************************************************
 * VAR_ARGS.H
 *****************************************************************************/

#ifndef __VAR_ARGS_H__
#define __VAR_ARGS_H__

#if defined(__cplusplus) || defined(__STDC__) || defined(HAVE_STDARG_H) || defined(STDC_HEADERS)
#  include <stdarg.h>
#  define __c_va_alist				, ...
#  define __c_va_dcl
#  define __c_va_list				va_list
#  define __c_va_start(pvar, prev) 	va_start(pvar, prev)
#  define __c_va_arg(pvar, type)	va_arg(pvar, type)
#  define __c_va_end(pvar) 			va_end(pvar)

/*
 * At least DEC/OSF1 uses a struct for type va_list,
 * so we have to use a function that returns an
 * empty argument list to specify no
 * arguments, because converting a NULL
 * pointer to a struct doesn't work
 */
#  define __c_va_no_args va_empty_args()
extern __c_va_list va_empty_args(void);

#else
#  include <varargs.h>
#  define __c_va_alist				, va_alist
#  define __c_va_dcl				va_dcl
#  define __c_va_list				va_list
#  define __c_va_start(pvar, prev) 	va_start(pvar)
#  define __c_va_arg(pvar, type)	va_arg(pvar, type)
#  define __c_va_end(pvar) 			va_end(pvar)
#endif

#ifndef __c_va_no_args
#  define __c_va_no_args NULL
#endif

#endif /* __VAR_ARGS_H__ */
