RUSSIAN LOCALIZATION (option "russian_localization" = 2)

alphabet.txt
  Two lines, 33 space separated glyphs each: first for upper case А..Я (with Ё after Е), second for lower case а..я.
  These are the characters the unofficial Russian localization mod for EU4 redraws as Cyrillic letters.
  Change them here if your Russian mod uses a different set. Keep the file encoded as UTF-8, glyphs may be control characters.

localisation/<same sub folder as in the mod>/<name>_l_russian.yml
  Plain Cyrillic translations of the mod's own texts. Each file is matched with <name>_l_english.yml in the output mod;
  every key found here replaces the English text of that key. Keys without translation stay in English.
  Write text in normal Cyrillic UTF-8, the converter converts it to the mod's glyphs. Avoid characters outside Cyrillic, Latin and basic punctuation.

Texts that must stay Latin (character names, province names, unit names) are never taken from here: any Cyrillic that ends up in
history files is transliterated (Яков Латыпов -> Jakov Latypov).
