#include "directwrite-fonts.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
	IDWriteFactory* factory = dwrite_fonts::createFactory();
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
	std::vector<dwrite_fonts::FamilyDataResult> results;
	const UINT32 family_count = collection->GetFontFamilyCount();
	for (UINT32 family_index = 0; family_index < family_count; ++family_index) {
		auto result = dwrite_fonts::ExtractNamesFromFamily(0, collection, family_index);
		font_count += result.fonts.size();
		results.push_back(std::move(result));
	}

	json fonts;
	for (auto result : results) {
		for (auto f : result.fonts) {
			json font;
			font["postscript_name"] = f.postscript_name;
			font["full_name"] = f.full_name;
			font["family"] = f.family;
			fonts.push_back(font);
		}
	}

	json output;
	output["fonts"] = fonts;
	output["stats"]["num_fonts"] = font_count;

	std::cout << output.dump(4) << std::endl;

	return 0;
}

