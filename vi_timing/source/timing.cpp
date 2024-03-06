// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi/timing.h>

#include <cassert>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

namespace
{
	class item_t
	{
	public:
		using container_t = std::unordered_map<std::string, item_t>;
	protected:
		static inline std::mutex s_instance_guard;
		static inline container_t s_instance;
	public:
		std::atomic<vi_tmTicks_t> time_total_{ 0 };
		std::size_t amount_{ 0 };
		std::size_t calls_cnt_{ 0 };

		static inline decltype(time_total_) s_dummy{ 0 };

		class instance_proxy
		{
			std::scoped_lock<std::mutex> lg_{ item_t::s_instance_guard };
		public:
			operator container_t& () const noexcept { return s_instance; }
		};
		static instance_proxy instance() { return {}; }
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
	if (!name)
		return &item_t::s_dummy;

	item_t::container_t& inst = item_t::instance();

	auto& ret = inst[name];
	ret.calls_cnt_++;
	ret.amount_ += amount;
	return &ret.time_total_;
}

int vi_tmResults(vi_tmLogRAW fn, void* data)
{
	item_t::container_t &inst = item_t::instance();

	auto result = -1;
	for (const auto& [name, inst] : inst)
	{
		assert(inst.calls_cnt_ && inst.amount_ >= inst.calls_cnt_);
		if (!name.empty() && 0 == fn(name.c_str(), inst.time_total_, inst.amount_, inst.calls_cnt_, data))
		{
			result = 0;
			break;
		}
	}

	return result;
}

//void vi_tmClear(void)
//{
//	item_t::container_t& inst = item_t::instance();
//	inst.clear();
//	item_t::s_dummy = 0U;
//}
