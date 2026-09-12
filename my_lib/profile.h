/*****************************************************************************
 * PROFILE.H
 *****************************************************************************/

#ifndef __PROFILE_H__
#define __PROFILE_H__

#ifndef __PORTAB_H__
#  include <portab.h>
#endif

_VOID Profile_UseFile(CONST _UBYTE *filename, CONST _UBYTE *author);

_VOID Profile_WriteString(CONST _UBYTE *section, CONST _UBYTE *key, CONST _UBYTE *str);
_VOID Profile_WriteInt(CONST _UBYTE *section, CONST _UBYTE *key, _WORD val);
_VOID Profile_WriteCard(CONST _UBYTE *section, CONST _UBYTE *key, _UWORD val);
_VOID Profile_WriteLong(CONST _UBYTE *section, CONST _UBYTE *key, _LONG val);
_VOID Profile_WriteLongcard(CONST _UBYTE *section, CONST _UBYTE *key, _ULONG val);
_VOID Profile_WriteBool(CONST _UBYTE *section, CONST _UBYTE *key, _BOOL val);

_BOOL Profile_ReadString(CONST _UBYTE *section, CONST _UBYTE *key, CONST _UBYTE *def, _UBYTE *str, size_t len);
_BOOL Profile_ReadInt(CONST _UBYTE *section, CONST _UBYTE *key, _WORD def, _WORD *pval);
_BOOL Profile_ReadLong(CONST _UBYTE *section, CONST _UBYTE *key, _LONG def, _LONG *pval);
_BOOL Profile_ReadCard(CONST _UBYTE *section, CONST _UBYTE *key, _UWORD def, _UWORD *pval);
_BOOL Profile_ReadLongCard(CONST _UBYTE *section, CONST _UBYTE *key, _ULONG def, _ULONG *pval);
_BOOL Profile_ReadBool(CONST _UBYTE *section, CONST _UBYTE *key, _BOOL def, _BOOL *pval);

_VOID Profile_DeleteKey(CONST _UBYTE *section, CONST _UBYTE *key);
_VOID Profile_DeleteSection(CONST _UBYTE *section);

#endif  /* __PROFILE_H__ */
