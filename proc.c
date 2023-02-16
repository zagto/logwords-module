#include <linux/proc_fs.h>

#include "proc.h"
#include "module.h"

static ssize_t logwords_proc_read(struct file *file, char __user *buf,
				  size_t size, loff_t *ppos)
{
	ssize_t ret;

	mutex_lock(&logwords_data.lock);
	if (*ppos > logwords_data.size) {
		ret = -EOVERFLOW;
		goto out;
	}
	if (*ppos + size > logwords_data.size) {
		size = logwords_data.size - *ppos;
	}

	if (copy_to_user(buf, logwords_data.data + *ppos, size) != 0) {
		pr_err("logwords: unable to copy output read from logwords"
		       "file\n");
		ret = -EFAULT;
		goto out;
	}
	*ppos += size;
	ret = size;

out:
	mutex_unlock(&logwords_data.lock);
	return ret;
}

static ssize_t logwords_proc_write(struct file *file, const char __user *buf,
				   size_t size, loff_t *ppos)
{
	char *new_data;
	size_t new_size;
	ssize_t ret;

	mutex_lock(&logwords_data.lock);

	new_size = (size_t)*ppos + size;
	new_data = kmalloc(new_size, GFP_KERNEL);
	if (new_data == NULL) {
		pr_err("logwords: failed to allocate new data buffer\n");
		ret = -ENOMEM;
		goto out;
	}

	if (logwords_data.data) {
		memcpy(new_data, logwords_data.data,
		       min(logwords_data.size, new_size));
	}

	if (copy_from_user(new_data + *ppos, buf, size) != 0) {
		pr_err("logwords: unable to copy input written to logwords"
		       "file\n");
		kfree(new_data);
		ret = -EFAULT;
		goto out;
	}

	if (logwords_data.data) {
		kfree(logwords_data.data);
	}
	logwords_data.data = new_data;
	logwords_data.size = new_size;

	/* reset current output position to 0 so it doesn't end up in the middle
	 * of a word */
	logwords_data.printing_position = 0;

	*ppos += size;
	ret = size;

out:
	mutex_unlock(&logwords_data.lock);
	return ret;
}

static struct proc_ops logwords_proc_ops = {
	.proc_read = logwords_proc_read,
	.proc_write = logwords_proc_write,
};

int logwords_proc_init(void)
{
	logwords_data.proc_entry = proc_create("logwords", 0644, NULL,
					       &logwords_proc_ops);
	if (logwords_data.proc_entry == NULL) {
		pr_err("unable to create /proc/logwords file\n");
		return -ENOENT;
	}
	return 0;
}

void logwords_proc_exit(void)
{
	proc_remove(logwords_data.proc_entry);
}
