#include <benchmark/benchmark.h>
#include "directwrite-fonts.h"

#include "ctpl_stl.h"

namespace dwrite_fonts {
	int run(int threads = 1) {

		int num_threads = max(threads, 1);
		ctpl::thread_pool p(num_threads);

		IDWriteFactory* factory = createFactory();
		if (!factory) {
			return 1;
		}

		Microsoft::WRL::ComPtr<IDWriteFontCollection> collection;
		HRESULT hr = factory->GetSystemFontCollection(&collection);
		if (FAILED(hr)) {
			std::cout << "Failed to get system fonts." << std::endl;
			return 1;
		}

		int font_count = 0;
		std::vector<std::future<FamilyDataResult>> futures;
		const UINT32 family_count = collection->GetFontFamilyCount();
		for (UINT32 family_index = 0; family_index < family_count; ++family_index) {
			futures.push_back(p.push(ExtractNamesFromFamily, collection, family_index));
		}

		for (std::vector<std::future<FamilyDataResult>>::iterator it = futures.begin(); it != futures.end(); ++it) {
			it->wait();
			auto result = it->get();
			font_count += result.fonts.size();
		}

		return 0;
	}
} // namespace dwrite_fonts

static void BM_Run(benchmark::State& state) {
	for (auto _ : state) {
		int result = dwrite_fonts::run(state.range(0));
		if (result) {
			state.SkipWithError("Error!");
		}
	}
}
BENCHMARK(BM_Run)->RangeMultiplier(2)->Range(1, 72)->Unit(benchmark::kMillisecond);;

BENCHMARK_MAIN();
