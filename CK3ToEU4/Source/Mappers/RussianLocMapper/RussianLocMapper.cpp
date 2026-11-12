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

// Order matches the alphabet file: А Б В Г Д Е Ё Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь Э Ю Я
const std::u32string upperAlphabet = U"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ";
const std::u32string lowerAlphabet = U"абвгдеёжзийклмнопрстуфхцчшщъыьэюя";

// Latin form of a lower case letter; the upper case form gets its first letter capitalised.
const std::map<char32_t, std::string> latin = {{U'а', "a"},
	 {U'б', "b"},
	 {U'в', "v"},
	 {U'г', "g"},
	 {U'д', "d"},
	 {U'е', "e"},
	 {U'ё', "e"},
	 {U'ж', "zh"},
	 {U'з', "z"},
	 {U'и', "i"},
	 {U'й', "j"},
	 {U'к', "k"},
	 {U'л', "l"},
	 {U'м', "m"},
	 {U'н', "n"},
	 {U'о', "o"},
	 {U'п', "p"},
	 {U'р', "r"},
	 {U'с', "s"},
	 {U'т', "t"},
	 {U'у', "u"},
	 {U'ф', "f"},
	 {U'х', "h"},
	 {U'ц', "c"},
	 {U'ч', "ch"},
	 {U'ш', "sh"},
	 {U'щ', "sch"},
	 {U'ъ', ""},
	 {U'ы', "y"},
	 {U'ь', ""},
	 {U'э', "e"},
	 {U'ю', "ju"},
	 {U'я', "ja"},
	 // Ukrainian / Belarusian
	 {U'і', "i"},
	 {U'ї', "ji"},
	 {U'є', "je"},
	 {U'ґ', "g"},
	 {U'ў', "u"}};

char32_t toLower(const char32_t cp)
{
	if (cp >= 0x0410 && cp <= 0x042F)
		return cp + 0x20;
	if (cp == 0x0401)
		return 0x0451; // Ё
	if (cp == 0x0406)
		return 0x0456; // І
	if (cp == 0x0407)
		return 0x0457; // Ї
	if (cp == 0x0404)
		return 0x0454; // Є
	if (cp == 0x0490)
		return 0x0491; // Ґ
	if (cp == 0x040E)
		return 0x045E; // Ў
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
