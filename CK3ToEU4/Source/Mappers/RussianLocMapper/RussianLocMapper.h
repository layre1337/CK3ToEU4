#ifndef RUSSIAN_LOC_MAPPER
#define RUSSIAN_LOC_MAPPER
#include <map>
#include <string>

namespace mappers
{
// EU4 has no Cyrillic glyphs. The unofficial Russian localization mod re-draws a set of "wrong" characters as Cyrillic letters.
// This class converts real Cyrillic text into those characters (encode), or into plain Latin (transliterate).
class RussianLocMapper
{
  public:
	RussianLocMapper() = default;
	// Reads configurables/russian_loc/alphabet.txt: line 1 = 33 glyphs for upper case A..Ya, line 2 = 33 glyphs for lower case a..ya, space separated.
	void loadAlphabet(const std::string& path = "configurables/russian_loc/alphabet.txt");
	void loadAlphabet(std::istream& theStream);

	// When set, people and dynasty names are taken from the Russian CK3 localization and transliterated (Yakov -> Jakov).
	inline static bool transliterateNames = false;
	[[nodiscard]] static std::string pickName(const std::string& english, const std::string& russian);

	[[nodiscard]] bool isLoaded() const { return !glyphs.empty(); }
	// Cyrillic -> glyphs of the Russian loc mod. Anything the alphabet doesn't cover (e.g. Ukrainian letters) is transliterated.
	[[nodiscard]] std::string encode(const std::string& utf8) const;
	// Cyrillic -> Latin (Yakov -> Jakov). Non-Cyrillic text is left untouched.
	[[nodiscard]] static std::string transliterate(const std::string& utf8);
	[[nodiscard]] static bool containsCyrillic(const std::string& utf8);

  private:
	std::map<char32_t, std::string> glyphs; // Cyrillic code point -> UTF-8 of the substitute glyph
};
} // namespace mappers

#endif // RUSSIAN_LOC_MAPPER
