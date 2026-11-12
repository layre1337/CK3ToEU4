#ifndef LOC_DEGRADER_MAPPER
#define LOC_DEGRADER_MAPPER
#include "Parser.h"

namespace mappers
{
class LocDegraderMapper: commonItems::parser
{
  public:
	LocDegraderMapper();
	explicit LocDegraderMapper(std::istream& theStream);

	// EU4 cannot show Cyrillic, so by default any Cyrillic left in the text is transliterated to Latin (Яков -> Jakov).
	[[nodiscard]] std::string degradeString(const std::string& inputString, bool transliterateCyrillic = true) const;

  private:
	void registerKeys();

	std::map<std::string, std::string> replacements;
};
} // namespace mappers

#endif // ISLAM_OVERRIDE_MAPPER