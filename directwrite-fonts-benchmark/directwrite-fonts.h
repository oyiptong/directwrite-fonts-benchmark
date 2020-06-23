#include <iostream>
#include <optional>
#include <vector>
#include <codecvt>
#include <sstream>
#include <dwrite.h>
#include <windows.h>
#include <time.h>
#include <wrl/client.h>

namespace dwrite_fonts {

	const std::string kVersionInfo = "1.0.0";

	constexpr HRESULT kErrorExtractingLocalizedStringsFailed =

		MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xD102);

	constexpr HRESULT kErrorNoFullNameOrPostScriptName =

		MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xD103);

	constexpr HRESULT kErrorNoFamilyName =

		MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xD104);

	struct FontMetadata {
		std::string postscript_name;
		std::string full_name;
		std::string family;
	};

	struct FamilyDataResult {
		std::vector<FontMetadata> fonts;
		HRESULT exit_hresult{ S_OK };
	};

	std::string utf8Encode(const std::wstring& wstr);
	std::wstring utf8Decode(const std::string& str);
	IDWriteFactory* createFactory();
	std::optional<std::string> GetLocalizedString(IDWriteLocalizedStrings* names, const std::string& locale);
	std::optional<std::string> GetNativeString(IDWriteLocalizedStrings* names);
	FamilyDataResult ExtractNamesFromFamily(int id, Microsoft::WRL::ComPtr<IDWriteFontCollection> collection, uint32_t family_index);
} // namespace directwriteFonts
