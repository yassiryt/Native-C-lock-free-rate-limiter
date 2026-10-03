#include "limiter.hpp"

static size_t lru_head = SIZE_MAX;
static size_t lru_tail = SIZE_MAX;
static size_t lru_next[TABLE_SIZE];

void init_lru_links()
{
	size_t i = 0;
	while (i < TABLE_SIZE) {
		lru_next[i] = i + 1;
		i++;
	}
	lru_next[TABLE_SIZE - 1] = SIZE_MAX;
	lru_head = 0;
	lru_tail = TABLE_SIZE - 1;
}

void move_lru_to_tail(size_t idx)
{
	if (lru_next[idx] == SIZE_MAX && idx == lru_tail) return;

	size_t prev = SIZE_MAX;
	size_t cur = lru_head;
	while (cur != SIZE_MAX && cur != idx) {
		prev = cur;
		cur = lru_next[cur];
	}

	if (prev != SIZE_MAX)
		lru_next[prev] = lru_next[idx];
	else
		lru_head = lru_next[idx];

	lru_next[lru_tail] = idx;
	lru_tail = idx;
	lru_next[idx] = SIZE_MAX;
}

size_t get_lru_head()
{
	return lru_head;
}

size_t get_lru_tail()
{
	return lru_tail;
}