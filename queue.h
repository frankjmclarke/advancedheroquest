#ifndef __QUEUE_H__
#define __QUEUE_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __PICE_H__
#include <pice.h>
#endif

_BOOL init_queues(_VOID);
_BOOL push_room(PICE *ptr);
_BOOL push_passage(PICE *ptr);
_BOOL pop_room(PICE *ptr);
_BOOL pop_passage(PICE *ptr);

#endif /* __QUEUE_H__ */
