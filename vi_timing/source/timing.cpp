// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi/timing.h>

#include <cassert>
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

namespace
{
	struct item_t
	{
		std::atomic<vi_tmTicks_t> time_total_{ 0 };
		std::size_t amount_{ 0 };
		std::size_t calls_cnt_{ 0 };

		static inline std::mutex s_instance_guard;
		static inline std::map<std::string, item_t, std::less<>> s_instance;
		static inline decltype(time_total_) s_dummy{ 0 };
	};
}

void vi_tmWarming(int all, std::size_t ms)
{
	if (0 != ms)
	{
		const auto hwcnt = std::thread::hardware_concurrency();
		std::atomic_bool done = false; // It must be defined before 'threads'!!!
		std::vector<std::thread> threads((all != 0 && hwcnt > 1) ? hwcnt - 1 : 0);

		auto load = [&done] {while (!done) {/**/ } }; //-V776
		for (auto& t : threads)
		{
			t = std::thread{ load };
		}

		namespace ch = std::chrono;
		for (const auto stop = ch::steady_clock::now() + ch::milliseconds{ ms }; ch::steady_clock::now() < stop;)
		{/**/}

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
		std::scoped_lock lg(item_t::s_instance_guard);

		auto& ret = item_t::s_instance[name];
		ret.calls_cnt_++;
		ret.amount_ += amount;
		result = &ret.time_total_;
	}

	return result;
}

void vi_tmClear(void)
{
	std::scoped_lock lg(item_t::s_instance_guard);
	item_t::s_instance.clear();
	item_t::s_dummy = 0U;
}

int vi_tmResults(vi_tmLogRAW fn, void* data)
{
	std::scoped_lock lg(item_t::s_instance_guard);

	auto result = -1;
	for (const auto& [name, inst] : item_t::s_instance)
	{
		assert(inst.calls_cnt_ && inst.amount_ >= inst.calls_cnt_);
		if (0 == fn(name.c_str(), inst.time_total_, inst.amount_, inst.calls_cnt_, data))
		{
			result = 0;
			break;
		}
	}

	return result;
}
