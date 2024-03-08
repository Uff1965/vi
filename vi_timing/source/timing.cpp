// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi/timing.h>

#include <cassert>
#include <mutex>
#include <string>
#include <unordered_map>

struct item_t
{
	std::atomic<vi_tmTicks_t> ticks_{ 0 };
	std::size_t amount_{ 0 };
	std::size_t calls_cnt_{ 0 };
};

decltype(item_t::ticks_) s_dummy_ticks{ 0 };
std::mutex s_instance_guard;
std::unordered_map<std::string, item_t> s_instance;

std::atomic<vi_tmTicks_t>* vi_tmItem(const char* name, std::size_t amount)
{
	if (nullptr == name)
		return &s_dummy_ticks;

	std::scoped_lock lg_{ s_instance_guard };
	auto& item = s_instance[name];
	item.calls_cnt_++;
	item.amount_ += amount;
	return &item.ticks_;
}

int vi_tmResults(vi_tmLogRAW_t fn, void* data)
{
	auto result = -1;
	std::scoped_lock lg_{ s_instance_guard };
	for (const auto& [name, item] : s_instance)
	{
		assert(item.calls_cnt_ && item.amount_ >= item.calls_cnt_);
		if (!name.empty() && 0 == fn(name.c_str(), item.ticks_, item.amount_, item.calls_cnt_, data))
		{
			result = 0;
			break;
		}
	}
	return result;
}

void vi_tmInit(std::size_t n)
{
	std::scoped_lock lg_{ s_instance_guard };
	assert(s_instance.empty());
	s_instance.reserve(n);
}

void vi_tmClear(void)
{
	std::scoped_lock lg_{ s_instance_guard };
 	s_instance.clear();
	s_dummy_ticks = 0U;
}
