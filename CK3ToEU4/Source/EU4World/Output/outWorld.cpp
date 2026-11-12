#include "../../CK3World/World.h"
#include "../../Configuration/Configuration.h"
#include "../EU4World.h"
#include "../Province/EU4Province.h"
#include "CommonFunctions.h"
#include "Log.h"
#include "OSCompatibilityLayer.h"
#include <filesystem>
#include <fstream>
#include "../../Mappers/RussianLocMapper/RussianLocMapper.h"
#include <ranges>
#include <regex>

#include "../../CK3World/Cultures/Culture.h"
#include "../../Mappers/CultureDefinitionsMapper/CultureDefiniton.h"
#include "../../Mappers/CultureDefinitionsMapper/CultureGroupDefinition.h"
namespace fs = std::filesystem;

void EU4::World::output(const commonItems::ConverterVersion& converterVersion,
	 const Configuration& theConfiguration,
	 const CK3::World& sourceWorld,
	 date startDate) const
{
	const auto invasion = theConfiguration.getSunset() == Configuration::SUNSET::ACTIVE;
	const auto dynamicInstitutions = theConfiguration.getDynamicInstitutions() == Configuration::INSTITUTIONS::DYNAMIC;
	date conversionDate = sourceWorld.getConversionDate();
	if (theConfiguration.getStartDateOption() == Configuration::STARTDATE::MANUAL)
		conversionDate = startDate;
	Log(LogLevel::Info) << "<- Creating Output Folder";

	fs::create_directory("output");
	if (commonItems::DoesFolderExist("output" / theConfiguration.getOutputName()))
	{
		Log(LogLevel::Info) << "<< Deleting existing mod folder.";
		fs::remove_all("output" / theConfiguration.getOutputName());
	}
	Log(LogLevel::Progress) << "80 %";

	Log(LogLevel::Info) << "<- Copying Mod Template";
	try
	{
		fs::copy("blankMod/output", "output/output", fs::copy_options::recursive);
	}
	catch (std::exception& e)
	{
		Log(LogLevel::Error) << "Could not copy folder: " << e.what() << " " << commonItems::GetLastErrorString();
	}
	Log(LogLevel::Progress) << "81 %";

	Log(LogLevel::Info) << "<- Moving Mod Template >> " << theConfiguration.getOutputName().string();
	try
	{
		fs::rename("output/output", "output" / theConfiguration.getOutputName());
	}
	catch (std::exception& e)
	{
		Log(LogLevel::Error) << "Could not rename folder: " << e.what() << " " << commonItems::GetLastErrorString();
	}
	Log(LogLevel::Progress) << "82 %";

	fs::create_directories("output" / theConfiguration.getOutputName() / "history");
	fs::create_directories("output" / theConfiguration.getOutputName() / "history/countries");
	fs::create_directories("output" / theConfiguration.getOutputName() / "history/advisors");
	fs::create_directories("output" / theConfiguration.getOutputName() / "history/provinces");
	fs::create_directories("output" / theConfiguration.getOutputName() / "history/diplomacy");
	fs::create_directories("output" / theConfiguration.getOutputName() / "common");
	fs::create_directories("output" / theConfiguration.getOutputName() / "common/countries");
	fs::create_directories("output" / theConfiguration.getOutputName() / "common/country_tags");
	fs::create_directories("output" / theConfiguration.getOutputName() / "localisation");
	Log(LogLevel::Progress) << "83 %";

	Log(LogLevel::Info) << "<- Crafting .mod File";
	createModFile(theConfiguration);
	Log(LogLevel::Progress) << "84 %";

	// Record converter version
	Log(LogLevel::Info) << "<- Writing version";
	outputVersion(converterVersion, theConfiguration);
	Log(LogLevel::Progress) << "85 %";

	// Output common\countries.txt
	Log(LogLevel::Info) << "<- Creating countries.txt";
	outputCommonCountriesFile(theConfiguration);
	Log(LogLevel::Progress) << "86 %";

	Log(LogLevel::Info) << "<- Writing Country Commons";
	outputCommonCountries(theConfiguration);
	Log(LogLevel::Progress) << "87 %";

	Log(LogLevel::Info) << "<- Writing Country Histories";
	outputHistoryCountries(theConfiguration);
	Log(LogLevel::Progress) << "88 %";

	Log(LogLevel::Info) << "<- Writing Religions";
	outputReligions(theConfiguration.getOutputName(), religionMapper.getGeneratedReligions(), religionMapper.getReformedReligions());
	outputChurchEstate(theConfiguration, religionMapper.getGeneratedReligions());

	Log(LogLevel::Info) << "<- Writing Cultures";
	outputCultures(theConfiguration.getOutputName());
	Log(LogLevel::Progress) << "89 %";

	Log(LogLevel::Info) << "<- Writing Dynamic Ideas";
	outputDynamicIdeasFile(theConfiguration.getOutputName());

	if (invasion)
	{
		Log(LogLevel::Info) << "<- Writing Sunset Invasion Files";
		outputInvasionExtras(theConfiguration);
	}
	if (dynamicInstitutions)
	{
		Log(LogLevel::Info) << "<- Writing Dynamic Institution Files";
		outputDynamicInstitutions(theConfiguration);
	}

	Log(LogLevel::Info) << "<- Writing Advisers";
	outputAdvisers(theConfiguration);
	Log(LogLevel::Progress) << "90 %";

	Log(LogLevel::Info) << "<- Writing Provinces";
	outputHistoryProvinces(theConfiguration);
	Log(LogLevel::Progress) << "91 %";

	Log(LogLevel::Info) << "<- Writing Localization";
	outputLocalization(theConfiguration, invasion);
	Log(LogLevel::Progress) << "92 %";

	Log(LogLevel::Info) << "<- Writing Emperor";
	outputEmperor(theConfiguration, conversionDate);
	Log(LogLevel::Progress) << "93 %";

	Log(LogLevel::Info) << "<- Writing Diplomacy";
	outputDiplomacy(theConfiguration, diplomacy.getAgreements(), invasion);
	Log(LogLevel::Progress) << "94 %";

	Log(LogLevel::Info) << "<- Copying Flags";
	outputFlags(theConfiguration);
	Log(LogLevel::Progress) << "95 %";

	Log(LogLevel::Info) << "<- Writing Religion Icons";
	outputReligionIcons(theConfiguration, religionMapper.getGeneratedReligions(), sourceWorld.getMods());
	Log(LogLevel::Progress) << "96 %";

	Log(LogLevel::Info) << "<- Replacing Bookmark";
	outputBookmark(theConfiguration, conversionDate);
	Log(LogLevel::Progress) << "97 %";
}

