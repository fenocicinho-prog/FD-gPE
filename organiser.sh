#!/usr/bin/env bash
# organiser.sh : range les fichiers FD gPE téléchargés "en vrac" dans la bonne arborescence.
#
# À lancer depuis la racine du projet.
#
#   bash organiser.sh                              range les fichiers qui sont déjà dans le dossier du projet
#   bash organiser.sh --depuis ~/Downloads         va chercher les fichiers récents du dossier Téléchargements
#   bash organiser.sh --depuis ~/Downloads --remplacer
#                                                  remplace aussi les fichiers déjà en place (mises à jour)
#   bash organiser.sh --dry-run ...                montre ce qui serait fait, sans rien déplacer
#
# Avec --depuis, seuls les fichiers de moins de 3 heures sont pris en compte
# (option --minutes N pour changer). Le script ne supprime rien d'autre que
# l'ancienne version d'un fichier remplacé : fais un commit git avant de remplacer.

set -euo pipefail
shopt -s nullglob

DRY=0
REMPLACER=0
MINUTES=180
SRC=""
PROJET="."

while [ $# -gt 0 ]; do
    case "$1" in
        -n|--dry-run)   DRY=1 ;;
        -r|--remplacer) REMPLACER=1 ;;
        --depuis)       shift; SRC="${1:?--depuis demande un dossier}" ;;
        --minutes)      shift; MINUTES="${1:?--minutes demande un nombre}" ;;
        *)              PROJET="$1" ;;
    esac
    shift
done

PROJET="$(cd "$PROJET" && pwd)"
[ -z "$SRC" ] && SRC="$PROJET"
SRC="$(cd "$SRC" && pwd)"
cd "$PROJET"

echo "Projet  : $PROJET"
[ "$SRC" != "$PROJET" ] && echo "Source  : $SRC (fichiers de moins de $MINUTES minutes)"
[ "$DRY" = 1 ] && echo "(mode simulation : rien ne sera déplacé)"
[ "$REMPLACER" = 1 ] && echo "(mode remplacement : les fichiers existants seront remplacés)"
echo

# placer <fichier source> <destination relative au projet>
placer() {
    local src="$1" dest="$2"
    local nom
    nom="$(basename "$src")"

    if [ "$src" -ef "$dest" ]; then
        return 0
    fi

    if [ -e "$dest" ] && [ "$REMPLACER" = 0 ]; then
        echo "  ! $dest existe déjà : « $nom » laissé en place (utilise --remplacer pour le mettre à jour)"
        return 0
    fi

    if [ -e "$dest" ]; then
        echo "  $nom  =>  $dest   (remplacé)"
    else
        echo "  $nom  ->  $dest"
    fi

    if [ "$DRY" = 0 ]; then
        mkdir -p "$(dirname "$dest")"
        mv -f -- "$src" "$dest"
    fi
}

# --- Fichiers reconnus par leur nom ------------------------------------------
declare -A DEST=(
    [_.gitignore]=".gitignore"
    [README.md]="README.md"
    [AGENTS.md]="AGENTS.md"
    [build.yml]=".github/workflows/build.yml"
    [verifier.sh]="scripts/verifier.sh"
    [ROADMAP.md]="docs/ROADMAP.md"
    [PROMPT_OPENCODE.md]="docs/PROMPT_OPENCODE.md"
    [chef.md]=".opencode/agents/chef.md"
    [architecte.md]=".opencode/agents/architecte.md"
    [codeur.md]=".opencode/agents/codeur.md"
    [relecteur.md]=".opencode/agents/relecteur.md"
    [interface.md]=".opencode/agents/interface.md"
    [ci.md]=".opencode/agents/ci.md"
    [redacteur.md]=".opencode/agents/redacteur.md"
    [Main.cpp]="app/src/Main.cpp"
    [EngineHost.h]="app/src/engine/EngineHost.h"
    [EngineHost.cpp]="app/src/engine/EngineHost.cpp"
    [MainWindow.h]="app/src/ui/MainWindow.h"
    [MainComponent.h]="app/src/ui/MainComponent.h"
    [MainComponent.cpp]="app/src/ui/MainComponent.cpp"
    [Palette.h]="app/src/ui/Palette.h"
    [Text.h]="app/src/ui/Text.h"
    [FdLookAndFeel.h]="app/src/ui/FdLookAndFeel.h"
    [FdLookAndFeel.cpp]="app/src/ui/FdLookAndFeel.cpp"
    [TimelineModel.h]="app/src/ui/TimelineModel.h"
    [TimelineRuler.h]="app/src/ui/TimelineRuler.h"
    [TimelineRuler.cpp]="app/src/ui/TimelineRuler.cpp"
    [TrackRow.h]="app/src/ui/TrackRow.h"
    [TrackRow.cpp]="app/src/ui/TrackRow.cpp"
    [TransportBar.h]="app/src/ui/TransportBar.h"
    [TransportBar.cpp]="app/src/ui/TransportBar.cpp"
    [PluginBrowser.h]="app/src/ui/PluginBrowser.h"
    [PluginBrowser.cpp]="app/src/ui/PluginBrowser.cpp"
    [PianoRollEditor.h]="app/src/ui/PianoRollEditor.h"
    [PianoRollEditor.cpp]="app/src/ui/PianoRollEditor.cpp"
    [TempoDialog.h]="app/src/ui/TempoDialog.h"
    [TempoDialog.cpp]="app/src/ui/TempoDialog.cpp"
)

