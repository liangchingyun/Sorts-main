#include "list.h"
#include "list_sort.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

size_t get_run_size(struct list_head *run_head)
{
	if (run_head == NULL || run_head->next == NULL ||
	    run_head->next->prev == NULL) {
		return 0;
	}
	return (size_t)(run_head->next->prev);
}

struct pair {
	struct list_head *head;
	struct list_head *next;
};

size_t stk_size;

static struct list_head *merge(void *priv, list_cmp_func_t cmp,
			       struct list_head *a, struct list_head *b)
{
	struct list_head *head = NULL, **tail = &head;

	for (;;) {
		/* if equal, take 'a' -- important for sort stability */
		if (cmp(priv, a, b) <= 0) {
			*tail = a;
			tail = &a->next;
			a = a->next;
			if (!a) {
				*tail = b;
				break;
			}
		} else {
			*tail = b;
			tail = &b->next;
			b = b->next;
			if (!b) {
				*tail = a;
				break;
			}
		}
	}
	return head;
}

static void build_prev_link(struct list_head *head, struct list_head *tail,
			    struct list_head *list)
{
	tail->next = list;
	do {
		list->prev = tail;
		tail = list;
		list = list->next;
	} while (list);

	/* The final links to make a circular doubly-linked list */
	tail->next = head;
	head->prev = tail;
}

static void merge_final(void *priv, list_cmp_func_t cmp, struct list_head *head,
			struct list_head *a, struct list_head *b)
{
	struct list_head *tail = head;

	for (;;) {
		/* if equal, take 'a' -- important for sort stability */
		if (cmp(priv, a, b) <= 0) {
			tail->next = a;
			a->prev = tail;
			tail = a;
			a = a->next;
			if (!a)
				break;
		} else {
			tail->next = b;
			b->prev = tail;
			tail = b;
			b = b->next;
			if (!b) {
				b = a;
				break;
			}
		}
	}

	/* Finish linking remainder of list b on to tail */
	build_prev_link(head, tail, b);
}

static struct pair find_run(void *priv, struct list_head *list,
			    list_cmp_func_t cmp)
{
	size_t len = 1;
	struct list_head *next = list->next;
	struct list_head *head = list;
	struct pair result;

	if (unlikely(next == NULL)) {
		result.head = head;
		result.next = next;
		return result;
	}

	if (cmp(priv, list, next) > 0) {
		/* decending run, also reverse the list */
		struct list_head *prev = NULL;
		do {
			len++;
			list->next = prev;
			prev = list;
			list = next;
			next = list->next;
			head = list;
		} while (next && cmp(priv, list, next) > 0);
		list->next = prev;
	} else {
		do {
			len++;
			list = next;
			next = list->next;
		} while (next && cmp(priv, list, next) <= 0);
		list->next = NULL;
	}
	head->prev = NULL;
	head->next->prev = (struct list_head *)len;
	result.head = head;
	result.next = next;
	return result;
}

static struct list_head *merge_at(void *priv, list_cmp_func_t cmp,
				  struct list_head *at)
{
	size_t len = get_run_size(at) + get_run_size(at->prev);
	struct list_head *prev = at->prev->prev;
	struct list_head *list = merge(priv, cmp, at->prev, at);
	list->prev = prev;
	list->next->prev = (struct list_head *)len;
	--stk_size;
	return list;
}

static struct list_head *merge_force_collapse(void *priv, list_cmp_func_t cmp,
					      struct list_head *tp)
{
	while (stk_size >= 3) {
		if (get_run_size(tp->prev->prev) < get_run_size(tp)) {
			tp->prev = merge_at(priv, cmp, tp->prev);
		} else {
			tp = merge_at(priv, cmp, tp);
		}
	}
	return tp;
}

static struct list_head *merge_collapse(void *priv, list_cmp_func_t cmp,
					struct list_head *tp)
{
	int n;
	while ((n = stk_size) >= 2) {
		if ((n >= 3 &&
		     get_run_size(tp->prev->prev) <=
			     get_run_size(tp->prev) + get_run_size(tp)) ||
		    (n >= 4 && get_run_size(tp->prev->prev->prev) <=
				       get_run_size(tp->prev->prev) +
					       get_run_size(tp->prev))) {
			if (get_run_size(tp->prev->prev) < get_run_size(tp)) {
				tp->prev = merge_at(priv, cmp, tp->prev);
			} else {
				tp = merge_at(priv, cmp, tp);
			}
		} else if (get_run_size(tp->prev) <= get_run_size(tp)) {
			tp = merge_at(priv, cmp, tp);
		} else {
			break;
		}
	}

	return tp;
}

void inplace_timsort(void *priv, struct list_head *head, list_cmp_func_t cmp)
{
	stk_size = 0;

	struct list_head *list = head->next;
	struct list_head *tp = NULL;

	if (head == head->prev)
		return;

	/* Convert to a null-terminated singly-linked list. */
	head->prev->next = NULL;

	do {
		/* Find next run */
		struct pair result = find_run(priv, list, cmp);
		result.head->prev = tp;
		tp = result.head;
		list = result.next;
		stk_size++;
		tp = merge_collapse(priv, cmp, tp);
	} while (list);

	/* End of input; merge together all the runs. */
	tp = merge_force_collapse(priv, cmp, tp);

	/* The final merge; rebuild prev links */
	struct list_head *stk0 = tp;
	struct list_head *stk1 = stk0->prev;
	while (stk1 && stk1->prev) {
		stk0 = stk0->prev;
		stk1 = stk1->prev;
	}
	if (stk_size > 1) {
		merge_final(priv, cmp, head, stk0, stk1);
	} else {
		build_prev_link(head, head, stk0);
	}
}