void EU4::World::outputReligionIcons(const Configuration& theConfiguration, const std::vector<GeneratedReligion>& generatedReligions, const Mods& mods) const
{
	// edit the strips
	flagFoundry.extendReligionStrips(theConfiguration, generatedReligions, mods);

	// edit interface file, raw search/replace.
	const auto originalIcons = religionDefinitionMapper.getOriginalIconCount();
	auto currentIcons = originalIcons + religionMapper.getGeneratedReligions().size();
	if (!commonItems::DoesFileExist("output" / theConfiguration.getOutputName() / "interface/z_converter.gfx"))
		throw std::runtime_error("Cannot open output/" + theConfiguration.getOutputName().string() + "/interface/z_converter.gfx");

	std::ifstream sourceInterfaces("output" / theConfiguration.getOutputName() / "interface/z_converter.gfx");
	std::stringstream interfaceStream;
	interfaceStream << sourceInterfaces.rdbuf();
	sourceInterfaces.close();

	auto interfaceData = interfaceStream.str();
	auto swap = std::to_string(originalIcons);
	auto with = std::to_string(currentIcons);
	auto pos = interfaceData.find(swap);
	while (pos != std::string::npos)
	{
		interfaceData.replace(pos, swap.size(), with);
		// Get the next occurrence from the current position
		pos = interfaceData.find(swap, pos + with.size());
	}

	std::ofstream interFaceDump("output" / theConfiguration.getOutputName() / "interface/z_converter.gfx");
	interFaceDump << interfaceData;
	interFaceDump.close();
}

void EU4::World::outputCultures(const std::filesystem::path& outputName) const
{
	std::ofstream cultureFile("output" / outputName / "common/cultures/!.ZZ-dynamic_cultures.txt");
	cultureFile << cultureDefinitionsMapper;
	cultureFile.close();
}

void EU4::World::outputChurchEstate(const Configuration& theConfiguration, const std::vector<GeneratedReligion>& generatedReligions) const
{
	// Religions of faiths with lay clergy have no clergy estate. EU4 decides who gets the estate in the "trigger" of estate_church, so we take the
	// vanilla file from the user's EU4 install (always matching their game version), and add an exception to its trigger.
	std::vector<std::string> noClergyReligions;
	for (const auto& religion: generatedReligions)
		if (religion.noClergyEstate)
			noClergyReligions.emplace_back(religion.name);
	if (noClergyReligions.empty())
		return;

	std::ifstream input(theConfiguration.getEU4Path() / "common/estates/01_church.txt");
	if (!input.is_open())
	{
		Log(LogLevel::Warning) << "Where is " << theConfiguration.getEU4Path().string() << "/common/estates/01_church.txt?! Clergy estate stays enabled.";
		return;
	}
	std::stringstream inStream;
	inStream << input.rdbuf();
	auto estateString = inStream.str();
	input.close();

	const std::string triggerOpening = "trigger = {";
	const auto estatePos = estateString.find("estate_church");
	const auto triggerPos = estatePos == std::string::npos ? std::string::npos : estateString.find(triggerOpening, estatePos);
	if (triggerPos == std::string::npos)
	{
		Log(LogLevel::Warning) << "Could not find the trigger of estate_church in EU4's 01_church.txt. Clergy estate stays enabled.";
		return;
	}

	std::string exception = "\n\t\t# CK3ToEU4: faiths with lay clergy have no clergy estate\n\t\tNOT = {\n\t\t\tOR = {";
	for (const auto& religionName: noClergyReligions)
		exception += "\n\t\t\t\treligion = " + religionName;
	exception += "\n\t\t\t}\n\t\t}";
	estateString.insert(triggerPos + triggerOpening.size(), exception);

	std::ofstream output("output" / theConfiguration.getOutputName() / "common/estates/01_church.txt");
	if (!output.is_open())
		throw std::runtime_error("Could not write the clergy estate file! Is the output folder writable?");
	output << estateString;
	output.close();
	Log(LogLevel::Info) << "<> Clergy estate disabled for " << noClergyReligions.size() << " religions with lay clergy.";
}

