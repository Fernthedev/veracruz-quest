#include "main.hpp"

#include "i18n.hpp"
#include "langs.hpp"

#include <fmt/core.h>

using namespace Veracruz;

using ModLocaleMap = std::unordered_map<ModKeyPtr, const Localization>;

static std::unordered_map<const LangKey, ModLocaleMap> registeredLocales;
static std::unordered_map<ModKeyPtr, LanguageSelectedEvent> languageLoadedEvents;
static BasicLanguageSelectedEvent basicLanguageLoadedEvent;

static ModLocaleMap const* selectedLanguageMap; // optional?
static std::optional<Lang> selectedLanguage;

void LocalizationHandler::Register(ModKey info,
                                   LocaleMap const &locale) {
    for (auto const& [lang, localization] : locale) {
        Register(info, lang, localization);
    }
}

void LocalizationHandler::Register(ModInfo const &info, LangKey const& lang, Localization const& locale) {
    ModLocaleMap& modLocaleMap = registeredLocales[lang];

    fmtLog(Logging::INFO, "Registering mod {} for language {}:{}", info.id, lang.langName, lang.region);

    if (modLocaleMap.contains(info)) {
        fmtThrowError("Mod key for id {} is already registered", info.id);
    }

    modLocaleMap.try_emplace(info, locale);
}


void LocalizationHandler::Unregister(ModInfo const &info) {
    for (auto const& [lang, map] : registeredLocales) {
        Unregister(info, lang);
    }
}

void LocalizationHandler::Unregister(ModInfo const &info, LangKey const& langKey) {
    fmtLog(Logging::INFO, "Unregistering mod {} for language {}:{}", info.id, langKey.langName, langKey.region);

    registeredLocales[langKey].erase(info);
}

static void SelectLanguageInternal(std::optional<std::reference_wrapper<Lang const>> langOpt) {
    if (langOpt) {
        Lang const& lang = *langOpt;
        auto it = registeredLocales.find(lang);

        if (it == registeredLocales.end()) {
            fmtThrowError("Language not recognized {0}:{1}", lang.langName, lang.region);
        }

        fmtLog(Logging::INFO, "Language selected {0}:{1}", lang.langName, lang.region);

        selectedLanguageMap = &it->second;
        selectedLanguage.emplace(lang);

        for (auto const& [mInfo, callback] : languageLoadedEvents) {
            if (callback.size() > 0) {
                auto localeIt = selectedLanguageMap->find(mInfo);

                if (localeIt != selectedLanguageMap->end()) {
                    callback.invoke(*selectedLanguage, std::cref(localeIt->second));
                } else {
                    callback.invoke(*selectedLanguage, std::nullopt);
                }
            }
        }
    } else {
        selectedLanguage = std::nullopt;
        selectedLanguageMap = {};
    }

    basicLanguageLoadedEvent.invoke(selectedLanguage);
}

void LocalizationHandler::SelectLanguage(Lang const &lang) {
    SelectLanguageInternal(lang);
}

void LocalizationHandler::UnSelectLanguage() {
    SelectLanguageInternal(std::nullopt);
}

std::unordered_set<LangKey> LocalizationHandler::GetLanguages() {
    std::unordered_set<LangKey> set;
    set.reserve(registeredLocales.size());

    for (auto const& [lang, _] : registeredLocales) {
        set.emplace(lang);
    }

    return set;
}

void LocalizationHandler::RegisterLanguage(LangKey const &lang) {
    registeredLocales.try_emplace(lang);
}

LangKey const &LocalizationHandler::GetSelectedLanguage() {
    return selectedLanguage.value();
}

bool LocalizationHandler::IsLanguageSelected() noexcept {
    return selectedLanguage.has_value();
}

std::optional<LangKey>
LocalizationHandler::FindSuitableFallback(ModInfo const &info, std::vector<LangKey> const &supportedLanguages) noexcept {
    if (selectedLanguageMap->contains(info)) {
        return *selectedLanguage;
    }

    for (auto const& lang : supportedLanguages) {
        auto langIt = registeredLocales.find(lang);
        if (langIt == registeredLocales.end()) continue;

        if (langIt->second.contains(info)) {
            return langIt->first;
        }
    }

    return std::nullopt;
}

std::optional<std::reference_wrapper<Localization const>>
LocalizationHandler::TryGetLocale(LangKey const &lang, ModKey info) {
    auto it = registeredLocales.find(lang);

    if (it == registeredLocales.end()) {
        return std::nullopt;
    }

    ModLocaleMap const& map = std::ref(it->second);

    auto mapIt = map.find(info);

    if (mapIt == map.end()) {
        return std::nullopt;
    }

    return std::cref(mapIt->second);
}

std::optional<std::reference_wrapper<Localization const>> LocalizationHandler::TryGetCurrentLocale(ModKey info) {
    if (!selectedLanguageMap) {
        return std::nullopt;
    }

    auto it = selectedLanguageMap->find(info);

    if (it == selectedLanguageMap->end()) {
        return std::nullopt;
    }

    return std::cref(it->second);
}

Localization const & LocalizationHandler::GetCurrentLocale(ModKey info) {
    if (!selectedLanguageMap) {
        fmtThrowError("Language not recognized");
    }

    auto it = selectedLanguageMap->find(info);

    if (it == selectedLanguageMap->end()) {
        fmtThrowError("Mod {} is not registered to language {}:{}", info.id, selectedLanguage->langName, selectedLanguage->region);
    }

    return it->second;
}

LanguageSelectedEvent &LocalizationHandler::GetLocaleEventHandler(ModInfo const &info) {
    return languageLoadedEvents[info];
}

BasicLanguageSelectedEvent& LocalizationHandler::GetBasicLocaleEventHandler() {
    return basicLanguageLoadedEvent;
}
