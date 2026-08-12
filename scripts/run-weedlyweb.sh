#!/usr/bin/env bash
# Lanceur universel WeedlyWeb — X11 / Wayland / KDE / GNOME / XFCE / LXQt / dwm / Lubuntu / Xubuntu…
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXECUTABLE=""

resolve_executable() {
    if [[ -n "${WEEDLYWEB_BIN:-}" && -x "${WEEDLYWEB_BIN}" ]]; then
        EXECUTABLE="${WEEDLYWEB_BIN}"
        return 0
    fi

    local ui="${WEEDLYWEB_UI:-gtk}"
    local candidates=()
    case "${ui}" in
        qt|Qt|QT)
            candidates=(
                "${ROOT_DIR}/build/WeedlyWebQt"
                "${ROOT_DIR}/build/src/ui/qt/WeedlyWebQt"
                "${ROOT_DIR}/build/ui-qt/build/WeedlyWebQt"
                "/usr/local/bin/WeedlyWebQt"
            )
            ;;
        *)
            candidates=(
                "${ROOT_DIR}/build/WeedlyWeb"
                "${ROOT_DIR}/build/src/ui/gtk/WeedlyWeb"
                "/usr/local/bin/WeedlyWeb"
                "/usr/bin/WeedlyWeb"
            )
            ;;
    esac

    local c
    for c in "${candidates[@]}"; do
        if [[ -x "$c" ]]; then
            EXECUTABLE="$c"
            return 0
        fi
    done
    return 1
}

detect_session() {
    if [[ -n "${WAYLAND_DISPLAY:-}" ]]; then
        echo "wayland"
    elif [[ -n "${DISPLAY:-}" ]]; then
        echo "x11"
    else
        echo "none"
    fi
}

detect_desktop() {
    echo "${XDG_CURRENT_DESKTOP:-${DESKTOP_SESSION:-unknown}}"
}

apply_safe_runtime_env() {
    # Stables sur la majorité des WM/DE (dwm, i3, XFCE, LXQt, Plasma, GNOME, Lubuntu…)
    export WEBKIT_DISABLE_COMPOSITING_MODE="${WEBKIT_DISABLE_COMPOSITING_MODE:-1}"
    export WEBKIT_DISABLE_DMABUF_RENDERER="${WEBKIT_DISABLE_DMABUF_RENDERER:-1}"

    # Accélération GPU : forcée soft par défaut (évite GBM/segfault sur beaucoup de configs)
    # Désactiver avec : WEEDLYWEB_ALLOW_GPU=1 make run
    if [[ "${WEEDLYWEB_ALLOW_GPU:-0}" != "1" ]]; then
        export LIBGL_ALWAYS_SOFTWARE="${LIBGL_ALWAYS_SOFTWARE:-1}"
        export MESA_GL_VERSION_OVERRIDE="${MESA_GL_VERSION_OVERRIDE:-3.3}"
    fi

    # Réduit les erreurs a11y sur WM minimalistes (dwm, awesome, openbox…)
    export NO_AT_BRIDGE="${NO_AT_BRIDGE:-1}"

    # Évite certains plantages GStreamer/WebKit média hors environnement complet
    export WEBKIT_GST_USE_PLAYBIN3="${WEBKIT_GST_USE_PLAYBIN3:-1}"
}

build_backend_list() {
    local session="$1"
    local backends=()

    # Forçage explicite uniquement (évite qu'un GDK_BACKEND ambient bloque les fallbacks)
    if [[ -n "${WEEDLYWEB_GDK_BACKEND:-}" ]]; then
        echo "${WEEDLYWEB_GDK_BACKEND}"
        return 0
    fi

    case "$session" in
        wayland)
            # Wayland natif puis XWayland (KDE/GNOME/Hyprland/sway…)
            backends+=(wayland x11)
            ;;
        x11)
            # X11 pur (XFCE, LXQt/Lubuntu, dwm, i3, MATE, Xubuntu…)
            backends+=(x11)
            ;;
        *)
            backends+=(x11 wayland)
            ;;
    esac

    # Si l'utilisateur a un GDK_BACKEND ambient, le tenter en premier puis les autres
    if [[ -n "${GDK_BACKEND:-}" ]]; then
        backends=("${GDK_BACKEND}" "${backends[@]}")
    fi

    # Déduplique en préservant l'ordre
    local out=() seen=""
    local b
    for b in "${backends[@]}"; do
        case " $seen " in
            *" $b "*) ;;
            *)
                out+=("$b")
                seen+=" $b"
                ;;
        esac
    done
    printf '%s\n' "${out[@]}"
}

print_diagnostics() {
    echo "── Diagnostic affichage ──"
    echo "  Session     : $(detect_session)"
    echo "  Bureau      : $(detect_desktop)"
    echo "  DISPLAY     : ${DISPLAY:-<vide>}"
    echo "  WAYLAND     : ${WAYLAND_DISPLAY:-<vide>}"
    echo "  XDG_SESSION : ${XDG_SESSION_TYPE:-<vide>}"
    echo "  GDK_BACKEND : ${GDK_BACKEND:-<auto>} (forcer: WEEDLYWEB_GDK_BACKEND=x11|wayland)"
    echo "  Exécutable  : ${EXECUTABLE}"
    echo "─────────────────────────"
}

if ! resolve_executable; then
    echo "❌ WeedlyWeb introuvable. Compilez d'abord : cmake -B build -S . && cmake --build build"
    exit 127
fi

SESSION="$(detect_session)"
if [[ "$SESSION" == "none" ]]; then
    echo "❌ Aucun serveur d'affichage détecté (ni DISPLAY ni WAYLAND_DISPLAY)."
    echo "   Démarrez une session graphique (X11, Wayland, Xwayland…)."
    exit 1
fi

apply_safe_runtime_env
print_diagnostics

mapfile -t BACKENDS < <(build_backend_list "$SESSION")
LAST_CODE=1

for backend in "${BACKENDS[@]}"; do
    echo "▶️  Essai GDK_BACKEND=${backend} …"
    # Sous-shell pour ne pas polluer le parent si plusieurs essais
    (
        export GDK_BACKEND="${backend}"
        exec "${EXECUTABLE}" "$@"
    )
    LAST_CODE=$?

    if [[ $LAST_CODE -eq 0 ]]; then
        exit 0
    fi

    # 130 = Ctrl+C, 143 = SIGTERM — ne pas retenter
    if [[ $LAST_CODE -eq 130 || $LAST_CODE -eq 143 ]]; then
        exit "$LAST_CODE"
    fi

    echo "⚠️  Échec avec backend « ${backend} » (code ${LAST_CODE})."
done

echo ""
echo "❌ Impossible de démarrer WeedlyWeb sur les backends testés : ${BACKENDS[*]}"
echo "💡 Pistes :"
echo "   - X11     : echo \$DISPLAY  | paquet xorg-xwayland si session Wayland"
echo "   - Wayland : echo \$WAYLAND_DISPLAY | qt/gtk wayland"
echo "   - Forcer  : WEEDLYWEB_GDK_BACKEND=x11 ${0}"
echo "   - GPU     : WEEDLYWEB_ALLOW_GPU=1 ${0}   # sinon soft GL par défaut"
echo "   - Deps    : ./scripts/install-deps.sh"
exit "$LAST_CODE"
