#pragma once

#include <linux/mutex.h>
#include <linux/hrtimer.h>

struct proc_dir_entry;

struct logwords {
	struct mutex lock;
	char *data;
	size_t size;
	size_t printing_position;
	struct hrtimer timer;
	struct proc_dir_entry *proc_entry;
};

extern struct logwords logwords_data;
