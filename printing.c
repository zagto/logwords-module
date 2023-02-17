#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

#include "module.h"
#include "printing.h"

struct logwords_word {
	size_t start;
	size_t length;
	struct list_head list_node;
};

static void workqueue_function(struct work_struct *work);
DECLARE_DELAYED_WORK(logwords_workqueue, workqueue_function);

static bool is_word_character(char c)
{
	return c > ' ';
}

static size_t find_word_character(const struct logwords_buffer *buffer,
				  size_t start_position)
{
	size_t position = start_position;
	while (position < buffer->size
			&& !is_word_character(buffer->data[position])) {
		position++;
	}
	return position;
}

static size_t find_non_word_character(const struct logwords_buffer *buffer,
				      size_t start_position)
{
	size_t position = start_position;
	while (position < buffer->size
			&& is_word_character(buffer->data[position])) {
		position++;
	}
	return position;
}

static void workqueue_function(struct work_struct *work)
{
	struct logwords_word *word;
	int print_length;

	mutex_lock(&logwords_data.lock);

	word = logwords_data.printing_node;
	if (word == NULL) {
		/* no word characters - skip printing */
		goto out;
	}
	if (word->length > INT_MAX) {
		pr_err("logwords: word longer than INT_MAX not supported\n");
		print_length = INT_MAX;
	} else {
		print_length = (int)word->length;
	}

	printk("logwords: %.*s\n", print_length,
	       logwords_data.buffer.data + word->start);

	logwords_data.printing_node = list_next_entry(word, list_node);

out:
	mutex_unlock(&logwords_data.lock);
	schedule_delayed_work(&logwords_workqueue, HZ);
}

static void free_whole_list(struct logwords_word *head) {
	struct logwords_word *current_word, *loop_storage;

	if (head != NULL) {
		list_for_each_entry_safe(current_word,
				loop_storage, &head->list_node, list_node) {
			list_del(&current_word->list_node);
			kfree(current_word);
		}
	};
}

int logwards_printing_new_data(const struct logwords_buffer *buffer)
{
	size_t position = 0;
	size_t word_start, word_end;
	struct logwords_word *word_list = NULL;
	struct logwords_word *word_list_to_delete = NULL;
	struct logwords_word *current_word;
	int ret = 0;

	BUG_ON(!mutex_is_locked(&logwords_data.lock));

	while (position < buffer->size) {
		word_start = find_word_character(buffer, position);
		if (word_start == buffer->size) {
			/* no more words in buffer */
			break;
		}

		word_end = find_non_word_character(buffer, word_start);
		current_word = kzalloc(
				sizeof(struct logwords_word), GFP_KERNEL);
		if (current_word == NULL) {
			ret = -ENOMEM;
			word_list_to_delete = word_list;
			goto out;
		}
		current_word->start = word_start;
		current_word->length = word_end - word_start;
		if (word_list) {
			list_add(&current_word->list_node,
				 &word_list->list_node);
		} else {
			INIT_LIST_HEAD(&current_word->list_node);
		}
		word_list = current_word;
		position = word_end;
	}

	word_list_to_delete = logwords_data.printing_head;
	logwords_data.printing_head = word_list;
	if (word_list) {
		logwords_data.printing_node = list_first_entry(
				&word_list->list_node,
				struct logwords_word, list_node);
	} else {
		logwords_data.printing_node = NULL;
	}

out:
	free_whole_list(word_list_to_delete);
	return ret;
}

void logwords_printing_init(void)
{
	schedule_delayed_work(&logwords_workqueue, HZ);
}

void logwords_printing_exit(void)
{
	cancel_delayed_work_sync(&logwords_workqueue);
	free_whole_list(logwords_data.printing_head);
}

