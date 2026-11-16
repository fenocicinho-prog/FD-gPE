# FD gPE

DAW gratuit et open source (Linux et Windows) avec un navigateur de plugins en un clic.
Voir la fiche technique pour la vision, l'architecture et les jalons.

**État : jalon M0** — une fenêtre s'ouvre et le moteur Tracktion démarre.

## 1. Prérequis

**Arch Linux**

```
sudo pacman -S --needed base-devel git cmake ninja pkgconf ccache \
    alsa-lib freetype2 fontconfig libx11 libxrandr libxinerama libxcursor libxext \
    libxi libxcomposite libxrender mesa curl
```

**Windows 10/11**

- Visual Studio 2022 (ou Build Tools) avec la charge de travail « Développement Desktop en C++ »
- CMake 3.22 ou plus récent, Git

## 2. Récupérer les dépendances (une seule fois)

Les dépendances sont deux sous-modules git, **épinglés** sur des versions testées ensemble.
Ne lance pas `git submodule update --recursive` : le sous-module JUCE de Tracktion utilise
une URL SSH, c'est pourquoi on ajoute JUCE nous-mêmes en HTTPS.

Depuis la racine du dépôt :

```
git submodule add https://github.com/Tracktion/tracktion_engine.git third_party/tracktion_engine
git -C third_party/tracktion_engine checkout c6293127cdc0a8488aea2ae0907f4709ee89c228

git submodule add https://github.com/juce-framework/JUCE.git third_party/JUCE
git -C third_party/JUCE checkout 7ff644f65d7daa1428c34b9738ebb5c449dc9172

git add .gitmodules third_party
```

Quelqu'un qui clone ensuite le dépôt fait seulement : `git submodule update --init`
(sans `--recursive`).

## 3. Compiler

**Linux**

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
cmake --build build
./build/app/FDgPE_artefacts/Debug/FDgPE
```

**Windows** (Developer PowerShell for VS 2022)

```
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug
.\build\app\FDgPE_artefacts\Debug\FDgPE.exe
```

La première compilation est longue (Tracktion et JUCE sont de gros modules) ; les suivantes
sont rapides. Le premier `cmake -S . -B build` prend aussi un moment (JUCE prépare ses outils).

## 4. Critère de réussite du jalon M0

- La fenêtre « FD gPE » s'ouvre.
- Elle affiche la version de Tracktion Engine et un périphérique audio.
- **Lecture / Stop** fait avancer puis arrêter la position.
- **Réglages audio** ouvre le choix du périphérique, de la fréquence et du tampon.

## 5. Dépannage

- *« third_party/... est vide »* : refais l'étape 2.
- *Périphérique audio « aucun »* sous Linux : vérifie `aplay -l`. Avec PipeWire, installe
  `pipewire-alsa` et `pipewire-jack`, puis choisis le périphérique dans Réglages audio.
- Envoie-moi les erreurs de compilation en entier, je renverrai les fichiers corrigés complets.

## Structure

```
CMakeLists.txt        projet racine (JUCE + Tracktion + app)
app/
  CMakeLists.txt      cible FDgPE
  src/Main.cpp        point d'entrée JUCE
  src/engine/         EngineHost : seul endroit qui parle à Tracktion
  src/ui/             fenêtre principale (écran M0)
third_party/          sous-modules (JUCE, Tracktion), épinglés
```
