#!/usr/bin/env bash
# verifier.sh : vérifie vite que le code compile, SANS tout recompiler ni lier le programme.
#
# Pourquoi : la compilation complète est lourde (JUCE + Tracktion). Ici on ne contrôle que
# la syntaxe des fichiers que tu viens de modifier, avec les vrais réglages du projet.
# Ça prend de quelques secondes à une minute par fichier et ça demande peu de mémoire.
#
# Usage (depuis la racine du projet) :
#     bash scripts/verifier.sh                    fichiers .cpp modifiés (git) ; tous si aucun repéré
#     bash scripts/verifier.sh --tout             tous les .cpp de app/src
#     bash scripts/verifier.sh app/src/ui/TrackRow.cpp ...   fichiers précis
#
# Prérequis : avoir lancé une fois   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
# (le dossier de build peut être changé avec  BUILD=build-xubuntu bash scripts/verifier.sh).

set -euo pipefail
cd "$(dirname "$0")/.."

BUILD="${BUILD:-build}"

if [ ! -f "$BUILD/compile_commands.json" ]; then
    echo "Il manque $BUILD/compile_commands.json."
    echo "Lance d'abord : cmake -S . -B $BUILD -G Ninja -DCMAKE_BUILD_TYPE=Debug"
    exit 1
fi

toutes_les_sources() {
    find app/src -name '*.cpp' | sort
}

fichiers=()

if [ "${1:-}" = "--tout" ]; then
    mapfile -t fichiers < <(toutes_les_sources)
elif [ $# -gt 0 ]; then
    fichiers=("$@")
elif git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    # fichiers modifiés ou nouveaux, par rapport au dernier commit
    mapfile -t changes < <( { git diff --name-only HEAD 2>/dev/null; git ls-files --others --exclude-standard; } | sort -u )

    headers_changes=0
    for f in "${changes[@]}"; do
        case "$f" in
            app/src/*.cpp) fichiers+=("$f") ;;
            app/src/*.h)   headers_changes=1 ;;
        esac
    done

    # un en-tête modifié peut casser n'importe quel .cpp qui l'inclut : on vérifie tout
    if [ "$headers_changes" = 1 ]; then
        mapfile -t fichiers < <(toutes_les_sources)
        echo "(un fichier .h a changé : vérification de tous les .cpp)"
    fi
fi

if [ "${#fichiers[@]}" -eq 0 ]; then
    mapfile -t fichiers < <(toutes_les_sources)
    echo "(aucune modification repérée : vérification de tous les .cpp)"
fi

export BUILD
printf '%s\n' "${fichiers[@]}" | nice -n 10 python3 -c '
import json, os, shlex, subprocess, sys, time

build = os.environ["BUILD"]
with open(os.path.join(build, "compile_commands.json")) as f:
    commandes = {os.path.realpath(e["file"]): e for e in json.load(f)}

erreurs = 0
for ligne in sys.stdin:
    chemin = ligne.strip()
    if not chemin:
        continue
    entree = commandes.get(os.path.realpath(chemin))
    if entree is None:
        print(f"?  {chemin} : absent du projet CMake (relance cmake -S . -B {build})")
        erreurs += 1
        continue

    brut = shlex.split(entree["command"])
    cmd, saute = [], False
    for p in brut:
        if saute:
            saute = False
            continue
        if p in ("-MT", "-MF"):
            saute = True
            continue
        if p == "-MD":
            continue
        cmd.append(p)
    i = cmd.index("-o")
    cmd[i + 1] = os.devnull
    cmd.append("-fsyntax-only")

    debut = time.time()
    r = subprocess.run(cmd, cwd=entree["directory"], capture_output=True, text=True)
    duree = round(time.time() - debut)
    sortie = (r.stderr or r.stdout).strip()

    if r.returncode == 0:
        print(f"OK {chemin} ({duree} s)")
        if sortie:
            print("\n".join(sortie.splitlines()[:40]))
    else:
        erreurs += 1
        print(f"ERREUR {chemin} ({duree} s)")
        print("\n".join(sortie.splitlines()[:80]))

print()
print("Tout compile." if erreurs == 0 else f"{erreurs} fichier(s) avec des erreurs.")
sys.exit(1 if erreurs else 0)
'
