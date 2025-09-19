#ifndef _QTK_CORE_QTK_QUEUE_H_
#define _QTK_CORE_QTK_QUEUE_H_

#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "qtk_api.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define qtk_data_offset(q,type,link) (type*)((q) ? (void*)((char*)q-offsetof(type,link)) : NULL)
#define qtk_data_offset2(q,type,link) (type*)((void*)((char*)q-offsetof(type,link)))

typedef struct qtk_queue_node qtk_queue_node_t;
typedef struct qtk_queue qtk_queue_t;

#define qtk_queue_node_init(n) (n)->next=(n)->prev=0
#define qtk_queue_init(q) memset(q,0,sizeof(*q))

typedef void (*queue_push_listener)(void* data);

struct qtk_queue_node
{
	qtk_queue_node_t* next;
	qtk_queue_node_t* prev;
};

#define QTK_QUEUE \
	qtk_queue_node_t *pop; \
	qtk_queue_node_t *push; \
	int length;\
	queue_push_listener listener; \
	void *data; \

struct qtk_queue
{
	QTK_QUEUE
};

/**
 * @brief push node to the tail of the queue;
 */
DLL_API int qtk_queue_push(qtk_queue_t *q,qtk_queue_node_t *n);

/**
 * @brief push node to the head of the queue;
 */
DLL_API int qtk_queue_push_front(qtk_queue_t *q,qtk_queue_node_t *n);

/**
 * @brief remove node from queue;
 * @warning: make sure node n is in queue;
 */
DLL_API int qtk_queue_remove(qtk_queue_t *q,qtk_queue_node_t *n);

/**
 *	@brief pop the head node from queue;
 */
DLL_API qtk_queue_node_t* qtk_queue_pop(qtk_queue_t *q);

#ifdef __cplusplus
};
#endif
#endif
