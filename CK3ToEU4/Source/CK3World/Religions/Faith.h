#ifndef CK3_FAITH_H
#define CK3_FAITH_H
#include "Color.h"
#include "Parser.h"
#include <set>

extern commonItems::Color::Factory laFabricaDeColor;

namespace CK3
{
class Religion;
struct RiteDoctrines
{
	std::vector<std::string> tenets;
	std::vector<std::string> doctrines;
};
class Faith: commonItems::parser
{
  public:
	Faith() = default;
	Faith(std::istream& theStream, long long theID);

	[[nodiscard]] const auto& getName() const { return tag; }
	[[nodiscard]] const auto& getColor() const { return color; }
	[[nodiscard]] const auto& getDoctrines() const { return doctrines; }
	[[nodiscard]] const auto& getReligion() const { return religion; }
	[[nodiscard]] const auto& getReligiousHead() const { return religiousHead; }
	[[nodiscard]] auto getID() const { return ID; }
	[[nodiscard]] const auto& getCustomName() const { return customName; }
	[[nodiscard]] const auto& getCustomAdj() const { return customAdjective; }
	[[nodiscard]] const auto& getDescription() const { return description; }
	[[nodiscard]] const auto& getTemplate() const { return religionTemplate; }
	[[nodiscard]] const auto& getIconPath() const { return iconPath; }
	[[nodiscard]] const auto& getReformedFlag() const { return reformedFlag; }

	[[nodiscard]] auto getMainRite() const { return mainRite; }
	// CK3 1.20: a faith may have several rites (currents) with their own tenets and doctrines. The main rite defines the faith for the converter.
	// Tenets always come first.
	void useMainRiteDoctrines(const RiteDoctrines& rite)
	{
		std::vector<std::string> newDoctrines = rite.tenets;
		newDoctrines.insert(newDoctrines.end(), rite.doctrines.begin(), rite.doctrines.end());
		if (!newDoctrines.empty())
			doctrines = newDoctrines;
	}
	void setReligiousHead(const auto& newHead) { religiousHead = newHead; }
	void loadReligion(const std::pair<long long, std::shared_ptr<Religion>>& theReligion) { religion = theReligion; }

  private:
	void registerKeys();

	bool reformedFlag = false;
	long long ID = 0;
	std::string tag;
	std::string religionTemplate;
	std::filesystem::path iconPath;
	std::string customName;
	std::string customAdjective;
	std::string description;
	std::string religiousHead;
	std::optional<commonItems::Color> color;
	std::vector<std::string>
		 doctrines; // This is a vector in order to keep order consistent. We want the first things read (tenets) to be the first things output, ALWAYS
	std::optional<long long> mainRite;
	std::pair<long long, std::shared_ptr<Religion>> religion;
};
} // namespace CK3

#endif // CK3_FAITH_H
