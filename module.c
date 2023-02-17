#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>

#include "module.h"
#include "printing.h"
#include "proc.h"

struct logwords logwords_data;

int logwords_init(void)
{
	int ret;

	pr_info("logwords: loading\n");
	mutex_init(&logwords_data.lock);
	ret = logwords_proc_init();
	if (ret < 0) {
		return ret;
	}
	logwords_printing_init();
	return 0;
}

void logwords_exit(void)
{
	pr_info("logwords: unloading\n");
	logwords_printing_exit();
	logwords_proc_exit();
	if (logwords_data.buffer.data) {
		kfree(logwords_data.buffer.data);
	}
}

MODULE_LICENSE("GPL");
module_init(logwords_init);
module_exit(logwords_exit);
