//
// Created by Matteo Cardinaletti on 06/01/23.
//

#ifndef HASH_TABLE_HASH_TABLE_H
#define HASH_TABLE_HASH_TABLE_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/// Default function that has to be passed to hm_allocate
#define DEFAULT_H_FUNC hm_default_hash_func

/// Default capacity of a new hash_map
#define DEFAULT_CAPACITY 16

/// Default percentage for knowing when the hash_map should be reallocated
#define DEFAULT_S_REALLOCATE 0.75

/// Inspired by Fowler–Noll–Vo hash function (check it on wikipedia)
#define FNV_PRIME 1099511628211UL
#define FNV_OFFSET_BASIS 14695981039346656037UL

/// Simple way to convert everything into a string
#define stringify(_VAR) #_VAR

/// Generic map_entry struct definition
typedef struct map_entry {
    const char *key;                                                  // generic key
    void *value;                                                      // generic value
    struct map_entry *next;                                           // linked-list to handle collisions
} map_entry;

/// Hash_map struct definition
typedef struct hash_map {
    map_entry **entries;                                              // array of entries
    size_t size;                                                      // number of elements present
    size_t capacity;                                                  // max num. of items

    float should_reallocate;                                           // should the hash_map be reallocated?

    int (*hash_func)(struct hash_map *map, const char *key);          // hash function (default one is good)
} hash_map;

typedef struct map_iterator {
    map_entry *entry;                                                 // current entry

    hash_map *map;                                                    // hash_map (don't use manually)
    int index;                                                        // item index (don't use manually)
} map_iterator;

/// Default hash function (implementation of FNV-1a hash algorithm)
/// \param map hash map
/// \param key key (string, is present a macro to convert any type)
/// \return hash index
int hm_default_hash_func(hash_map *map, const char *key);

/// Init function for an hash_map
/// \param hash_func hash func. that has to be used
/// \param capacity map capacity
/// \param should_reallocate when the map should be reallocated
/// \return new hash map
hash_map *hm_allocate(int (*hash_func)(hash_map *map, const char *key),
                      size_t capacity, float should_reallocate);

/// Push an lvalue inside the hash_map
/// \param map hash map
/// \param key key (string, is present a macro to convert any type)
/// \param value some generic value
/// \return true or false if executed or not
bool hm_push(hash_map *map, const char *key, void *value);

/// Macro to push an rvalue instead of an lvalue
#define hm_push_direct(_T, _MAP, _KEY, _VAL) \
    do {                                     \
        _T variable = (_T) (_VAL);           \
        hm_push(_MAP, (char *)_KEY, (void *) &variable);      \
    } while (0)

/// Retrieve an item from the hash_map
/// \param map hash map
/// \param key key (string, is present a macro to convert any type)
/// \return pointer to the map_entry
map_entry *hm_get(hash_map *map, const char *key);

/// Free the map and its content
/// \param map hash map
/// \return true or false if executed or not
bool hm_free(hash_map *map);

/// Resize the hash_map to store more data
/// \param map hash map
/// \return true or false if executed or not
bool hm_resize(hash_map *map);

/// Clear the hash map without freeing the entries
/// \param map hash map
/// \return true or false if executed or not
bool hm_clear(hash_map *map);

/// Init function for map_iterator struct
/// \param map hash map
/// \return new iterator
map_iterator it_create(hash_map *map);

/// Return true or false if exec. or not, store the current entry inside it. var
/// \param it map iterator
/// \return true or false if executed or not
bool it_next(map_iterator *it);

/// Print out the hash_map
/// \param map hash map
/// \param hm_print_value callback function to print the correct type casted value
void hm_print(hash_map *map, void (*hm_print_value)(void *value));

#endif //HASH_TABLE_HASH_TABLE_H

#ifdef HM_IMPL

int hm_default_hash_func(hash_map *map, const char *key) {
    // if map or key is NULL, exit
    if (!map || !key) return -1;

    // hash variable
    uint64_t hash = FNV_OFFSET_BASIS;

    // do the FNV-1a algo over every element of the key
    for (int i = 0; i < strlen(key); ++i) {
        hash ^= key[i];
        hash *= FNV_PRIME;
    }

    // everything is fine, return the hash index
    return (int) (hash % map->capacity);
}

hash_map *hm_allocate(int (*hash_func)(hash_map *map, const char *key),
                      size_t capacity, float should_reallocate) {
    // if the hash_func is NULL, capacity is <= 0 or should_reallocate is <= 0, exit
    if (!hash_func || capacity <= 0 || should_reallocate <= 0.0f) return NULL;

    // create a new map
    hash_map *hm = malloc(sizeof(hash_map));
    // creating a new array of entries
    hm->entries = malloc(sizeof(map_entry *) * capacity);
    // setting all the items to NULL
    for (int i = 0; i < capacity; ++i) {
        hm->entries[i] = NULL;
    }

    // storing data inside the relative variables
    hm->size = 0;
    hm->capacity = capacity;

    hm->hash_func = hash_func;

    hm->should_reallocate = should_reallocate;

    // everything is fine, return the new map
    return hm;
}

