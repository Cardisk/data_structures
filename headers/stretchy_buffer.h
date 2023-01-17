//
// Created by Matteo Cardinaletti on 06/01/23.
//

#ifndef STRETCHY_BUFFERS_STRETCHY_BUFFER_H
#define STRETCHY_BUFFERS_STRETCHY_BUFFER_H

#include <stdlib.h>
#include <stdbool.h>

/// This struct represent the header of the buffer
typedef struct buffer_h {
    size_t size;            // the very first empty space of the array
    size_t capacity;        // how many items can be stored into the buffer
} buffer_h;

/// Macro to create a generic buffer (it basically does the same thing as c but with a more oob approach)
#define s_buffer(T) T*

/// It returns the header of the buffer
#define buffer_header(_BUF) (((buffer_h *) (_BUF)) - 1)

/// It returns the size of the buffer (number of elements)
#define buffer_size(_BUF) ((_BUF) ? (buffer_header(_BUF))->size : 0)
/// It returns the capacity of the buffer
#define buffer_capacity(_BUF) ((_BUF) ? (buffer_header(_BUF))->capacity : 0)

/// It returns the pointer to the first empty element
# define buffer_get_ptr(_BUF) (buffer_grow(_BUF), &((_BUF)[buffer_header(_BUF)->size]))
/// It allows to push an element inside the buffer
#define buffer_push(_BUF, _VAL) ((*(buffer_get_ptr(_BUF)) = (_VAL)), (buffer_header(_BUF))->size++)

/// It clears the buffer (reset the first empty space to 0)
#define buffer_clear(_BUF) ((_BUF) ? ((buffer_header(_BUF))->size = 0) : 0)
/// It frees the buffer memory and it sets the buffer pointer to NULL
#define buffer_free(_BUF) (((_BUF) ? free(buffer_header(_BUF)) : 0), (_BUF) = NULL)

/// If statement to check if a pointer is NULL
#define check_ptr(_PTR) if (!(_PTR)) { free(_PTR); return; }
/// It resizes the buffer to store new data
static void buffer_resize(void **buffer, size_t val_size) {
    // dumb method to determine the new capacity
    size_t new_capacity = buffer_capacity(*buffer) <= 10 ? buffer_capacity(*buffer) * 2 : buffer_capacity(*buffer) * 0.5;
    new_capacity += (buffer_capacity(*buffer) + 1);

    // numbers of bytes to allocate
    size_t bytes = sizeof(buffer_h) + (val_size * new_capacity);

    // new buffer_h pointer
    buffer_h* tmp = NULL;
    if (*buffer) {
        // if the buffer passed is not NULL, reallocate it
        tmp = (buffer_h*) realloc(buffer_header(*buffer), bytes);
        check_ptr(tmp)
    } else {
        // if the buffer passed is NULL, allocate it
        tmp = (buffer_h*) malloc(bytes);
        check_ptr(tmp)
        tmp->size = 0;
    }

    tmp->capacity = new_capacity;

    // set the buffer passed to the new pointer + 1 (the first item of the array, ignoring the header)
    *buffer = tmp + 1;
}

/// It checks whether or not the buffer needs to grow
#define buffer_grow(_BUF) (((buffer_size(_BUF)) == (buffer_capacity(_BUF))) ? buffer_resize(((void**)&(_BUF)), sizeof(*(_BUF))) : 0)

#endif //STRETCHY_BUFFERS_STRETCHY_BUFFER_H