void EU4::World::outputReligions(const std::filesystem::path& outputName,
	 const std::vector<GeneratedReligion>& generatedReligions,
	 const std::vector<std::string>& reformedReligions) const
{
	// Reformed Religion global flag output
	std::ofstream globalFlagFile("output" / outputName / "common/on_actions/ZZZ_replaced_on_startup.txt");
	// Basegame stuff first
	globalFlagFile << "on_startup = {\n\t"
						<< "emperor = {\n\t\t"
						<< "save_global_event_target_as = Emperor\n\t"
						<< "}\n\t"
						<< "if = {\n\t\t"
						<< "limit = { is_emperor_of_china = yes }\n\t\t"
						<< "save_global_event_target_as = EmperorOfChina\n\t"
						<< "}\n\t"
						<< "events = {\n\t\t"
						<< "muslim_school_events.20 #Pick School #flavor_fra .15000 #Make the French Revolution happen if starting in 1789\n\t"
						<< "}\n\t"
						<< "if = {\n\t\t"
						<< "limit = { has_dlc = \"Cradle of Civilization\" has_country_modifier = tur_janissary }\n\t\t"
						<< "remove_country_modifier = tur_janissary\n\t"
						<< "}\n\t"
						<< "initialize_schools_effect = yes #This is only used here but is used for readability of on_action file.\n\n\t"
						<< "if = {\n\t\t"
						<< "limit = { NOT = { has_country_flag = new_flavour_bav_13_has_been_triggered } OR = { tag = UBV tag = LBV tag = ING } }\n\t\t"
						<< "set_country_flag = new_flavour_bav_13_has_been_triggered country_event = { id = new_flavour_bav.13 days = 90 } #better not to allow "
							"almost instant DOWs\n\t"
						<< "}\n\t"
						<< "if = {\n\t\t"
						<< "limit = { tag = FRA started_in = 1789.7.14 NOT = { is_year = 1790 } NOT = { has_disaster = french_revolution } }\n\t\t"
						<< "add_disaster_progress = { value = 100 disaster = french_revolution }\n\t"
						<< "}\n\t";
	// Now our stuff
	globalFlagFile << "if={\n\t\tlimit = { NOT = { has_global_flag = ZZZ_enhanced_random_world } }";
	for (const auto& religion: reformedReligions)
	{
		globalFlagFile << "\n\t\tset_global_flag = ZZZ_enhanced_" << religion << "_reformed";
	}
	globalFlagFile << "\n\t}\n}";
	globalFlagFile.close();

	for (const auto& religion: generatedReligions)
	{
		// Main Religion File
		std::ofstream religionFile("output" / outputName / ("common/religions/99_" + religion.name + "-from-" + religion.parent + ".txt"));
		religionFile << religion;
		religionFile.close();
		// Religious Rebels
		std::ofstream religionRebelFile("output" / outputName / ("common/rebel_types/99_rebel_" + religion.name + "-from-" + religion.parent + ".txt"));
		religion.outputRebels(religionRebelFile);
		religionRebelFile.close();
		// Religious Personal Deities
		std::ofstream religionPersonalDeityFile(
			 "output" / outputName / "common/personal_deities" / (religion.name + "-from-" + religion.parent + "_deities.txt"));
		religion.outputPersonalDeities(religionPersonalDeityFile);
		religionPersonalDeityFile.close();
		// Religion Sounds
		std::ofstream religionSoundFile("output" / outputName / ("sound/ZZZ_converted_" + religion.name + "-from-" + religion.parent + "sounds.asset"));
		religion.outputSounds(religionSoundFile);
		religionSoundFile.close();
		// Religion GUI (Only if necessary)
		if (religion.unique.find("uses_judaism_power = yes") != std::string::npos)
		{
			std::ofstream religionGUIFile("output" / outputName / ("interface/ZZZ_" + religion.name + "-from-" + religion.parent + ".gui"));
			religion.outputGUI(religionGUIFile);
			religionGUIFile.close();
		}
	}
}

