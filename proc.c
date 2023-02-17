#include <linux/proc_fs.h>
#include <linux/slab.h>

#include "proc.h"
#include "printing.h"
#include "module.h"

static ssize_t logwords_proc_read(struct file *file, char __user *buf,
				  size_t size, loff_t *ppos)
{
	ssize_t ret;

	mutex_lock(&logwords_data.lock);
	if (*ppos > logwords_data.buffer.size) {
		ret = -EOVERFLOW;
		goto out;
	}
	if (*ppos + size > logwords_data.buffer.size) {
		size = logwords_data.buffer.size - *ppos;
	}

	if (copy_to_user(buf, logwords_data.buffer.data + *ppos, size) != 0) {
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
	struct logwords_buffer new_buffer;
	ssize_t ret;

	mutex_lock(&logwords_data.lock);

	new_buffer.size = (size_t)*ppos + size;
	new_buffer.data = kmalloc(new_buffer.size, GFP_KERNEL);
	if (new_buffer.data == NULL) {
		pr_err("logwords: failed to allocate new data buffer\n");
		ret = -ENOMEM;
		goto out;
	}

	if (logwords_data.buffer.data) {
		memcpy(new_buffer.data, logwords_data.buffer.data,
		       min(logwords_data.buffer.size, new_buffer.size));
	}

	if (copy_from_user(new_buffer.data + *ppos, buf, size) != 0) {
		pr_err("logwords: unable to copy input written to logwords"
		       "file\n");
		kfree(new_buffer.data);
		ret = -EFAULT;
		goto out;
	}

	ret = logwards_printing_new_data(&new_buffer);
	if (ret < 0) {
		kfree(new_buffer.data);
		goto out;
	}

	if (logwords_data.buffer.data) {
		kfree(logwords_data.buffer.data);
	}
	logwords_data.buffer = new_buffer;

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
