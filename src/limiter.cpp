#include "limiter.hpp"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <functional>
#include <string>
#include <algorithm>

#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

static Bucket*	table = nullptr;

bool	init_limiter(int32_t max_tokens, uint64_t refill_ms, const char** err_msg)
{
	int		shm_fd;
	size_t	i;

	if (table)
		return true;

	shm_fd = shm_open("/node_rate_limiter_shm", O_CREAT | O_RDWR, 0666);
	if (shm_fd < 0)
	{
		*err_msg = "shm_open failed";
		return false;
	}

	if (ftruncate(shm_fd, TABLE_SIZE * sizeof(Bucket)) < 0)
	{
		*err_msg = "ftruncate failed";
		close(shm_fd);
		return false;
	}

	table = (Bucket*)mmap(nullptr, TABLE_SIZE * sizeof(Bucket), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	close(shm_fd);

	if (table == MAP_FAILED)
	{
		*err_msg = "mmap failed";
		return false;
	}

	i = 0;
	while (i < TABLE_SIZE)
	{
		table[i].ip_hash.store(0, std::memory_order_relaxed);
		table[i].toks.store(max_tokens, std::memory_order_relaxed);
		table[i].ts.store(0, std::memory_order_relaxed);
		i++;
	}

	return true;
}

bool	consume_token(const char* ip_str, int32_t max_tokens, uint64_t refill_ms)
{
	uint64_t		ip_h;
	size_t			idx;
	size_t			attempts;
	Bucket*			b;
	uint64_t		stored_hash;
	uint64_t		expected;
	uint64_t		now;
	uint64_t		last;
	int32_t			toks;
	int32_t			add;
	int32_t			updated;

	if (!table || !ip_str)
		return false;

	ip_h = std::hash<std::string>{}(ip_str);
	idx = ip_h % TABLE_SIZE;
	attempts = 0;

	while (attempts < 100)
	{
		b = &table[idx];
		stored_hash = b->ip_hash.load(std::memory_order_acquire);

		if (stored_hash == 0)
		{
			expected = 0;
			if (b->ip_hash.compare_exchange_strong(expected, ip_h, std::memory_order_acq_rel))
				break;
			continue;
		}

		if (stored_hash == ip_h)
			break;

		idx = (idx + 1) % TABLE_SIZE;
		attempts++;
	}

	b = &table[idx];
	now = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()
	).count();

	last = b->ts.load(std::memory_order_acquire);
	toks = b->toks.load(std::memory_order_acquire);
	add = (now - last) / refill_ms;

	if (add > 0)
	{
		updated = std::min(max_tokens, toks + add);
		if (b->ts.compare_exchange_strong(last, now))
		{
			b->toks.store(updated, std::memory_order_release);
			toks = updated;
		}
	}

	while (toks > 0)
	{
		if (b->toks.compare_exchange_weak(toks, toks - 1, std::memory_order_acq_rel))
			return true;
	}

	return false;
}