void EU4::World::outputBookmark(const Configuration& theConfiguration, date conversionDate) const
{
	if (theConfiguration.getStartDateOption() != Configuration::STARTDATE::EU)
	{
		// fix the dynamic bookmark in defines
		if (!commonItems::DoesFileExist("output" / theConfiguration.getOutputName() / "common/defines/00_converter_defines.lua"))
			throw std::runtime_error("Can not find output/" + theConfiguration.getOutputName().string() + "/common/defines/00_converter_defines.lua!");
		std::ofstream defines("output" / theConfiguration.getOutputName() / "common/defines/00_converter_defines.lua");
		defines << "-- Defines modified by the converter\n\n";
		defines << "\nNDefines.NGame.START_DATE = \"" << conversionDate << "\"\n";
		defines.close();

		// fix the dynamic bookmark in bookmark file
		if (!commonItems::DoesFileExist("output" / theConfiguration.getOutputName() / "common/bookmarks/converter_bookmark.txt"))
			throw std::runtime_error("Can not find output/" + theConfiguration.getOutputName().string() + "/common/bookmarks/converter_bookmark.txt!");

		std::string startDate = "<CONVERSIONDATE>";
		std::ostringstream incomingBookmarks;

		std::ifstream bookmarks_txt("output" / theConfiguration.getOutputName() / "common/bookmarks/converter_bookmark.txt");
		incomingBookmarks << bookmarks_txt.rdbuf();
		bookmarks_txt.close();
		auto strBookmarks = incomingBookmarks.str();
		auto pos2 = strBookmarks.find(startDate);
		strBookmarks.replace(pos2, startDate.length(), conversionDate.toString());
		std::ofstream out_bookmarks_txt("output" / theConfiguration.getOutputName() / "common/bookmarks/converter_bookmark.txt");
		out_bookmarks_txt << strBookmarks;
		out_bookmarks_txt.close();

		// And wipe regular one
		std::ifstream bookmarks1444_txt("output" / theConfiguration.getOutputName() / "common/bookmarks/converter_bookmark_1444.txt",
			 std::ofstream::out | std::ofstream::trunc);
		bookmarks1444_txt.close();
	}
	else
	{
		// Vanilla defines are fine, just wipe dynamic bookmark
		std::ifstream bookmarks_txt("output" / theConfiguration.getOutputName() / "common/bookmarks/converter_bookmark.txt",
			 std::ofstream::out | std::ofstream::trunc);
		bookmarks_txt.close();
	}
}

void EU4::World::createModFile(const Configuration& theConfiguration) const
{
	std::ofstream output("output/" + theConfiguration.getOutputName().string() + ".mod");
	if (!output.is_open())
		throw std::runtime_error("Could not create " + theConfiguration.getOutputName().string() + ".mod");
	Log(LogLevel::Info) << "<< Writing to: "
							  << "output/" + theConfiguration.getOutputName().string() + ".mod";
	output << modFile;
	output.close();

	std::ofstream output2("output" / theConfiguration.getOutputName() / "descriptor.mod");
	if (!output2.is_open())
		throw std::runtime_error("Could not create " + theConfiguration.getOutputName().string() + "/descriptor.mod");
	Log(LogLevel::Info) << "<< Writing to: "
							  << "output/" + theConfiguration.getOutputName().string() + "descriptor.mod";
	output2 << modFile;
	output2.close();
}

void EU4::World::outputVersion(const commonItems::ConverterVersion& converterVersion, const Configuration& theConfiguration) const
{
	std::ofstream output("output" / theConfiguration.getOutputName() / "ck3toeu4_version.txt");
	if (!output.is_open())
		throw std::runtime_error("Error writing version file! Is the output folder writable?");
	output << converterVersion;
	output.close();
}

void EU4::World::outputInvasionExtras(const Configuration& theConfiguration) const
{
	// Sunset Religions
	auto files = commonItems::GetAllFilesInFolder("configurables/sunset/common/religions");
	for (const auto& file: files)
		fs::copy_file("configurables/sunset/common/religions" / file,
			 "output" / theConfiguration.getOutputName() / "common/religions" / file,
			 fs::copy_options::overwrite_existing);
	// Sunset Ideas
	files = commonItems::GetAllFilesInFolder("configurables/sunset/common/ideas");
	for (const auto& file: files)
		fs::copy_file("configurables/sunset/common/ideas" / file,
			 "output" / theConfiguration.getOutputName() / "common/ideas" / file,
			 fs::copy_options::overwrite_existing);
	// Sunset Cultures
	files = commonItems::GetAllFilesInFolder("configurables/sunset/common/cultures");
	for (const auto& file: files)
		fs::copy_file("configurables/sunset/common/cultures" / file,
			 "output" / theConfiguration.getOutputName() / "common/cultures" / file,
			 fs::copy_options::overwrite_existing);
	// Sunset Decisions
	files = commonItems::GetAllFilesInFolder("configurables/sunset/decisions");
	for (const auto& file: files)
		fs::copy_file("configurables/sunset/decisions" / file,
			 "output" / theConfiguration.getOutputName() / "decisions" / file,
			 fs::copy_options::overwrite_existing);
}

void EU4::World::outputDynamicInstitutions(const Configuration& theConfiguration) const
{
	// Dynamic Institions
	auto files = commonItems::GetAllFilesInFolder("configurables/dynamicInstitutions/institutions");
	fs::create_directories("output" / theConfiguration.getOutputName() / "common/institutions");
	for (const auto& file: files)
		fs::copy_file("configurables/dynamicInstitutions/institutions" / file,
			 "output" / theConfiguration.getOutputName() / "common/institutions" / file,
			 fs::copy_options::overwrite_existing);
	// Dynamic Ideas
	files = commonItems::GetAllFilesInFolder("configurables/dynamicInstitutions/ideas");
	for (const auto& file: files)
		fs::copy_file("configurables/dynamicInstitutions/ideas" / file,
			 "output" / theConfiguration.getOutputName() / "common/ideas" / file,
			 fs::copy_options::overwrite_existing);
}

