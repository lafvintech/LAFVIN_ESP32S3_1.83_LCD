/**
 * @file input_handler.h
 * @brief Input Event Abstract Header File
 */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    INPUT_EVENT_NONE = 0,
    INPUT_EVENT_SINGLE_CLICK,
    INPUT_EVENT_DOUBLE_CLICK,
    INPUT_EVENT_TRIPLE_CLICK,
} InputEvent;

void input_handler_init(void);
InputEvent input_handler_poll(void);
bool input_handler_peek_serial_debug_char(char* out_char);
void input_handler_consume_serial_debug_char(void);

#ifdef __cplusplus
}
#endif

#endif // INPUT_HANDLER_H
