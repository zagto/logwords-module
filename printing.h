#pragma once

#include <asm/posix_types.h>

struct logwords_buffer;

void logwords_printing_init(void);
void logwords_printing_exit(void);
int logwards_printing_new_data(const struct logwords_buffer *buffer);