void EU4::World::outputCommonCountriesFile(const Configuration& theConfiguration) const
{
	std::ofstream output("output" / theConfiguration.getOutputName() / "common/country_tags/00_countries.txt");
	if (!output.is_open())
		throw std::runtime_error("Could not create countries file!");
	output << "REB = \"countries/Rebels.txt\"\n\n"; // opening with rebels manually.

	for (const auto& country: countries)
	{
		if (specialCountryTags.count(country.first))
			continue; // Not outputting specials.
		if (country.first != "REB")
			output << country.first << " = \"countries/" << country.second->getCommonCountryFile().string() << "\"\n";
	}
	output << "\n";
	output.close();
}

void EU4::World::outputCommonCountries(const Configuration& theConfiguration) const
{
	for (const auto& country: countries)
	{
		std::ofstream output("output" / theConfiguration.getOutputName() / "common/countries" / country.second->getCommonCountryFile());
		if (!output.is_open())
			throw std::runtime_error("Could not create country common file: output/" + theConfiguration.getOutputName().string() + "/common/countries/" +
											 country.second->getCommonCountryFile().string());
		country.second->outputCommons(output);
		output.close();
	}
}

void EU4::World::outputHistoryCountries(const Configuration& theConfiguration) const
{
	for (const auto& country: countries)
	{
		std::ofstream output("output" / theConfiguration.getOutputName() / "history/countries" / country.second->getHistoryCountryFile());
		if (!output.is_open())
			throw std::runtime_error("Could not create country history file: output/" + theConfiguration.getOutputName().string() + "/history/countries/" +
											 country.second->getHistoryCountryFile().string());
		output << *country.second;
		output.close();
	}
}

void EU4::World::outputDynamicIdeasFile(const std::filesystem::path& outputName) const
{
	std::ofstream output("output" / outputName / "common/ideas/00_dynamic_ideas.txt"); // Not sure where to put this
	if (!output.is_open())
		throw std::runtime_error("Could not create dynamic ideas file!");
	output << "### National idea groups generated via cultural traditions. \n\n";

	for (const auto& idea: dynamicNationalIdeas)
	{
		output << idea;
		output << "\n";
	}
	output.close();
}

void EU4::World::outputAdvisers(const Configuration& theConfiguration) const
{
	std::ofstream output("output" / theConfiguration.getOutputName() / "history/advisors/00_converter_advisors.txt");
	if (!output.is_open())
		throw std::runtime_error("Could not create " + theConfiguration.getOutputName().string() + "/history/advisors/00_converter_advisors.txt");
	for (const auto& country: countries)
	{
		country.second->outputAdvisers(output);
	}
	output.close();
}

void EU4::World::outputHistoryProvinces(const Configuration& theConfiguration) const
{
	for (const auto& province: provinces | std::views::values)
	{
		std::ofstream output("output" / theConfiguration.getOutputName() / "history/provinces" / province->getHistoryProvincesFile());
		if (!output.is_open())
			throw std::runtime_error("Could not create country history file: output/" + theConfiguration.getOutputName().string() + "/history/provinces/" +
											 province->getHistoryProvincesFile().string());
		output << *province;
		output.close();
	}
}

