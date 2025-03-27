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

static void free_list(const struct list_head *head)
{
	element_t *entry, *safe;
	list_for_each_entry_safe(entry, safe, head, list) {
		list_del(&entry->list);
		free(entry);
	}
}

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

int main(int argc, char **argv)
{
	srand(atoi(argv[1]));
	const char filename[] = "data.txt";
	FILE *fp = fopen(filename, "w");

	int start = 5;
	int end = 5;

	for (int i = start; i <= end; i += 100000) {
		fprintf(fp, "%d ", i);

		struct list_head sample_head, warmdata_head, testdata_head;
		element_t *samples, *warmdata, *testdata;
		int count;
		int nums = i;

		test_t tests[] = { { list_sort, "list_sort" },
				   { list_sort_old, "list_sort_old" },
				   { shiverssort, "shiverssort" },
				   { timsort, "timsort" },
				   { NULL, NULL } },
		       *test = tests;

		INIT_LIST_HEAD(&sample_head);

		samples = malloc(sizeof(*samples) * i);
		warmdata = malloc(sizeof(*warmdata) * i);
		testdata = malloc(sizeof(*testdata) * i);

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
			count = 0;
			struct timespec tstart = { 0, 0 }, tend = { 0, 0 };
			clock_gettime(CLOCK_MONOTONIC, &tstart);
			test->fp(&count, &testdata_head, compare);
			clock_gettime(CLOCK_MONOTONIC, &tend);
			long long duration =
				(1e9 * tend.tv_sec + tend.tv_nsec) -
				(1e9 * tstart.tv_sec + tstart.tv_nsec);
			printf("  Elapsed time:   %lld\n", duration);
			printf("  Comparisons:    %d\n", count);
			printf("  List is %s\n",
			       check_list(&testdata_head, nums) ? "sorted" :
								  "not sorted");
			// fprintf(fp, "%lld ", duration);
			fprintf(fp, "%d ", count);
			test++;
		}
		fprintf(fp, "\n");
		// free_list(&sample_head);
		// free_list(&warmdata_head);
		// free_list(&testdata_head);
	}
	fclose(fp);

	return 0;
}
