#!/usr/bin/env bash
# build_local.sh : compile FD gPE sur la machine de développement (4 Go de RAM).
#
# Pourquoi : l'assistant ne peut pas lancer la compilation complète (règle AGENTS.md),
# et GitHub ne veut pas du binaire de 211 Mo. Ce script est fait pour être lancé
# par l'utilisateur, sans risque de faire planter le PC.
#
# Sécurité mémoire :
#   - un seul cœur de compilation (--parallel 1) ;
#   - priorité basse (nice 19 + ionice classe 3 si disponible) ;
#   - contrôle de la RAM disponible avant de commencer ;
#   - le swap existant n'est pas modifié ;
#   - aucune option CMake, aucune cible et aucun sous-module ne sont changés.
#
# Usage :  bash scripts/build_local.sh
#          FDGPE_BUILD_LOW_MEM=1 bash scripts/build_local.sh   # force malgré peu de RAM

set -euo pipefail
cd "$(dirname "$0")/.."

BUILD="${BUILD:-build}"
BIN="$BUILD/app/FDgPE_artefacts/Debug/FDgPE"

if [ ! -f "$BUILD/build.ninja" ]; then
    echo "ERREUR : $BUILD n'est pas configuré."
    echo "Lance une fois : cmake -S . -B $BUILD -G Ninja -DCMAKE_BUILD_TYPE=Debug"
    exit 1
fi

echo "=== État avant compilation ==="
free -h
df -h .
if [ -f "$BIN" ]; then
    echo "Binaire actuel (ancien) : $(stat -c '%y' "$BIN")"
fi

dispo_kb="$(awk '/^MemAvailable:/ {print $2}' /proc/meminfo 2>/dev/null || true)"
if [ -n "$dispo_kb" ] && [ "$dispo_kb" -lt 700000 ]; then
    echo ""
    echo "ATTENTION : moins de 700 Mo de RAM réellement disponible."
    echo "Ferme des applications (navigateur, autres compilations) avant de continuer."
    if [ "${FDGPE_BUILD_LOW_MEM:-0}" != "1" ]; then
        echo "Compilation annulée. Pour forcer : FDGPE_BUILD_LOW_MEM=1 bash scripts/build_local.sh"
        exit 1
    fi
    echo "Forçage demandé : on continue à tes risques."
fi

echo ""
echo "=== Compilation : 1 cœur, priorité basse (cela peut prendre un certain temps) ==="

commande=(nice -n 19)
if command -v ionice >/dev/null 2>&1; then
    commande+=(ionice -c 3)
fi
commande+=(cmake --build "$BUILD" --target FDgPE --parallel 1)

"${commande[@]}"

echo ""
echo "=== Compilation terminée ==="
if [ -f "$BIN" ]; then
    ls -lh "$BIN"
    echo "Binaire : $BIN"
else
    echo "Attention : binaire introuvable à l'emplacement attendu ($BIN)."
fi
echo "Tu peux maintenant lancer : $BIN"