void EU4::World::outputLocalization(const Configuration& theConfiguration, bool invasion) const
{
	std::ofstream english("output" / theConfiguration.getOutputName() / "localisation/replace/converter_l_english.yml");
	std::ofstream french("output" / theConfiguration.getOutputName() / "localisation/replace/converter_l_french.yml");
	std::ofstream spanish("output" / theConfiguration.getOutputName() / "localisation/replace/converter_l_spanish.yml");
	std::ofstream german("output" / theConfiguration.getOutputName() / "localisation/replace/converter_l_german.yml");
	if (!english.is_open())
		throw std::runtime_error("Error writing localization file! Is the output folder writable?");
	if (!french.is_open())
		throw std::runtime_error("Error writing localization file! Is the output folder writable?");
	if (!spanish.is_open())
		throw std::runtime_error("Error writing localization file! Is the output folder writable?");
	if (!german.is_open())
		throw std::runtime_error("Error writing localization file! Is the output folder writable?");
	// With the Russian localization option the "english" file carries Russian text written in the glyphs of the unofficial Russian
	// localization mod (EU4 has no Cyrillic). That mod redraws certain characters as Cyrillic letters.
	const auto russian = theConfiguration.getRussianLocalization();
	mappers::RussianLocMapper russianLoc;
	if (russian)
		russianLoc.loadAlphabet();
	const auto englishText = [&](const mappers::LocBlock& block) {
		if (!russian)
			return locDegrader.degradeString(block.english);
		return russianLoc.encode(locDegrader.degradeString(block.russian.empty() ? block.english : block.russian, false));
	};
	const auto englishPlain = [&](const std::string& text) {
		if (!russian)
			return locDegrader.degradeString(text);
		return russianLoc.encode(locDegrader.degradeString(text, false));
	};

	english << commonItems::utf8BOM << "l_english:\n"; // write BOM
	french << commonItems::utf8BOM << "l_french:\n";	// write BOM
	spanish << commonItems::utf8BOM << "l_spanish:\n"; // write BOM
	german << commonItems::utf8BOM << "l_german:\n";	// write BOM

	for (const auto& country: countries)
	{
		for (const auto& locblock: country.second->getLocalizations())
		{
			english << " " << locblock.first << ":3 \"" << englishText(locblock.second) << "\"\n";
			french << " " << locblock.first << ":3 \"" << locDegrader.degradeString(locblock.second.french) << "\"\n";
			spanish << " " << locblock.first << ":3 \"" << locDegrader.degradeString(locblock.second.spanish) << "\"\n";
			german << " " << locblock.first << ":3 \"" << locDegrader.degradeString(locblock.second.german) << "\"\n";
		}
	}

	for (const auto& locblock: religionMapper.getLocalizations())
	{
		english << " " << locblock.first << ":3 \"" << englishText(locblock.second) << "\"\n";
		french << " " << locblock.first << ":3 \"" << locDegrader.degradeString(locblock.second.french) << "\"\n";
		spanish << " " << locblock.first << ":3 \"" << locDegrader.degradeString(locblock.second.spanish) << "\"\n";
		german << " " << locblock.first << ":3 \"" << locDegrader.degradeString(locblock.second.german) << "\"\n";
	}

	// localizations for dynamic cultures - don't have locblocks.

	for (const auto& cultureGroup: cultureDefinitionsMapper.getCultureGroupsMap() | std::views::values)
		for (const auto& [cultureName, culture]: cultureGroup->getCultures())
			if (culture->getSourceCulture() && culture->getSourceCulture()->isDynamic() && !culture->getSourceCulture()->isEU4Ready() &&
				 culture->getSourceCulture()->getLocalizedName())
			{
				english << " " << cultureName << ":0 \"" << englishPlain(*culture->getSourceCulture()->getLocalizedName()) << "\"\n";
				french << " " << cultureName << ":0 \"" << locDegrader.degradeString(*culture->getSourceCulture()->getLocalizedName()) << "\"\n";
				spanish << " " << cultureName << ":0 \"" << locDegrader.degradeString(*culture->getSourceCulture()->getLocalizedName()) << "\"\n";
				german << " " << cultureName << ":0 \"" << locDegrader.degradeString(*culture->getSourceCulture()->getLocalizedName()) << "\"\n";
			}

	// localizations for dynamic nation ideas
	for (const auto& [idea, locBlock]: dynamicIdeasMapper.getTraditionLocs())
	{
		english << " " << idea << ":0 \"" << (russian ? englishText(locBlock) : locBlock.english) << "\"\n";
		french << " " << idea << ":0 \"" << locBlock.french << "\"\n";
		spanish << " " << idea << ":0 \"" << locBlock.spanish << "\"\n";
		german << " " << idea << ":0 \"" << locBlock.german << "\"\n";
	}
	// more localizations for dynamic national ideas - don't have locblocks.
	std::vector<std::string> suffix{"_ideas", "_ideas_start", "_ideas_bonus"};
	std::vector<std::string> eng_ideaText{"Ideas", "Traditions", "Ambitions"};
	std::vector<std::string> fra_ideaText{"Idées", "Traditions", "Ambitions"};
	std::vector<std::string> spa_ideaText{"Ideas", "tradiciones", "Ambiciones"};
	std::vector<std::string> ger_ideaText{"Ideen", "Traditionen", "Ambitionen"};
	// "Идеи", "Традиции", "Амбиции" - written as UTF-8 bytes so the source file encoding doesn't matter.
	std::vector<std::string> rus_ideaText{"\xD0\x98\xD0\xB4\xD0\xB5\xD0\xB8",
		 "\xD0\xA2\xD1\x80\xD0\xB0\xD0\xB4\xD0\xB8\xD1\x86\xD0\xB8\xD0\xB8",
		 "\xD0\x90\xD0\xBC\xD0\xB1\xD0\xB8\xD1\x86\xD0\xB8\xD0\xB8"};

	for (const auto& idea: dynamicNationalIdeas)
	{
		for (auto i = 0; i < suffix.size(); i++)
		{
			english << " " << idea.getDynamicName() + suffix[i] << ":0 \""
					<< (russian ? englishPlain(idea.getLocalizedName() + " " + rus_ideaText[i]) : locDegrader.degradeString(idea.getLocalizedName()) + " " + eng_ideaText[i])
					<< "\"\n";
			french << " " << idea.getDynamicName() + suffix[i] << ":0 \"" << locDegrader.degradeString(idea.getLocalizedName()) + " " + fra_ideaText[i] << "\"\n";
			spanish << " " << idea.getDynamicName() + suffix[i] << ":0 \"" << locDegrader.degradeString(idea.getLocalizedName()) + " " + spa_ideaText[i] << "\"\n";
			german << " " << idea.getDynamicName() + suffix[i] << ":0 \"" << locDegrader.degradeString(idea.getLocalizedName()) + " " + ger_ideaText[i] << "\"\n";
		}
	}

	// port manually named provinces over from CK3
	auto userRenamedOnly = [](std::shared_ptr<EU4::Province> p) {
		return bool(p->isRenamed());
	};

	for (const auto& province: provinces | std::views::values | std::views::filter(userRenamedOnly))
	{
		english << " PROV" << province->getProvinceID() << ":0 \"" << province->getCustomName() << "\"\n";
		french << " PROV" << province->getProvinceID() << ":0 \"" << province->getCustomName() << "\"\n";
		spanish << " PROV" << province->getProvinceID() << ":0 \"" << province->getCustomName() << "\"\n";
		german << " PROV" << province->getProvinceID() << ":0 \"" << province->getCustomName() << "\"\n";

		english << " PROV_ADJ" << province->getProvinceID() << ":0 \"" << province->getCustomName() << "\"\n";
		french << " PROV_ADJ" << province->getProvinceID() << ":0 \"" << province->getCustomName() << "\"\n";
		spanish << " PROV_ADJ" << province->getProvinceID() << ":0 \"" << province->getCustomName() << "\"\n";
		german << " PROV_ADJ" << province->getProvinceID() << ":0 \"" << province->getCustomName() << "\"\n";
	}
	english.close();
	french.close();
	spanish.close();
	german.close();

	if (invasion)
	{
		auto fileNames = commonItems::GetAllFilesInFolder("configurables/sunset/localisation/");
		for (const auto& fileName: fileNames)
			fs::copy_file("configurables/sunset/localisation" / fileName,
				 "output" / theConfiguration.getOutputName() / "localisation" / fileName,
				 fs::copy_options::overwrite_existing);
	}

	auto fileNames = commonItems::GetAllFilesInFolder("configurables/religions/reformation/roman/");
	for (const auto& fileName: fileNames)
		fs::copy_file("configurables/religions/reformation/roman" / fileName,
			 "output" / theConfiguration.getOutputName() / "localisation" / fileName,
			 fs::copy_options::overwrite_existing);

	if (russian)
		outputRussianStaticLocalization(theConfiguration, russianLoc);
}

