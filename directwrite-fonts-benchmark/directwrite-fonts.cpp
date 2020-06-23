#include "directwrite-fonts.h"


namespace dwrite_fonts {

	// Convert a wide string to a string.
	std::string utf8Encode(const std::wstring& wstr) {
		if (wstr.empty())
			return std::string();
		// win32 API to convert a wstring to string: https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
		return strTo;
	}

	// Convert a string to a wide string.
	std::wstring utf8Decode(const std::string& str) {
		if (str.empty())
			return std::wstring();
		// win32 API to convert a string to a wstring: https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}

	IDWriteFactory* createFactory() {
		IDWriteFactory* factory = NULL;
		HRESULT hr =
			DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
				reinterpret_cast<IUnknown * *>(&factory));

		if (FAILED(hr)) {
			std::cout << "Failed to create factory." << std::endl;
			return NULL;
		}
		return factory;
	}


	std::optional<std::string> GetLocalizedString(IDWriteLocalizedStrings* names, const std::string& locale) {
		std::wstring locale_wide = utf8Decode(locale);

		// If locale is empty, index 0 will be used. Otherwise, the locale name must
		// be found and must exist.
		UINT32 index = 0;
		BOOL exists = false;
		if (!locale.empty() &&
			(FAILED(names->FindLocaleName(locale_wide.c_str(), &index, &exists)) ||
				!exists)) {
			return std::nullopt;
		}

		// Get the string length.
		UINT32 length = 0;
		if (FAILED(names->GetStringLength(index, &length)))
			return std::nullopt;

		// The output buffer length needs to be one larger to receive the NUL
		// character.
		std::wstring buffer;
		buffer.resize(length + 1);
		if (FAILED(names->GetString(index, &buffer[0], buffer.size())))
			return std::nullopt;

		// Shrink the string to fit the actual length.
		buffer.resize(length);

		return utf8Encode(buffer);
	}


	std::optional<std::string> GetNativeString(IDWriteLocalizedStrings* names) {
		auto output = GetLocalizedString(names, "");
		if (output)
			return output;

		return GetLocalizedString(names, "en-us");
	}

	FamilyDataResult ExtractNamesFromFamily(
		int id,
		Microsoft::WRL::ComPtr<IDWriteFontCollection> collection,
		uint32_t family_index) {

		FamilyDataResult family_result;

		Microsoft::WRL::ComPtr<IDWriteFontFamily> family;
		Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> family_names;

		HRESULT hr = collection->GetFontFamily(family_index, &family);
		if (FAILED(hr)) {
			family_result.exit_hresult = hr;
			return family_result;
		}

		hr = family->GetFamilyNames(&family_names);
		if (FAILED(hr)) {
			family_result.exit_hresult = hr;
			return family_result;
		}
		std::optional<std::string> native_family_name =
			GetNativeString(family_names.Get());
		if (!native_family_name) {
			family_result.exit_hresult = kErrorNoFamilyName;
			return family_result;
		}

		std::optional<std::string> localized_family_name =
			GetLocalizedString(family_names.Get(), "en-us");
		if (!localized_family_name)
			localized_family_name = native_family_name;

		UINT32 font_count = family->GetFontCount();
		for (UINT32 font_index = 0; font_index < font_count; ++font_index) {
			BOOL exists = false;

			Microsoft::WRL::ComPtr<IDWriteFont> font;

			hr = family->GetFont(font_index, &font);

			if (FAILED(hr)) {
				family_result.exit_hresult = hr;
				return family_result;
			}

			// Skip this font if it's a simulation.
			if (font->GetSimulations() != DWRITE_FONT_SIMULATIONS_NONE)
				continue;

			Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> postscript_name;
			Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> full_name;

			hr = font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME, &postscript_name, &exists);
			if (FAILED(hr)) {
				family_result.exit_hresult = hr;
				return family_result;
			}
			if (!exists) {
				family_result.exit_hresult = kErrorNoFullNameOrPostScriptName;
				return family_result;
			}

			std::optional<std::string> native_postscript_name =
				GetNativeString(postscript_name.Get());
			if (!native_postscript_name) {
				family_result.exit_hresult = kErrorNoFullNameOrPostScriptName;
				return family_result;
			}

			hr = font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_FULL_NAME, &full_name, &exists);
			if (FAILED(hr)) {
				family_result.exit_hresult = hr;
				return family_result;
			}
			if (!exists) {
				family_result.exit_hresult = kErrorNoFullNameOrPostScriptName;
				return family_result;
			}

			std::optional<std::string> localized_full_name = GetLocalizedString(full_name.Get(), "en-us");
			if (!localized_full_name)
				localized_full_name = GetLocalizedString(full_name.Get(), "");
			if (!localized_full_name)
				localized_full_name = native_postscript_name;

			FontMetadata unique_font;
			unique_font.postscript_name = native_postscript_name.value();
			unique_font.full_name = localized_full_name.value();
			unique_font.family = localized_family_name.value();

			family_result.fonts.push_back(std::move(unique_font));
		}

		return family_result;
	}

} // namespace dwrite_fonts

