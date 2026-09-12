#include <queue.h>

typedef struct queue
{
	PICE *pice;
	_WORD first, last;
} QUEUE;

LOCAL QUEUE Room_queue, Passage_queue;

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _BOOL init_queues(_VOID)
{
	PICE *ptr = Pice;
	
	ptr += MAX_PICE;
	Passage_queue.pice = ptr;
	Passage_queue.last = Passage_queue.first = 0;
	
	ptr += MAX_PICE;
	Room_queue.pice = ptr;
	Room_queue.last = Room_queue.first = 0;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL pop_first(QUEUE *queue_ptr, PICE *pice_ptr)
{
	if (queue_ptr->first != queue_ptr->last)
	{
		copy_pice(pice_ptr, &queue_ptr->pice[queue_ptr->first]);
		if (queue_ptr->first++ >= MAX_PICE)
		{
			queue_ptr->first = 0;
		}
		return TRUE;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

#if 0
LOCAL _BOOL pop_last(QUEUE *queue_ptr, PICE *pice_ptr)
{
	if (queue_ptr->first != queue_ptr->last)
	{
		if (queue_ptr->last == 0)
		{
			queue_ptr->last = MAX_PICE;
		}
		copy_pice(pice_ptr, &queue_ptr->pice[--queue_ptr->last]);
		return TRUE;
	}
	return FALSE;
}
#endif

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL push(QUEUE *queue_ptr, PICE *pice_ptr)
{
	_WORD new;
	
	new = queue_ptr->last;
	
	if (new++ >= MAX_PICE)
	{
		new = 0;
	}
	
	if (queue_ptr->first != new)
	{
		copy_pice(&queue_ptr->pice[queue_ptr->last], pice_ptr);
		queue_ptr->last = new;
		return TRUE;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL push_room(PICE *ptr)
{
	return push(&Room_queue, ptr);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL push_passage(PICE *ptr)
{
	return push(&Passage_queue, ptr);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL pop_room(PICE *ptr)
{
	return pop_first(&Room_queue, ptr);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL pop_passage(PICE *ptr)
{
	return pop_first(&Passage_queue, ptr);
}