// The converter's own static texts (decisions, missions, events, ideas...) ship in English. Russian translations live in
// configurables/russian_loc/localisation/**/<name>_l_russian.yml in plain Cyrillic. For every key found there, the matching line of the
// output <name>_l_english.yml is replaced with the translation, written in the Russian localization mod's glyphs. Untranslated keys stay English.
void EU4::World::outputRussianStaticLocalization(const Configuration& theConfiguration, const mappers::RussianLocMapper& russianLoc) const
{
	const std::filesystem::path sourceRoot = "configurables/russian_loc/localisation";
	if (!commonItems::DoesFolderExist(sourceRoot))
		return;

	const std::regex linePattern(R"re(^(\s*)([^\s:#]+):(\d*)\s*"(.*)"\s*(#.*)?$)re");
	const auto readLines = [](const std::filesystem::path& path) {
		std::vector<std::string> lines;
		std::ifstream in(path, std::ios::binary);
		std::string line;
		while (std::getline(in, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			lines.push_back(line);
		}
		if (!lines.empty() && lines.front().starts_with(commonItems::utf8BOM))
			lines.front().erase(0, std::string(commonItems::utf8BOM).size());
		return lines;
	};

	auto translatedFiles = 0;
	auto translatedKeys = 0;
	for (const auto& relative: commonItems::GetAllFilesInFolderRecursive(sourceRoot))
	{
		const std::filesystem::path relativePath(relative);
		auto fileName = relativePath.filename().string();
		const std::string suffix = "_l_russian.yml";
		if (!fileName.ends_with(suffix))
			continue;
		fileName.replace(fileName.size() - suffix.size(), suffix.size(), "_l_english.yml");
		const auto target = "output" / theConfiguration.getOutputName() / "localisation" / relativePath.parent_path() / fileName;
		if (!std::filesystem::exists(target))
		{
			Log(LogLevel::Warning) << "Russian localization " << relative << " has no matching " << fileName << " in the output mod. Skipping.";
			continue;
		}

		std::map<std::string, std::string> translations;
		for (const auto& line: readLines(sourceRoot / relativePath))
			if (std::smatch match; std::regex_match(line, match, linePattern))
				translations[match[2].str()] = match[4].str();

		auto lines = readLines(target);
		std::ofstream out(target, std::ios::binary | std::ios::trunc);
		if (!out.is_open())
			continue;
		out << commonItems::utf8BOM;
		for (const auto& line: lines)
		{
			std::smatch match;
			if (std::regex_match(line, match, linePattern) && translations.contains(match[2].str()))
			{
				out << match[1].str() << match[2].str() << ":" << match[3].str() << " \"" << russianLoc.encode(translations.at(match[2].str())) << "\"\n";
				++translatedKeys;
			}
			else
				out << line << "\n";
		}
		++translatedFiles;
	}
	Log(LogLevel::Info) << "Russian localization: " << translatedKeys << " texts in " << translatedFiles << " files.";
}

void EU4::World::outputEmperor(const Configuration& theConfiguration, date conversionDate) const
{
	auto actualConversionDate = conversionDate;
	if (theConfiguration.getStartDateOption() == Configuration::STARTDATE::EU)
		actualConversionDate = date(1444, 11, 11);

	std::ofstream output("output" / theConfiguration.getOutputName() / "history/diplomacy/hre.txt");
	if (!output.is_open())
		throw std::runtime_error("Could not create hre diplomacy file: output/" + theConfiguration.getOutputName().string() + "/history/diplomacy/hre.txt!");
	if (emperorTag.empty())
		output << actualConversionDate << " = { emperor = --- }\n";
	else
		output << actualConversionDate << " = { emperor = " << emperorTag << " }\n";
	output.close();

	output.open("output" / theConfiguration.getOutputName() / "history/diplomacy/celestial_empire.txt");
	if (MoHTag.empty())
	{
		output << actualConversionDate << " = { celestial_emperor = --- }\n";
	}
	else
	{
		output << actualConversionDate << " = { celestial_emperor = " << MoHTag << " }\n";
	}
	output.close();

	if (!actualHRETag.empty())
	{
		output.open("output" / theConfiguration.getOutputName() / "i_am_hre.txt");
		output << actualHRETag;
		output.close();

		if (actualHRETag != "HRE")
		{
			std::ifstream input(theConfiguration.getEU4Path() / "events/HolyRomanEmpire.txt");
			if (!input.is_open())
			{
				Log(LogLevel::Warning) << "Where is " << theConfiguration.getEU4Path().string() << "/events/HolyRomanEmpire.txt?!";
			}
			else
			{
				std::stringstream inStream;
				inStream << input.rdbuf();
				auto eventFileString = inStream.str();
				input.close();
				eventFileString = std::regex_replace(eventFileString, std::regex("HLR"), actualHRETag);
				output.open("output" / theConfiguration.getOutputName() / "events/HolyRomanEmpire.txt");
				output << eventFileString;
				output.close();
			}
		}
	}
}

void EU4::World::outputDiplomacy(const Configuration& theConfiguration, const std::vector<std::shared_ptr<Agreement>>& agreements, bool invasion) const
{
	std::ofstream alliances("output" / theConfiguration.getOutputName() / "history/diplomacy/converter_alliances.txt");
	if (!alliances.is_open())
		throw std::runtime_error("Could not create alliances history file!");

	std::ofstream guarantees("output" / theConfiguration.getOutputName() / "history/diplomacy/converter_guarantees.txt");
	if (!guarantees.is_open())
		throw std::runtime_error("Could not create guarantees history file!");

	std::ofstream puppetStates("output" / theConfiguration.getOutputName() / "history/diplomacy/converter_puppetstates.txt");
	if (!puppetStates.is_open())
		throw std::runtime_error("Could not create puppet states history file!");

	std::ofstream unions("output" / theConfiguration.getOutputName() / "history/diplomacy/converter_unions.txt");
	if (!unions.is_open())
		throw std::runtime_error("Could not create unions history file!");

	for (const auto& agreement: agreements)
	{
		// Let's skip dead nations.
		if (countries.at(agreement->getFirst())->getProvinces().empty())
			continue;
		if (countries.at(agreement->getSecond())->getProvinces().empty())
			continue;

		if (agreement->getType() == "guarantee")
		{
			guarantees << *agreement;
		}
		else if (agreement->getType() == "union")
		{
			unions << *agreement;
		}
		else if (agreement->getType() == "vassal" || agreement->getType() == "dependency")
		{
			puppetStates << *agreement;
		}
		else if (agreement->getType() == "alliance")
		{
			alliances << *agreement;
		}
		else
		{
			Log(LogLevel::Warning) << "Cannot output diplomatic agreement type " << agreement->getType() << "!";
		}
	}

	alliances.close();
	guarantees.close();
	puppetStates.close();
	unions.close();

	if (invasion)
	{
		// Blank american diplomacy
		std::ofstream diplo("output" / theConfiguration.getOutputName() / "history/diplomacy/American_alliances.txt");
		diplo << "\n";
		diplo.close();
		// and move over our alliances.
		fs::copy_file("configurables/sunset/history/diplomacy/SunsetInvasion.txt",
			 "output" / theConfiguration.getOutputName() / "history/diplomacy/SunsetInvasion.txt",
			 fs::copy_options::overwrite_existing);
	}
}

void EU4::World::outputFlags(const Configuration& theConfiguration) const
{
	auto counter = 0;
	if (!fs::exists("flags.tmp") || !fs::is_directory("flags.tmp"))
	{
		Log(LogLevel::Warning) << "Flag folder flags.tmp not found!";
		return;
	}
	for (const auto& filename: commonItems::GetAllFilesInFolder("flags.tmp"))
	{
		if (fs::copy_file("flags.tmp" / filename, "output" / theConfiguration.getOutputName() / "gfx/flags" / filename, fs::copy_options::overwrite_existing))
			++counter;
	}
	Log(LogLevel::Info) << "<< " << counter << " flags copied.";
}
