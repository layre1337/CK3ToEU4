#include "RussianLocMapper.h"
#include "Log.h"
#include <cctype>
#include <fstream>
#include <sstream>
#include <vector>

namespace
{
std::u32string decodeUtf8(const std::string& in)
{
	std::u32string out;
	for (size_t i = 0; i < in.size();)
	{
		const auto c = static_cast<unsigned char>(in[i]);
		char32_t cp = c;
		size_t len = 1;
		if (c >= 0xF0 && c < 0xF8)
			cp = c & 0x07, len = 4;
		else if (c >= 0xE0)
			cp = c & 0x0F, len = 3;
		else if (c >= 0xC0)
			cp = c & 0x1F, len = 2;
		if (len > 1 && i + len > in.size())
		{
			out.push_back(c); // truncated sequence, keep byte as is
			++i;
			continue;
		}
		for (size_t k = 1; k < len; ++k)
			cp = (cp << 6) | (static_cast<unsigned char>(in[i + k]) & 0x3F);
		out.push_back(cp);
		i += len;
	}
	return out;
}

std::string encodeUtf8(const char32_t cp)
{
	std::string out;
	if (cp < 0x80)
		out += static_cast<char>(cp);
	else if (cp < 0x800)
	{
		out += static_cast<char>(0xC0 | (cp >> 6));
		out += static_cast<char>(0x80 | (cp & 0x3F));
	}
	else if (cp < 0x10000)
	{
		out += static_cast<char>(0xE0 | (cp >> 12));
		out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		out += static_cast<char>(0x80 | (cp & 0x3F));
	}
	else
	{
		out += static_cast<char>(0xF0 | (cp >> 18));
		out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
		out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		out += static_cast<char>(0x80 | (cp & 0x3F));
	}
	return out;
}

bool isCyrillic(const char32_t cp)
{
	return cp >= 0x0400 && cp <= 0x04FF;
}

// Order matches the alphabet file: upper/lower case A..Ya, with Yo after Ye
const std::u32string upperAlphabet = U"\u0410\u0411\u0412\u0413\u0414\u0415\u0401\u0416\u0417\u0418\u0419\u041A\u041B\u041C\u041D\u041E\u041F\u0420\u0421\u0422\u0423\u0424\u0425\u0426\u0427\u0428\u0429\u042A\u042B\u042C\u042D\u042E\u042F";
const std::u32string lowerAlphabet = U"\u0430\u0431\u0432\u0433\u0434\u0435\u0451\u0436\u0437\u0438\u0439\u043A\u043B\u043C\u043D\u043E\u043F\u0440\u0441\u0442\u0443\u0444\u0445\u0446\u0447\u0448\u0449\u044A\u044B\u044C\u044D\u044E\u044F";

// Latin form of a lower case letter; the upper case form gets its first letter capitalised.
const std::map<char32_t, std::string> latin = {{U'\u0430', "a"},
	 {U'\u0431', "b"},
	 {U'\u0432', "v"},
	 {U'\u0433', "g"},
	 {U'\u0434', "d"},
	 {U'\u0435', "e"},
	 {U'\u0451', "e"},
	 {U'\u0436', "zh"},
	 {U'\u0437', "z"},
	 {U'\u0438', "i"},
	 {U'\u0439', "j"},
	 {U'\u043A', "k"},
	 {U'\u043B', "l"},
	 {U'\u043C', "m"},
	 {U'\u043D', "n"},
	 {U'\u043E', "o"},
	 {U'\u043F', "p"},
	 {U'\u0440', "r"},
	 {U'\u0441', "s"},
	 {U'\u0442', "t"},
	 {U'\u0443', "u"},
	 {U'\u0444', "f"},
	 {U'\u0445', "h"},
	 {U'\u0446', "c"},
	 {U'\u0447', "ch"},
	 {U'\u0448', "sh"},
	 {U'\u0449', "sch"},
	 {U'\u044A', ""},
	 {U'\u044B', "y"},
	 {U'\u044C', ""},
	 {U'\u044D', "e"},
	 {U'\u044E', "ju"},
	 {U'\u044F', "ja"},
	 // Ukrainian / Belarusian
	 {U'\u0456', "i"},
	 {U'\u0457', "ji"},
	 {U'\u0454', "je"},
	 {U'\u0491', "g"},
	 {U'\u045E', "u"}};

char32_t toLower(const char32_t cp)
{
	if (cp >= 0x0410 && cp <= 0x042F)
		return cp + 0x20;
	if (cp == 0x0401)
		return 0x0451; // (Cyrillic letter, see code point above)
	if (cp == 0x0406)
		return 0x0456; // (Cyrillic letter, see code point above)
	if (cp == 0x0407)
		return 0x0457; // (Cyrillic letter, see code point above)
	if (cp == 0x0404)
		return 0x0454; // (Cyrillic letter, see code point above)
	if (cp == 0x0490)
		return 0x0491; // (Cyrillic letter, see code point above)
	if (cp == 0x040E)
		return 0x045E; // (Cyrillic letter, see code point above)
	return cp;
}

std::string latinFor(const char32_t cp)
{
	const auto lower = toLower(cp);
	const auto itr = latin.find(lower);
	if (itr == latin.end())
		return "?";
	auto out = itr->second;
	if (lower != cp && !out.empty())
		out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
	return out;
}
} // namespace

