#include "list.h"
#include "list_sort.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#define MAX_MERGE_PENDING 85

struct run {
	struct list_head *list;
	size_t len;
	struct run *next;
	struct run *prev;
};

int stk_size = 0;

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

static struct list_head *find_run(void *priv, struct list_head *list,
				  size_t *len, list_cmp_func_t cmp)
{
	*len = 1;
	struct list_head *next = list->next;

	if (unlikely(next == NULL))
		return NULL;

	if (cmp(priv, list, next) > 0) {
		/* decending run, also reverse the list */
		struct list_head *prev = NULL;
		do {
			(*len)++;
			list->next = prev;
			prev = list;
			list = next;
			next = list->next;
		} while (next && cmp(priv, list, next) > 0);
		list->next = prev;
	} else {
		do {
			(*len)++;
			list = next;
			next = list->next;
		} while (next && cmp(priv, list, next) <= 0);
		list->next = NULL;
	}

	return next;
}

static void merge_at(void *priv, list_cmp_func_t cmp, struct run *at)
{
	// printf("merge stk_size = %d\n", stk_size);
	// at[0] -> (*at)
	// at[1] -> (*(at->prev))
	(*at).list = merge(priv, cmp, (*at).list, (*(at->prev)).list);
	(*at).len += (*(at->prev)).len;
}

static struct run *merge_force_collapse(void *priv, list_cmp_func_t cmp,
					struct run *stk, struct run *tp)
{
	while (stk_size >= 3) {
		if ((*(tp->next->next)).len < (*tp).len) {
			merge_at(priv, cmp, &(*(tp->next->next)));
			tp->next->list = tp->list;
			tp->next->len = tp->len;
		} else {
			merge_at(priv, cmp, &(*(tp->next)));
		}
		// tp--;
		tp = tp->next;
		stk_size--;
	}
	return tp;
}

static struct run *merge_collapse(void *priv, list_cmp_func_t cmp,
				  struct run *stk, struct run *tp)
{
	int n;
	while ((n = stk_size) >= 2) {
		if ((n >= 3 && (*(tp->next->next)).len <=
				       (*(tp->next)).len + (*tp).len) ||
		    (n >= 4 &&
		     (*(tp->next->next->next)).len <=
			     (*(tp->next->next)).len + (*(tp->next)).len)) {
			if ((*(tp->next->next)).len < (*tp).len) {
				merge_at(priv, cmp, &(*(tp->next->next)));
				tp->next->list = tp->list;
				tp->next->len = tp->len;
			} else {
				merge_at(priv, cmp, &(*(tp->next)));
			}
		} else if ((*(tp->next)).len <= (*tp).len) {
			merge_at(priv, cmp, &(*(tp->next)));
		} else {
			break;
		}
		// tp--;
		struct run *tmp = tp;
		tp = tp->next;
		tp->prev = NULL;
		free(tmp);
		stk_size--;
	}

	return tp;
}

void inplace_timsort(void *priv, struct list_head *head, list_cmp_func_t cmp)
{
	struct list_head *list = head->next;
	// struct run stk[MAX_MERGE_PENDING], *tp = stk - 1;
	struct run *stk = NULL;
	struct run *tp = NULL;

	stk_size = 0;

	if (head == head->prev)
		return;

	/* Convert to a null-terminated singly-linked list. */
	head->prev->next = NULL;
	do {
		// tp++;
		/* Find next run */
		struct run *new_tp = malloc(sizeof(struct run));
		if (!new_tp) {
			printf("malloc failed\n");
			exit(1);
		}
		if (tp)
			tp->prev = new_tp;
		new_tp->next = tp;
		new_tp->prev = NULL;
		tp = new_tp;
		stk_size++;

		tp->list = list;
		list = find_run(priv, list, &tp->len, cmp);
		tp = merge_collapse(priv, cmp, stk, tp);
	} while (list);

	/* End of input; merge together all the runs. */
	tp = merge_force_collapse(priv, cmp, stk, tp);

	/* The final merge; rebuild prev links */
	// if (tp > stk) {
	// 	merge_final(priv, cmp, head, stk[0].list, stk[1].list);
	// } else {
	// 	build_prev_link(head, head, stk->list);
	// }
	struct run *stk0 = tp;
	while (stk0 && stk0->next)
		stk0 = stk0->next;
	if (stk_size > 1) {
		merge_final(priv, cmp, head, (*stk0).list,
			    (*(stk0->prev)).list);
	} else {
		build_prev_link(head, head, stk0->list);
	}
}
