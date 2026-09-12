/*****************************************************************************
 * HELP.H
 *****************************************************************************/

#ifndef __RO_HELP_H__
#define __RO_HELP_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif

_BOOL help_init(CONST _UBYTE *help_name);
_BOOL help_show(CONST _UBYTE *entry);
_BOOL help_show_with(CONST _UBYTE *filename, CONST _UBYTE *entry);
_VOID help_contents(_VOID);
_VOID help_index(_VOID);
_VOID help_using_help(_VOID);
_BOOL help_ok(_VOID);

#endif /* __RO_HELP_H__ */
