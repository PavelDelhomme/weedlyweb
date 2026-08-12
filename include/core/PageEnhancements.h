#ifndef WEEDLYWEB_CORE_PAGE_ENHANCEMENTS_H
#define WEEDLYWEB_CORE_PAGE_ENHANCEMENTS_H

namespace PageEnhancements {

/** Active un thème sombre forcé (approche type Dark Reader : invert + hue-rotate). */
const char* darkModeEnableScript();

/** Retire le thème sombre injecté. */
const char* darkModeDisableScript();

/** Active le mode lecture (extrait l’article, typographie épurée). */
const char* readerModeEnableScript();

/** Indique si le mode lecture est actuellement injecté (retourne "1"/"0" via evaluate — non utilisé ici). */
const char* readerModeIsActiveScript();

} // namespace PageEnhancements

#endif
