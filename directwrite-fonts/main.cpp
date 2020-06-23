#include <fstream>
#include <iomanip>

#include <nlohmann/json.hpp>
#include <cxxopts.hpp>

#include "directwrite-fonts.h"

using json = nlohmann::json;

int main(int argc, char** argv) {
	cxxopts::Options options("directwrite-fonts", "Obtains the list of fonts returned by DirectWrite.");
	options.add_options()
		("h,help", "Prints help information")
		("o,output_file", "Write the output to the given file path", cxxopts::value<std::string>()->default_value("prints to std out"), "FILE")
		("v,version", "Display version information", cxxopts::value<bool>()->default_value("false"));

	bool show_version_info = false;
	std::string output_file;
	try {
		auto result = options.parse(argc, argv);
		show_version_info = result["version"].as<bool>();
		if (result.count("output_file"))
			output_file = result["output_file"].as<std::string>();

		if (result.count("help")) {
			std::cout << options.help({ "" }) << std::endl;
			return 0;
		}

		if (show_version_info) {
			std::cout << "directwrite-fonts " << dwrite_fonts::kVersionInfo << std::endl;
			return 0;
		}
	}
	catch (const cxxopts::OptionException& e) {
		std::cout << "error parsing options: " << e.what() << std::endl;
		return 1;
	}

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

	if (!output_file.size()) {
		// No output file specified.
		std::cout << std::setw(2) << output << std::endl;
		return 0;
	}

	std::ofstream out_file(output_file);
	if (!out_file.is_open()) {
		std::cout << "Error: cannot open file named '" << output_file << "'.\n"
			<< "Aborting."
			<< std::endl;
		return 1;
	}
	out_file << std::setw(2) << output << std::endl;
	out_file.close();
	std::cout << "wrote to " << output_file << std::endl;
	return 0;
}

