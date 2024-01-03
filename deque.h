#pragma once

#include <stdint.h>
#include <stdlib.h>

typedef struct point_t {
    int32_t x;
    int32_t y;
} point_t;

typedef struct info_t {
    struct info_t *last;
    point_t value;
    int32_t cost;
} info_t;

typedef struct node_t {
    struct node_t *next;
    struct node_t *last;
    info_t value;
} node_t;

typedef struct deque_t {
    node_t *head;
    node_t *tail;
    int32_t size;
} deque_t;

void deque_push_back(deque_t *deque, info_t info)
{
    node_t *node = (node_t *) malloc(sizeof(node_t));
    if (!node) return;

    node->value = info;
    node->next = NULL;

    if (deque->size <= 0) {
        deque->head = node;
        deque->tail = node;
    }
    else {
        node->last = deque->tail;
        deque->tail->next = node;
        deque->tail = node;
    }
    deque->size++;
}

void deque_pop(deque_t *deque)
{
    if(deque->size <= 0) return;

    if(deque->size == 1) {
        free(deque->head);
        deque->head = NULL;
        deque->tail = NULL;
    } else {
        node_t *next_node = deque->head->next;
        next_node->last = NULL;
        free(deque->head);
        deque->head = next_node;
    }
    deque->size--;
}