candidats() {
    if [ "$SRC" = "$PROJET" ]; then
        find "$SRC" -maxdepth 1 -type f -print0
    else
        find "$SRC" -maxdepth 1 -type f -mmin "-$MINUTES" -print0
    fi
}

echo "Rangement :"
trouve=0
while IFS= read -r -d '' f; do
    base="$(basename "$f")"

    # "Main(1).cpp" ou "Main (1).cpp" (doublons du navigateur) -> "Main.cpp"
    cle="$(printf '%s' "$base" | sed -E 's/ ?\([0-9]+\)(\.[^.]*)$/\1/')"

    if [ -n "${DEST[$cle]+x}" ]; then
        placer "$f" "${DEST[$cle]}"
        trouve=1
        continue
    fi

    case "$base" in
        *CMakeLists*)
            # Les deux CMakeLists.txt se reconnaissent à leur contenu :
            # celui de l'application contient "juce_add_gui_app", celui de la racine "project(FDgPE".
            if grep -q "juce_add_gui_app" "$f"; then
                placer "$f" "app/CMakeLists.txt"; trouve=1
            elif grep -q "project(FDgPE" "$f"; then
                placer "$f" "CMakeLists.txt"; trouve=1
            else
                echo "  ? $base : contenu non reconnu, laissé en place"
            fi
            ;;
        FD*[Ff]iche*.pdf)
            placer "$f" "docs/fiche-technique-v0.1.pdf"; trouve=1
            ;;
    esac
done < <(candidats)

[ "$trouve" = 0 ] && echo "  (aucun fichier à ranger)"

# --- Contrôles -----------------------------------------------------------------
echo
echo "Contrôles :"

if [ -e ".~lock.README.md#" ]; then
    echo "  ! README.md est encore ouvert dans LibreOffice : ferme-le, puis supprime le fichier « .~lock.README.md# »."
fi

attendus=(
    CMakeLists.txt README.md .gitignore app/CMakeLists.txt
    AGENTS.md .github/workflows/build.yml scripts/verifier.sh
    docs/ROADMAP.md docs/PROMPT_OPENCODE.md
    .opencode/agents/chef.md .opencode/agents/architecte.md .opencode/agents/codeur.md
    .opencode/agents/relecteur.md .opencode/agents/interface.md .opencode/agents/ci.md
    .opencode/agents/redacteur.md
    app/src/Main.cpp
    app/src/engine/EngineHost.h app/src/engine/EngineHost.cpp
    app/src/ui/MainWindow.h app/src/ui/MainComponent.h app/src/ui/MainComponent.cpp
    app/src/ui/Palette.h app/src/ui/Text.h
    app/src/ui/FdLookAndFeel.h app/src/ui/FdLookAndFeel.cpp
    app/src/ui/TimelineModel.h app/src/ui/TimelineRuler.h app/src/ui/TimelineRuler.cpp
    app/src/ui/TrackRow.h app/src/ui/TrackRow.cpp
    app/src/ui/TransportBar.h app/src/ui/TransportBar.cpp
)

if [ "$DRY" = 0 ]; then
    manque=0
    for f in "${attendus[@]}"; do
        if [ ! -e "$f" ]; then
            echo "  ! fichier manquant : $f"
            manque=1
        fi
    done
    [ "$manque" = 0 ] && echo "  Tous les fichiers du jalon M1 sont en place."

    if [ "$SRC" = "$PROJET" ]; then
        autres=()
        for f in * .[!.]*; do
            [ -f "$f" ] || continue
            case "$f" in
                CMakeLists.txt|README.md|AGENTS.md|.gitignore|organiser.sh|.~lock.*) ;;
                *) autres+=("$f") ;;
            esac
        done
        if [ "${#autres[@]}" -gt 0 ]; then
            echo "  Fichiers non reconnus, laissés à la racine :"
            printf '    - %s\n' "${autres[@]}"
        fi
    fi

    echo
    echo "Arborescence :"
    if command -v tree >/dev/null 2>&1; then
        tree -a -I '.git|third_party|build|build-*' --noreport
    else
        find . -path ./.git -prune -o -path ./third_party -prune -o -path './build*' -prune -o -print | sort
    fi

    if [ ! -e third_party/JUCE/CMakeLists.txt ] || [ ! -e third_party/tracktion_engine/CMakeLists.txt ]; then
        echo
        echo "Étape suivante : récupérer JUCE et Tracktion (README.md, guide de ton système)."
    fi
fi
