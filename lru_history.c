#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apsh_module.h"


// Simple strdup implementation (in case your compiler doesn't have strdup)
char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

LRUCache *lru_create(int capacity) {
    LRUCache *cache = malloc(sizeof(LRUCache));
    if (!cache) return NULL;
    cache->head = cache->tail = NULL;
    cache->capacity = capacity;
    cache->size = 0;
    return cache;
}

void lru_free(LRUCache *cache) {
    Node *cur = cache->head;
    while (cur) {
        Node *next = cur->next;
        free(cur->cmd);
        free(cur);
        cur = next;
    }
    free(cache);
}

// Move a node to the front (head) of the list
void move_to_front(LRUCache *cache, Node *node) {
    if (cache->head == node) return;  // already at front

    // Detach node
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;

    // If it was tail, update tail
    if (cache->tail == node)
        cache->tail = node->prev;

    // Insert at front
    node->prev = NULL;
    node->next = cache->head;
    if (cache->head)
        cache->head->prev = node;
    cache->head = node;

    if (cache->tail == NULL)
        cache->tail = node;
}

// Remove node from the tail (LRU)
void remove_tail(LRUCache *cache) {
    if (!cache->tail) return;

    Node *old_tail = cache->tail;
    if (old_tail->prev) {
        cache->tail = old_tail->prev;
        cache->tail->next = NULL;
    } else {
        // Only one element
        cache->head = cache->tail = NULL;
    }

    free(old_tail->cmd);
    free(old_tail);
    cache->size--;
}

// Add command to LRU history
void lru_put(LRUCache *cache, const char *cmd) {
    // First see if command already exists (simple O(n) search)
    Node *cur = cache->head;
    while (cur) {
        if (strcmp(cur->cmd, cmd) == 0) {
            // Move this existing node to front
            move_to_front(cache, cur);
            return;
        }
        cur = cur->next;
    }

    // Create new node
    Node *node = malloc(sizeof(Node));
    if (!node) return;
    node->cmd = my_strdup(cmd);
    if (!node->cmd) {
        free(node);
        return;
    }
    node->prev = node->next = NULL;

    // Insert at front
    node->next = cache->head;
    if (cache->head)
        cache->head->prev = node;
    cache->head = node;
    if (cache->tail == NULL)
        cache->tail = node;

    cache->size++;

    // If over capacity, remove LRU (tail)
    if (cache->size > cache->capacity) {
        remove_tail(cache);
    }
}

// Print history from most recent to least recent
int lru_print(LRUCache *cache) {
    Node *cur = cache->head;
    int i =1;
    printf("History (MRU -> LRU):\n");
    while (cur) {
        printf("%d. %s\n",i, cur->cmd);
        cur = cur->next;
        i++;
    }
    return 1;
}