void mappers::RussianLocMapper::loadAlphabet(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		Log(LogLevel::Warning) << "Russian alphabet file " << path << " not found. Russian localization will be transliterated to Latin instead.";
		return;
	}
	loadAlphabet(file);
}

void mappers::RussianLocMapper::loadAlphabet(std::istream& theStream)
{
	glyphs.clear();
	std::vector<std::vector<std::string>> rows;
	std::string line;
	while (std::getline(theStream, line) && rows.size() < 2)
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty())
			continue;
		std::vector<std::string> tokens;
		std::stringstream ss(line);
		std::string token;
		while (std::getline(ss, token, ' ')) // glyphs may be control characters, so only plain spaces separate them
			if (!token.empty())
				tokens.push_back(token);
		rows.push_back(tokens);
	}
	if (rows.size() != 2 || rows[0].size() != upperAlphabet.size() || rows[1].size() != lowerAlphabet.size())
	{
		Log(LogLevel::Error) << "Russian alphabet file must have two lines of " << upperAlphabet.size() << " glyphs (upper case, lower case). Got "
									<< (rows.size() > 0 ? rows[0].size() : 0) << " and " << (rows.size() > 1 ? rows[1].size() : 0)
									<< ". Russian localization will be transliterated to Latin instead.";
		return;
	}
	for (size_t i = 0; i < upperAlphabet.size(); ++i)
	{
		glyphs[upperAlphabet[i]] = rows[0][i];
		glyphs[lowerAlphabet[i]] = rows[1][i];
	}
	Log(LogLevel::Info) << "Loaded " << glyphs.size() << " Cyrillic glyph substitutions.";
}

std::string mappers::RussianLocMapper::pickName(const std::string& english, const std::string& russian)
{
	if (transliterateNames && containsCyrillic(russian))
		return transliterate(russian);
	return english;
}

bool mappers::RussianLocMapper::containsCyrillic(const std::string& utf8)
{
	for (const auto cp: decodeUtf8(utf8))
		if (isCyrillic(cp))
			return true;
	return false;
}

std::string mappers::RussianLocMapper::encode(const std::string& utf8) const
{
	std::string out;
	for (const auto cp: decodeUtf8(utf8))
	{
		if (!isCyrillic(cp))
			out += encodeUtf8(cp);
		else if (const auto itr = glyphs.find(cp); itr != glyphs.end())
			out += itr->second;
		else
			out += latinFor(cp);
	}
	return out;
}

std::string mappers::RussianLocMapper::transliterate(const std::string& utf8)
{
	std::string out;
	for (const auto cp: decodeUtf8(utf8))
		out += isCyrillic(cp) ? latinFor(cp) : encodeUtf8(cp);
	return out;
}
