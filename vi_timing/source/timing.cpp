// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi/timing.h>

#include <cassert>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#define UNORDERED
#ifdef UNORDERED
#	include <unordered_map>
#	define CONTAINER std::unordered_map<std::string, item_t>
#else
#	include <map>
#	define CONTAINER std::map<std::string, item_t, std::less<>>
#endif

using namespace std::literals;
namespace ch = std::chrono;

struct item_t
{
	std::atomic<vi_tmTicks_t> time_total_{ 0 };
	std::size_t amount_{ 0 };
	std::size_t calls_cnt_{ 0 };

	static inline std::mutex s_instance_guard;
	static inline CONTAINER s_instance;
	static inline decltype(time_total_) s_dummy{ 0 };
};

void vi_tmWarming(int all, std::size_t ms)
{
	if (0 != ms)
	{
		std::atomic_bool done = false; // It must be defined before 'threads'!!!
		auto load = [&done] {while (!done) {/**/}}; //-V776

		const auto hwcnt = std::thread::hardware_concurrency();
		std::vector<std::thread> threads((0 != all && 1 < hwcnt) ? hwcnt - 1 : 0);
		for (auto& t : threads)
		{
			t = std::thread{ load };
		}

		for (const auto stop = ch::steady_clock::now() + ch::milliseconds{ ms }; ch::steady_clock::now() < stop;)
		{/*The thread is fully loaded.*/}

		done = true;

		for (auto& t : threads)
		{
			t.join();
		}
	}
}

std::atomic<vi_tmTicks_t>* vi_tmItem(const char* name, std::size_t amount)
{
	auto result = &item_t::s_dummy;
	if (name)
	{
		std::scoped_lock lg_{ item_t::s_instance_guard };
		auto& item = item_t::s_instance[name];
		item.calls_cnt_++;
		item.amount_ += amount;
		result = &item.time_total_;
	}
	return result;
}

int vi_tmResults(vi_tmLogRAW_t fn, void* data)
{
	auto result = -1;
	std::scoped_lock lg_{ item_t::s_instance_guard };
	for (const auto& [name, item] : item_t::s_instance)
	{
		assert(item.calls_cnt_ && item.amount_ >= item.calls_cnt_);
		if (!name.empty() && 0 == fn(name.c_str(), item.time_total_, item.amount_, item.calls_cnt_, data))
		{
			result = 0;
			break;
		}
	}
	return result;
}

void vi_tmClear(void)
{
	std::scoped_lock lg_{ item_t::s_instance_guard };
 	item_t::s_instance.clear();
	item_t::s_dummy = 0U;
}
