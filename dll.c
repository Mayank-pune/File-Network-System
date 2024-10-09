#include "headers.h"

// Function to create a new node with the given string data
Node *createNode(const char *data) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    newNode->data = strdup(data); // Duplicate the string
    newNode->prev = NULL;
    newNode->next = NULL;

    return newNode;
}

// Function to initialize an empty LRU cache
LRUCache *createLRUCache(int capacity, int ssnum) {
    LRUCache *cache = (LRUCache *)malloc(sizeof(LRUCache));
    if (cache == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    cache->capacity = capacity;
    cache->size = 0;
    cache->head = NULL;
    cache->tail = NULL;

    return cache;
}

// Function to move a node to the front of the list (most recently used)
void moveToFront(LRUCache *cache, Node *node) {
    if (node == cache->head) {
        // Node is already at the front
        return;
    }

    if (node->prev != NULL) {
        node->prev->next = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        // Node is the tail, update the tail
        cache->tail = node->prev;
    }

    node->next = cache->head;
    node->prev = NULL;

    if (cache->head != NULL) {
        cache->head->prev = node;
    }

    cache->head = node;

    // Update the tail if the node was the last node
    if (cache->tail == NULL) {
        cache->tail = node;
    }
}

// Function to remove the tail node (least recently used) from the list
void removeTail(LRUCache *cache) {
    if (cache->tail != NULL) {
        Node *tail = cache->tail;

        if (tail->prev != NULL) {
            tail->prev->next = NULL;
        } else {
            // Tail is also the head, update the head
            cache->head = NULL;
        }

        cache->tail = tail->prev;

        free(tail->data);
        free(tail);
    }
}

// Function to insert a new node with the given data at the front of the list
void insertAtFront(LRUCache *cache, const char *data) {
    Node *newNode = createNode(data);

    if (cache->head == NULL) {
        // The list is empty
        cache->head = newNode;
        cache->tail = newNode;
    } else {
        // Insert at the front
        newNode->next = cache->head;
        cache->head->prev = newNode;
        newNode->prev = NULL;
        cache->head = newNode;
    }

    cache->size++;
}

// Function to get a value from the LRU cache
int get(LRUCache *cache, const char *data) {
    Node *current = cache->head;

    // Search for the data in the cache
    while (current != NULL) {
        if (strcmp(current->data, data) == 0) {
            // Move the accessed node to the front
            moveToFront(cache, current);
            return 1;
        }
        current = current->next;
    }

    return -1; // Data not found in the cache
}

// Function to put a new data into the LRU cache
void put(LRUCache *cache, const char *data) {
    if (cache->size >= cache->capacity) {
        // Remove the least recently used node (tail)
        removeTail(cache);
        cache->size--;
    }

    // Insert the new data at the front
    insertAtFront(cache, data);
}

// Function to display the contents of the LRU cache
void displayLRUCache(LRUCache *cache) {
    Node *current = cache->head;
    while (current != NULL) {
        printf("%s ", current->data);
        current = current->next;
    }
    printf("\n");
}

// Function to free the memory used by the LRU cache
void destroyLRUCache(LRUCache *cache) {
    Node *current = cache->head;
    Node *next;

    while (current != NULL) {
        next = current->next;
        free(current->data);
        free(current);
        current = next;
    }

    free(cache);
}

// int main() {
//     LRUCache *stringLRUCache = createLRUCache(4, 0);

//     // Insert some strings into the LRU cache
//     put(stringLRUCache, "Hello");
//     put(stringLRUCache, "World");
//     put(stringLRUCache, "Doubly");
//     put(stringLRUCache, "Linked");
//     put(stringLRUCache, "List");

//     // Display the contents of the LRU cache
//     printf("LRU Cache contents: ");
//     displayLRUCache(stringLRUCache);

//     // Access a string to move it to the front
//     const char *accessedData = get(stringLRUCache, "Hello");
//     if (accessedData != NULL) {
//         printf("Data accessed: %s\n", accessedData);
//     }

//     // Display the updated contents of the LRU cache
//     printf("Updated LRU Cache contents: ");
//     displayLRUCache(stringLRUCache);

//     // Clean up the memory used by the LRU cache
//     destroyLRUCache(stringLRUCache);

//     return 0;
// }
