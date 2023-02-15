#include <linux/kernel.h>

#include "module.h"
#include "printing.h"

static bool is_word_character(char c)
{
	return c > ' ';
}

static size_t find_word_character(size_t start_position)
{
	size_t position = start_position;
	while (position < logwords_data.size
			&& !is_word_character(logwords_data.data[position])) {
		position++;
	}
	return position;
}

static size_t find_non_word_character(size_t start_position)
{
	size_t position = start_position;
	while (position < logwords_data.size
			&& is_word_character(logwords_data.data[position])) {
		position++;
	}
	return position;
}

static enum hrtimer_restart timer_function(struct hrtimer *timer)
{
	size_t start, end;
	int print_length;

	mutex_lock(&logwords_data.lock);

	start = find_word_character(logwords_data.printing_position);
	if (start == logwords_data.size) {
		start = find_word_character(0);
		if (start == logwords_data.size) {
			/* no word characters - skip printing */
			goto out;
		}
	}

	end = find_non_word_character(start);


	if (end - start > INT_MAX) {
		pr_err("logwords: word longer than INT_MAX not supported\n");
		print_length = INT_MAX;
	} else {
		print_length = (int)(end - start);
	}

	printk("logwords: %.*s\n", print_length, logwords_data.data + start);

	logwords_data.printing_position = end;

out:
	hrtimer_forward_now(timer, ktime_set(1, 0));
	mutex_unlock(&logwords_data.lock);
	return HRTIMER_RESTART;
}

void logwords_printing_init(void) {
	hrtimer_init(&logwords_data.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	logwords_data.timer.function = timer_function;
	hrtimer_start(&logwords_data.timer, ktime_set(1, 0), HRTIMER_MODE_REL);
}

void logwords_printing_exit(void)
{
	int ret = hrtimer_cancel(&logwords_data.timer);
	if (ret < 0) {
		pr_err("logwords: Failed to cancel timer: %i\n", ret);
	}
}

