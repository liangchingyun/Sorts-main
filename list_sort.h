/* SPDX-License-Identifier: GPL-2.0 */
#pragma once

struct list_head;

typedef int (*list_cmp_func_t)(void *, const struct list_head *,
			       const struct list_head *);

void list_sort(void *priv, struct list_head *head, list_cmp_func_t cmp);
void shiverssort(void *priv, struct list_head *head, list_cmp_func_t cmp);
void timsort(void *priv, struct list_head *head, list_cmp_func_t cmp);
void list_sort_old(void *priv, struct list_head *head, list_cmp_func_t cmp);
