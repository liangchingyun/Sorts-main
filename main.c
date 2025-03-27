#include "list.h"
#include "list_sort.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct element {
	struct list_head list;
	int val;
	int seq;
} element_t;

#define SAMPLES ((1 << 20) + 20)

static void create_sample(struct list_head *head, element_t *space, int samples)
{
	for (int i = 0; i < samples; i++) {
		element_t *elem = space + i;
		elem->val = rand();
		elem->seq = i;
		list_add_tail(&elem->list, head);
	}
}

static void copy_list(const struct list_head *from, struct list_head *to,
		      element_t *space)
{
	if (list_empty(from))
		return;

	element_t *entry;
	list_for_each_entry(entry, from, list) {
		element_t *copy = space++;
		copy->val = entry->val;
		copy->seq = entry->seq;
		list_add_tail(&copy->list, to);
	}
}

#if 0
static void free_list(struct list_head *head)
{
	element_t *entry, *safe;
	list_for_each_entry_safe(entry, safe, head, list) {
		list_del(&entry->list);
		free(entry);
	}
}
#endif

int compare(void *priv, const struct list_head *a, const struct list_head *b)
{
	if (a == b)
		return 0;

	int res = list_entry(a, element_t, list)->val -
		  list_entry(b, element_t, list)->val;

	if (priv) {
		*((int *)priv) += 1;
	}

	return res;
}

bool check_list(const struct list_head *head, int count)
{
	if (list_empty(head))
		return count == 0;

	element_t *entry, *safe;
	list_for_each_entry_safe(entry, safe, head, list) {
		if (entry->list.next != head) {
			if (entry->val > safe->val ||
			    (entry->val == safe->val &&
			     entry->seq > safe->seq)) {
				return false;
			}
		}
	}
	return true;
}

typedef void (*test_func_t)(void *priv, struct list_head *head,
			    list_cmp_func_t cmp);

typedef struct test {
	test_func_t fp;
	char *name;
} test_t;

int main(void)
{
	struct list_head sample_head, warmdata_head, testdata_head;
	element_t *samples, *warmdata, *testdata;
	int count;
	int nums = SAMPLES;

	srand(1050);

	test_t tests[] = { { list_sort, "list_sort" },
			   { shiverssort, "shiverssort" },
			   { timsort, "timsort" },
			   { NULL, NULL } },
	       *test = tests;

	INIT_LIST_HEAD(&sample_head);

	samples = malloc(sizeof(*samples) * SAMPLES);
	warmdata = malloc(sizeof(*warmdata) * SAMPLES);
	testdata = malloc(sizeof(*testdata) * SAMPLES);

	create_sample(&sample_head, samples, nums);

	while (test->fp != NULL) {
		printf("==== Testing %s ====\n", test->name);
		/* Warm up */
		INIT_LIST_HEAD(&warmdata_head);
		INIT_LIST_HEAD(&testdata_head);
		copy_list(&sample_head, &testdata_head, testdata);
		copy_list(&sample_head, &warmdata_head, warmdata);
		test->fp(&count, &warmdata_head, compare);
		/* Test */
		clock_t begin;
		count = 0;
		begin = clock();
		test->fp(&count, &testdata_head, compare);
		printf("  Elapsed time:   %ld\n", clock() - begin);
		printf("  Comparisons:    %d\n", count);
		printf("  List is %s\n", check_list(&testdata_head, nums) ?
						 "sorted" :
						 "not sorted");
		test++;
	}

	return 0;
}
