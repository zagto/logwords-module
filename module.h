#pragma once

#include <linux/mutex.h>

struct proc_dir_entry;
struct logwords_word;

struct logwords_buffer {
	char *data;
	size_t size;
};

struct logwords {
	struct mutex lock;
	struct logwords_buffer buffer;
	struct logwords_word *printing_head;
	struct logwords_word *printing_node;
	struct proc_dir_entry *proc_entry;
};

extern struct logwords logwords_data;
