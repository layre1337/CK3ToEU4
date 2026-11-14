#ifndef CK3_COUNTYDETAIL_H
#define CK3_COUNTYDETAIL_H
#include "Parser.h"
#include <map>
#include <optional>
#include <set>

namespace CK3
{
class Faith;
class Culture;
class CountyDetail: commonItems::parser
{
  public:
	CountyDetail() = default;
	CountyDetail(std::istream& theStream);

	[[nodiscard]] auto getDevelopment() const { return development; }
	[[nodiscard]] const auto& getCulture() const { return culture; }
	[[nodiscard]] const auto& getFaith() const { return faith; }
	[[nodiscard]] const auto& isDeJureHRE() const { return deJureHRE; }

	void loadCulture(const std::pair<long long, std::shared_ptr<Culture>>& theCulture) { culture = theCulture; }
	void loadFaith(const std::pair<long long, std::shared_ptr<Faith>>& theFaith) { faith = theFaith; }
	// CK3 1.20: counties reference a rite, and the rite knows its faith.
	void resolveRite(const std::map<long long, long long>& riteToFaith)
	{
		if (!rite)
			return;
		if (const auto& itr = riteToFaith.find(*rite); itr != riteToFaith.end())
			faith.first = itr->second;
	}
	void setDeJureHRE() { deJureHRE = true; }

  private:
	void registerKeys();

	int development = 0;
	std::pair<long long, std::shared_ptr<Culture>> culture;
	std::pair<long long, std::shared_ptr<Faith>> faith;
	std::optional<long long> rite;
	bool deJureHRE = false;
};
} // namespace CK3

#endif // CK3_COUNTYDETAIL_H
