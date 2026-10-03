#ifndef LIMITER_HPP
# define LIMITER_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>

struct alignas(64)	Bucket{
	std::atomic<uint64_t>	ip_hash;
	std::atomic<uint64_t>	ts;
	std::atomic<int32_t>	toks;
};

const size_t	TABLE_SIZE = 65536;

bool	init_limiter(int32_t max_tokens, uint64_t refill_ms, const char** err_msg);
bool	consume_token(const char* ip_str, int32_t max_tokens, uint64_t refill_ms);
void	cleanup_limiter();

#endif