bool hm_push(hash_map *map, const char *key, void *value) {
    // if map or key is NULL, exit
    if (!map || !key) return false;

    // creating a new entry
    map_entry *entry = malloc(sizeof(map_entry));
    // storing data inside variables
    entry->key = key;
    entry->value = value;
    // 'cause this is a linked list, the next item is NULL
    entry->next = NULL;

    // if the map need to be resized, call the relative function
    // the second member of the condition is "x : map->capacity = map->should_reallocate : 1" proportion
    if (map->size >= (int) ((float) (map->capacity) * map->should_reallocate)) hm_resize(map);

    // find the index of the new element
    int index = map->hash_func(map, key);
    // if the array at the new index does not contain anything, add the new entry there
    if (map->entries[index] == NULL) {
        map->entries[index] = entry;
        // update the size
        map->size++;
    }
        // add the new item from the tail of the linked list
    else {
        // storing the linked list to a temp. variable
        map_entry *tmp = map->entries[index];
        // looping over the linked list while reaching the last item or finding the new key already stored
        while (strcmp(map->entries[index]->key, key) != 0 && tmp->next != NULL) tmp = tmp->next;

        // if the next item is not NULL, update the entry
        if (strcmp(tmp->key, key) == 0) {
            tmp->value = value;

            // TODO: check why entry may point to deallocated memory
            free(entry);
        }
            // add the new element from the tail
        else tmp->next = entry;

        // everything is fine, success
        return true;
    }

    // everything is fine, success
    return true;
}

map_entry *hm_get(hash_map *map, const char *key) {
    // if map or key is NULL, exit
    if (!map || !key) return NULL;

    // evaluate the index with the hash function
    int index = map->hash_func(map, key);

    // if the item is directly the first one inside the linked list, return it
    if (strcmp(map->entries[index]->key, key) == 0) return map->entries[index];
    else {
        // storing the first entry inside a temp. variable
        map_entry *tmp = map->entries[index];
        // looping through the linked list
        while (tmp != NULL) {
            // if the two keys are equals, return the current entry
            if (strcmp(tmp->key, key) == 0) return tmp;

            // updating the temp. pointer
            tmp = tmp->next;
        }
    }

    // item not found inside the map
    return NULL;
}

bool hm_free(hash_map *map) {
    // if map is NULL, exit
    if (!map) return false;

    // storing the map entries inside a temp. array
    map_entry **entries = map->entries;
    // looping through all the entries to free them
    for (int i = 0; i < map->capacity; ++i) {
        if (entries[i] != NULL) {
            // looping through all the linked lists to free them
            while (entries[i] != NULL) {
                // store the head of the list inside a tmp. var
                map_entry *tmp = entries[i];
                // update the head of the list
                entries[i] = entries[i]->next;
                // free the node
                free(tmp);
            }
        }
    }

    // free the array of linked lists
    free(map->entries);
    // update the entries
    map->entries = NULL;
    // free the map struct
    free(map);
    // set the map to NULL
    map = NULL;

    // everything is fine, success
    return true;
}

bool hm_resize(hash_map *map) {
    // if map is NULL, exit
    if (!map) return false;

    // storing the entries inside a temp. array
    map_entry **old_entries = map->entries;

    // re-allocate the map entries with a new capacity
    map_entry **tmp = malloc(map->capacity * 2);
    // if realloc function failed, free tmp and exit
    if (!tmp) {
        // free the pointer
        free(tmp);
        // something wrong occurred, exit
        return false;
    }
    // updating map entries pointer to the tmp one
    map->entries = tmp;
    // reset the size back to 0
    map->size = 0;
    // updating map capacity with the new one
    map->capacity *= 2;

    // looping through the entries to hash and re-push them inside the map
    for (int i = 0; i < map->capacity / 2; ++i) {
        // if the item is not NULL, restore it inside the map
        if (old_entries[i] != NULL) {
            // push inside the map all the items of the current linked list
            map_entry *it = old_entries[i];
            // looping through all the nodes
            while (it != NULL) {
                // calling the relative function
                hm_push(map, it->key, it->value);

                // free the pushed element
                map_entry *node = it;
                it = it->next;
                free(node);
            }
        }
    }

    // free the pointer to the linked lists
    free(old_entries);

    // everything is fine, success
    return true;
}

bool hm_clear(hash_map *map) {
    // if map is NULL, exit
    if (!map) return false;

    // looping through all the entries
    for (int i = 0; i < map->capacity; ++i) {
        // if the current entries list is not NULL, replace it
        if (map->entries[i] != NULL) {
            // replacing the list with NULL pointer
            map->entries[i] = NULL;
        }
    }
    // reset the map's size to 0
    map->size = 0;

    // everything is fine, success
    return true;
}

map_iterator it_create(hash_map *map) {
    // creating a new iterator
    map_iterator iterator;

    // storing data inside the variables
    iterator.map = map;
    iterator.index = 0;
    iterator.entry = NULL;

    // everything is fine, return the iterator
    return iterator;
}

bool it_next(map_iterator *it) {
    // if iterator is NULL, exit
    if (!it) return false;

    // if the iterator's map is NULL, exit
    if (!(it->map) || !(it->map->entries)) return false;

    // looping until the condition is true
    while (it->index < it->map->capacity) {
        // store the iterator's index inside a temp. var.
        int index = it->index;
        // update the iterator's index
        it->index++;
        // if the item pointed by the index contains something, store it
        if (it->map->entries[index] != NULL) {
            // assign the entry pointed by the index to the iterator's entry
            it->entry = it->map->entries[index];

            // everything is fine, success
            return true;
        }
    }

    // no other elements inside the hash map, exit
    return false;
}

void hm_print(hash_map *map, void (*hm_print_value)(void *value)) {
    // if map is NULL, exit
    if (!map || !hm_print_value) return;

    // create a new map iterator
    map_iterator it = it_create(map);
    // printing the map
    printf("MAP (size -> %zu | capacity -> %zu): \n", map->size, map->capacity);
    while (it_next(&it)) {
        printf("\tslot [ %d ] ~ ", it.index - 1);

        do {
            printf("( %s, ", it.entry->key);
            hm_print_value(it.entry->value);
            printf(" )");

            if (it.entry->next != NULL) printf(" - ");

            it.entry = it.entry->next;
        } while (it.entry != NULL);

        printf("\n");
    }

    printf("\n");
}

#endif
