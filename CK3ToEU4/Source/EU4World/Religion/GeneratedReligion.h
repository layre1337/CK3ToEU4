#ifndef GENERATED_RELIGION_H
#define GENERATED_RELIGION_H

#include "Color.h"

namespace EU4
{
struct GeneratedReligion
{
	void outputGUI(std::ostream& output) const;
	void outputPersonalDeities(std::ostream& output) const;
	void outputRebels(std::ostream& output) const;
	void outputSounds(std::ostream& output) const;

	std::string country;
	std::string countrySecondary;
	std::string province;
	std::string unique;
	std::string nonUnique;
	std::filesystem::path iconPath;
	std::string religionGroup;
	std::string staticBlob;
	std::string name;
	std::string allowedConversion;
	std::string parent;
	std::optional<commonItems::Color> color;
	bool noClergyEstate = false; // CK3 lay clergy: the clergy estate is disabled for this religion
	int icon = 0;
	short personalDeityNumber = 0;
	friend std::ostream& operator<<(std::ostream& output, const GeneratedReligion& religion);
};
} // namespace EU4

#endif // GENERATED_RELIGION_H