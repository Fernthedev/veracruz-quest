#include "main.hpp"

#include "i18n.hpp"
#include "langs.hpp"

#include <fmt/core.h>

using namespace Veracruz;

using ModLocaleMap = std::unordered_map<ModKeyPtr, const Localization>;

static std::unordered_map<const LangKey, ModLocaleMap> registeredLocales;
static std::unordered_map<ModKeyPtr, LanguageSelectedEvent> languageLoadedEvents;

static ModLocaleMap const* selectedLanguageMap; // optional?
static std::optional<Lang> selectedLanguage;

void LocalizationHandler::Register(ModKey info,
                                   LocaleMap const &locale) {
    for (auto const& [lang, localization] : locale) {
        Register(info, lang, localization);
    }
}

void LocalizationHandler::Register(ModInfo const &info, LangKey const& lang, Localization const& locale) {
    ModKeyPtr modKey = &info;
    ModLocaleMap& modLocaleMap = registeredLocales[lang];

    fmtLog(Logging::INFO, "Registering mod {} for language {}:{}", info.id, lang.langName, lang.region);

    if (modLocaleMap.contains(modKey)) {
        fmtThrowError("Mod key for id {} is already registered", info.id);
        throw std::runtime_error(fmt::format("Mod key for id {} is already registered", info.id));
    }

    modLocaleMap.try_emplace(modKey, locale);
}


void LocalizationHandler::Unregister(ModInfo const &info) {
    for (auto const& [lang, map] : registeredLocales) {
        Unregister(info, lang);
    }
}

void LocalizationHandler::Unregister(ModInfo const &info, LangKey const& langKey) {
    fmtLog(Logging::INFO, "Unregistering mod {} for language {}:{}", info.id, langKey.langName, langKey.region);

    registeredLocales[langKey].erase(&info);
}


void LocalizationHandler::SelectLanguage(Lang const &lang) {
    auto it = registeredLocales.find(lang);

    if (it == registeredLocales.end()) {
        fmtThrowError("Language not recognized {0}:{1}", lang.langName, lang.region);
    }

    fmtLog(Logging::INFO, "Language selected {0}:{1}", lang.langName, lang.region);

    selectedLanguageMap = &it->second;
    selectedLanguage.emplace(lang);

    // todo: const
    for (auto& [modInfo, callback] : languageLoadedEvents) {
        if (callback.size() > 0) {
            Localization const& locale = selectedLanguageMap->at(modInfo);
            callback.invoke(*selectedLanguage, locale);
        }
    }
}

LangKey const &LocalizationHandler::GetSelectedLanguage() {
    return selectedLanguage.value();
}

Localization const & LocalizationHandler::GetCurrentLocale(ModKey info) {
    if (!selectedLanguageMap) {
        fmtThrowError("Language not recognized");
    }

    auto it = selectedLanguageMap->find(&info);

    if (it == selectedLanguageMap->end()) {
        fmtThrowError("Mod {} is not registered to language {}:{}", info.id, selectedLanguage->langName, selectedLanguage->region);
    }

    return it->second;
}

LanguageSelectedEvent &LocalizationHandler::GetLocaleEventHandler(ModInfo const &info) {
    return languageLoadedEvents[&info];
}