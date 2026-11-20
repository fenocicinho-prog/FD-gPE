# FD gPE

DAW gratuit et open source (Linux et Windows) avec un navigateur de plugins en un clic.
Voir la fiche technique (`docs/`) pour la vision, l'architecture et les jalons.

**État : jalons M2 et M3 en cours (partiel)** : pistes audio et MIDI, transport, enregistrement,
mixage de base et mixeur, piano roll, carte de tempo, marqueurs, forme d'onde, édition des clips,
plugins VST3 et intégrés en tuiles, annuler/rétablir et export WAV. Thème gris clair légèrement
brillant + bleu, dans l'esprit Firefox.

Vérification syntaxique du 19/11/2026 : `bash scripts/verifier.sh --tout` → 10/10 sources OK,
aucun avertissement. Compilation locale réussie le 19/11/2026 avec `bash scripts/build_local.sh`
(1 seul cœur, priorité basse, sans risque pour 4 Go de RAM). Le binaire n'est plus versionné :
il reste dans `build/` et une copie de sauvegarde est à conserver hors du dépôt.

Le logo est chargé automatiquement depuis `~/Documents/FD-gPE/logo.png` s'il existe ;
sinon un logo textuel s'affiche. Aucune image n'est fournie par le dépôt : tu prépares le fichier toi-même.

| Système | Statut |
|---|---|
| Ubuntu 24.04 / Xubuntu (GCC 13, CMake 3.28) | Compilé et lancé avec succès |
| Arch Linux | Non testé |
| Windows 10/11 | Non testé |

Choisis ton guide : [Xubuntu](#guide-xubuntu-principal) · [Arch Linux](#guide-arch-linux) · [Windows](#guide-windows)

---

## Guide Xubuntu (principal)

Ce guide marche aussi pour Ubuntu et la plupart des dérivés. Il a été validé sur Ubuntu 24.04.

### Étape 1 : vérifier le système

```
lsb_release -ds
cmake --version     # il faut 3.22 ou plus
g++ --version       # il faut un GCC qui gère le C++20 (GCC 11 ou plus)
```

Si CMake est trop ancien, installe une version récente avec `pip install --user cmake`
ou via le site de CMake.

### Étape 2 : installer les paquets

```
sudo apt update
sudo apt install -y build-essential git cmake ninja-build pkg-config ccache \
    libasound2-dev libfreetype-dev libfontconfig1-dev libx11-dev libxrandr-dev \
    libxinerama-dev libxcursor-dev libxext-dev libxi-dev libxcomposite-dev \
    libxrender-dev libgl-dev libcurl4-openssl-dev
```

Sur Xubuntu 22.04, remplace `libfreetype-dev` par `libfreetype6-dev`.

### Étape 3 : récupérer les dépendances (une seule fois)

Les dépendances sont deux sous-modules git, **épinglés** sur des versions testées ensemble.
Ne lance pas `git submodule update --recursive` : le sous-module JUCE de Tracktion utilise
une URL SSH. C'est pourquoi on ajoute JUCE nous-mêmes, en HTTPS.

Depuis la racine du dépôt :

```
git submodule add https://github.com/Tracktion/tracktion_engine.git third_party/tracktion_engine
git -C third_party/tracktion_engine checkout c6293127cdc0a8488aea2ae0907f4709ee89c228

git submodule add https://github.com/juce-framework/JUCE.git third_party/JUCE
git -C third_party/JUCE checkout 7ff644f65d7daa1428c34b9738ebb5c449dc9172

git add .gitmodules third_party
git commit -m "Ajout des dépendances JUCE et Tracktion Engine"
```

Quelqu'un qui clone ensuite le dépôt fait seulement :

```
git submodule update --init
```

(sans `--recursive`).

### Étape 4 : compiler

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
cmake --build build -j$(nproc)
```

- La première compilation est longue (de plusieurs minutes à plus d'une demi-heure selon
  ta machine). Les suivantes sont rapides, surtout avec `ccache`.
- Le premier `cmake -S . -B build` prend aussi un moment : JUCE prépare ses outils.
- Si ta machine a peu de mémoire (8 Go ou moins) et que la compilation se bloque ou est
  tuée, limite les tâches en parallèle : `cmake --build build -j2`.

### Étape 5 : lancer

```
./build/app/FDgPE_artefacts/Debug/FDgPE
```

### Étape 6 : vérifier le jalon M0

- La fenêtre « FD gPE » s'ouvre.
- Elle affiche la version de Tracktion Engine et un périphérique audio.
- **Lecture / Stop** fait avancer puis arrêter la position.
- **Réglages audio** ouvre le choix du périphérique, de la fréquence et du tampon.

### Audio sous Xubuntu

- Si l'écran affiche « Périphérique audio : aucun », ouvre **Réglages audio** et choisis
  un périphérique (ALSA, ou celui de PipeWire/PulseAudio selon ton système).
- Vérifie que ta carte son est vue : `aplay -l`.
- Les messages `ALSA lib seq_hw.c ... open /dev/snd/seq failed` ou
  `JUCE Assertion failure in juce_Midi_linux.cpp` peuvent apparaître quand il n'y a pas de
  MIDI sur la machine. Ils sont sans gravité pour le jalon M0.

### Dépannage Xubuntu

| Symptôme | Cause et solution |
|---|---|
| `third_party/... est vide ou absent` | Refais l'étape 3. |
| `X11/extensions/XInput2.h: No such file` | Paquet manquant : `sudo apt install libxi-dev`. |
| `undefined reference to __atomic_store` | Déjà géré dans `app/CMakeLists.txt` (`libatomic`). Si tu le revois, récupère la dernière version du fichier. |
| `Package 'egl'`, `glesv2`, `webkit2gtk` not found | Messages d'information de JUCE, sans gravité : on n'utilise pas ces fonctions. |
| Compilation tuée ou machine qui gèle | Manque de mémoire : `cmake --build build -j1`. |

Pour toute autre erreur de compilation, envoie-la en entier : les fichiers corrigés
seront renvoyés complets.

---

## Guide Arch Linux

Arch fournit des compilateurs très récents. Ce guide n'a pas été testé sur Arch : si une
erreur de compilation apparaît, envoie-la en entier.

### Paquets

```
sudo pacman -S --needed base-devel git cmake ninja pkgconf ccache \
    alsa-lib freetype2 fontconfig libx11 libxrandr libxinerama libxcursor libxext \
    libxi libxcomposite libxrender mesa curl pipewire-alsa
```

### Ensuite

Les étapes **3 à 6** sont identiques à celles du guide Xubuntu :
dépendances git, compilation, lancement, vérification.

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
cmake --build build -j$(nproc)
./build/app/FDgPE_artefacts/Debug/FDgPE
```

### Particularités Arch

- Audio : PipeWire est le serveur de son habituel. `pipewire-alsa` permet à l'application
  de le voir comme un périphérique ALSA. Choisis-le dans **Réglages audio**.
- Mises à jour : après une grosse mise à jour système, supprime le dossier `build/` et
  recompile depuis zéro si CMake se plaint d'un ancien compilateur.

---

## Guide Windows

Ce guide n'a pas été testé sur Windows : si une étape échoue, envoie le message d'erreur
en entier.

### Étape 1 : installer les outils

Dans PowerShell (ou avec les installateurs graphiques de chaque outil) :

```
winget install --id Git.Git -e
winget install --id Kitware.CMake -e
winget install --id Ninja-build.Ninja -e
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

Visual Studio 2022 (ou plus récent) complet convient aussi : il faut la charge de travail
« Développement Desktop en C++ ». Ferme et rouvre ensuite ton terminal.

### Étape 2 : dossier court et chemins longs

Windows limite les chemins à 260 caractères, et JUCE/Tracktion ont des chemins profonds.

- Mets le projet dans un dossier court, par exemple `C:\dev\fd-gpe`.
- Dans un PowerShell **administrateur** :

```
git config --system core.longpaths true
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force
```

### Étape 3 : ouvrir le bon terminal

Ouvre **x64 Native Tools Command Prompt for VS 2022** (menu Démarrer). Ce terminal sait
trouver le compilateur. Un PowerShell ou un cmd normal ne suffit pas.

```
cd C:\dev\fd-gpe
```

### Étape 4 : récupérer les dépendances (une seule fois)

Mêmes commandes que l'étape 3 du guide Xubuntu :

```
git submodule add https://github.com/Tracktion/tracktion_engine.git third_party/tracktion_engine
git -C third_party/tracktion_engine checkout c6293127cdc0a8488aea2ae0907f4709ee89c228

git submodule add https://github.com/juce-framework/JUCE.git third_party/JUCE
git -C third_party/JUCE checkout 7ff644f65d7daa1428c34b9738ebb5c449dc9172

git add .gitmodules third_party
git commit -m "Ajout des dépendances JUCE et Tracktion Engine"
```

Pour un clone existant : `git submodule update --init` (sans `--recursive`).

### Étape 5 : compiler et lancer

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
build\app\FDgPE_artefacts\Debug\FDgPE.exe
```

### Étape 6 : vérifier le jalon M0

Mêmes critères que sous Xubuntu. L'audio utilise le pilote WASAPI de Windows. Si aucun
périphérique n'est affiché, ouvre **Réglages audio** et choisis ta sortie.

### Dépannage Windows

| Symptôme | Cause et solution |
|---|---|
| `cl` ou `cmake` introuvable | Tu n'es pas dans le « x64 Native Tools Command Prompt », ou tu n'as pas rouvert le terminal après l'installation. |
| Erreur « path too long » ou fichier introuvable | Refais l'étape 2 (dossier court et chemins longs). |
| `third_party/... est vide ou absent` | Refais l'étape 4. |

---

## Utiliser et tester

Au premier lancement, un projet est créé dans `~/Documents/FD-gPE/Projet.tracktionedit`
avec une piste **Audio 1** et une piste **MIDI 1**. Il est sauvegardé à chaque arrêt
d'enregistrement, quand tu ajoutes ou supprimes une piste, et à la fermeture.

### Prise en main

| Pour faire ça | Fais ça |
|---|---|
| Lecture / stop | Bouton **▶** (triangle, au survol « Lancer ou arrêter la lecture »), ou la barre d'espace |
| Revenir au début | Bouton **\|<** |
| Enregistrer | Arme une piste avec **ARM**, puis le bouton **●** (disque rouge). L'enregistrement refuse de démarrer si aucune piste n'est armée |
| Enregistrer de l'audio | Piste **AUDIO** armée : entrée 1 de la carte son (mono) |
| Enregistrer du MIDI | Piste **MIDI** armée : bouton **CLAVIER** de la piste (clavier horizontal jouable), touches A W S E D F T G Y H U J du clavier de l'ordinateur, ou clavier MIDI branché |
| Boucle / métronome | Bouton **↻** (flèche circulaire, boucle de 4 mesures depuis la position courante) et **MÉTRO**. Pour régler la boucle : glisser les deux petites poignées violettes en bas de la règle (avec magnétisme selon le réglage) |
| Tempo | Boutons **-** et **+** à côté de TEMPO |
| Déplacer la tête de lecture | Clic ou glissé sur la règle (en dehors des poignées de boucle), ou clic dans une piste |
| Zoom / défilement | Ctrl + molette = zoom ; Maj + molette = défilement horizontal |
| Renommer une piste | Double-clic sur son nom |
| Remettre un curseur à zéro | Double-clic sur VOL ou PAN |
| Ajouter / supprimer une piste | Bouton **+** (menu **Piste audio** / **Piste MIDI**), bouton **X** |
| Nouveau / ouvrir / enregistrer sous | Bouton **PROJET…**, ou **Ctrl+N** / **Ctrl+O** / **Ctrl+Maj+S** |
| Importer un fichier audio | **PROJET… → Importer**, **Ctrl+I**, ou clic droit sur une piste |
| Dupliquer une piste | **PROJET… → Dupliquer**, **Ctrl+D**, ou clic droit sur une piste |
| Ouvrir le mixeur vertical | **MENU → Mixeur** (VOL, PAN, MUTE par piste) |
| Changer le magnétisme | **MENU → Magnétisme** : OFF, 1/16, 1/8, 1/4, 1/2, 1 mesure |
| Ajouter un clip MIDI | Double-clic dans une piste MIDI, ou clic droit → **Nouveau clip MIDI ici** |
| Couleur d'une piste | Clic droit sur la piste → **Changer la couleur…** |
| Déplacer / redimensionner un clip | Glisser le clip ; saisir son bord droit pour changer sa durée |
| Étirer / réduire un clip **audio** | Clic droit sur le clip → **ÉTIRER ×1,5** ou **RÉDUIRE ×0,667** : la durée change, la hauteur du son est conservée (SoundTouch). Les clips MIDI ne sont pas concernés. |
| Couper un clip **MIDI** | Clic droit sur le clip MIDI → **COUPER LES NOTES À LA TÊTE** : les notes après la tête de lecture sont supprimées ou raccourcies, et le clip s'arrête à la tête. **Ctrl+Z** annule. |
| Faire de la place dans un clip **MIDI** | Clic droit sur le clip MIDI → **FAIRE DE LA PLACE (+1 TEMPS)** : les notes après la tête de lecture sont décalées d'un temps ; une note à cheval est divisée et le clip s'allonge. **Ctrl+Z** annule. Les clips bouclés ne sont pas concernés. |
|Faire un beat de batterie|**PROJET… → Séquenceur de pas…** : cliquez les pads (6 instruments × 16 pas) ; la piste « Batterie » et son clip d'une mesure sont créés au premier clic. **LECTURE** écoute le motif, **EFFACER** vide la grille, **FERMER** replie le panneau|
|Charger ses propres sons|Dans le séquenceur, cliquez le nom d'un instrument (GROSSE CAISSE, etc.) puis choisissez un WAV/AIFF : il remplace le synthé de la piste « Batterie » et est joué par les pads. Le son reste associé à la note MIDI du pad et est enregistré dans le projet.|
|Changer l'apparence|**PROJET… → Apparence…** : choisissez **CLAIR** ou **SOMBRE** et un accent (**JAUNE FANÉ** ou **VERT CLAIR**). Le changement est immédiat et enregistré : il reste actif au prochain lancement. Le violet des bordures et témoins suit l'accent. Le mode sombre est en gris neutre (jamais noir).|
|Éditer des notes MIDI|Double-clic sur un clip MIDI, ou **MENU → Piano roll** : l'éditeur s'ouvre intégré au niveau de la piste ; **FERMER** pour le replier|
|Jouer une note MIDI|Bouton **CLAVIER** de la piste MIDI, ou touches de l'ordinateur : la note part vers la piste MIDI sélectionnée|
|Revenir en arrière dans le temps|Faire défiler avec la molette sur la règle, ou cliquer sur la règle pour recentrer la tête de lecture|
|Créer un clip MIDI vide|**MENU → Piano roll** sans clip sélectionné : un clip de 4 mesures est créé à la tête de lecture sur la première piste MIDI|
|Gérer le tempo et les signatures|**MENU → Carte de tempo** : ajout/suppression de points, 4/4, 3/4, 6/8, etc.|
|Ajouter un marqueur|**+ MARQUEUR**, ou touche **M**|
|Aller au marqueur suivant / précédent|**M>** / **<M**|
|Ouvrir le navigateur de plugins|**BIBLIOTHÈQUE**, **MENU → Bibliothèque de plugins**, touche **P**, ou tuile **+** sur une piste|
|Éditer / activer / retirer un plugin|Tuile du plugin dans l'en-tête de piste : clic = éditeur, point = ON/OFF, **x** = retrait|
|Couper / dupliquer / supprimer un clip|Clic droit sur le clip ; ou touche **S** (coupe à la tête de lecture), **Suppr**, **Ctrl+D**|
|Copier / coller un clip|**Ctrl+C** puis **Ctrl+V** : collage à la tête de lecture sur la piste sélectionnée|
|Exporter le mixage|**MENU → Export WAV**, ou **Ctrl+E**|
|Annuler / rétablir|**ANNULER** / **RÉTABLIR**, **Ctrl+Z** / **Ctrl+Maj+Z** / **Ctrl+Y**|
|Enregistrer le projet|**Ctrl+S** (également automatique après ajout/suppression de piste ou de clip)|

Les témoins bleus sont les petits voyants allumés (lecture, boucle, métronome). Le voyant
rouge de REC indique l'enregistrement. Pendant l'enregistrement, un bloc rouge grandit sur
la piste armée ; le clip définitif apparaît quand tu arrêtes.

### Piano roll

- Double-clic sur une case vide : créer une note.
- Glisser une note : déplacer en hauteur et dans le temps.
- Glisser le bord droit d'une note (petite poignée) : ajuster sa durée, sans déplacer son début.
- Clic droit sur une note : la supprimer. **Suppr** supprime la note sélectionnée.
- Quantification : 1/4, 1/8, 1/16, 1/32.
- Ctrl + molette : zoom horizontal ; Maj + molette : défilement.

### Plugins

La bibliothèque (**BIBLIOTHÈQUE**) affiche les plugins intégrés Tracktion et les VST3 scannés.
Le scan VST3 se lance depuis le bouton **SCAN** ; le bouton **DOSSIER** ajoute un chemin
supplémentaire. Les tuiles sont classées par Instruments / Effets / Externes, avec recherche.
Un clic sur **+** ajoute le plugin à la piste choisie.

### Critères de réussite

- Enregistrer quelques secondes d'audio, arrêter, rejouer : le clip est entendu sans craquements.
- Sur la piste MIDI, cliquer sur **CLAVIER** : le synthé intégré sonne. Enregistrer, rejouer.
- MUTE et SOLO coupent ou isolent les pistes ; VOL et PAN s'entendent.
- Le vu-mètre de la piste bouge pendant la lecture.
- Fermer et rouvrir le programme : les pistes et les clips sont toujours là.

### Limites connues

- Un seul armement à la fois (armer une piste désarme les autres).
- Une seule entrée audio (entrée 1, mono), sans écoute directe, pour éviter le larsen.
- Les formes d'onde audio sont chargées à la demande ; sur un très gros projet, l'affichage peut prendre un instant.
- L'export WAV est synchrone : l'interface peut se figer quelques instants sur un long projet.
- Le scan VST3 n'est pas hors processus : un plugin qui plante peut faire tomber l'application. Pas de LV2 ni de CLAP pour l'instant.
- Le magnétisme et les bornes de boucle à la souris sont disponibles ; copier/coller des clips se fait par duplication.
- Le séquenceur de pas, le sampler à pads, les thèmes interchangeables et les bus d'envoi ne sont pas encore implémentés.
- Catalogue en ligne, installateur, signatures, comptes et packs de sons (jalons M4+) arrivent plus tard.

## Plusieurs systèmes sur le même ordinateur

- Ne partage pas le dossier `build/` entre deux systèmes : CMake mémorise le compilateur
  et les chemins. Utilise un dossier par système : `-B build-xubuntu`, `-B build-arch`.
  Ils sont ignorés par git (voir `.gitignore`).
- Le code et git sont communs : tu modifies sur un système, tu fais un commit, tu récupères
  sur l'autre.

## Structure

> **Sons du séquenceur** : les fichiers WAV/AIFF chargés restent à leur emplacement d'origine
> (le projet ne fait que référencer leur chemin). Ne les déplacez pas et ne les supprimez pas,
> sinon les pads concernés deviendront silencieux après réouverture du projet.

> **Apparence** : certaines fenêtres déjà ouvertes (piano roll, navigateur de plugins, mixeur) gardent
> la couleur de leurs libellés jusqu'à leur réouverture ; le reste de l'interface change immédiatement.

```
CMakeLists.txt        projet racine (JUCE + Tracktion + app)
app/
  CMakeLists.txt      cible FDgPE
  src/Main.cpp        point d'entrée JUCE
  src/engine/         EngineHost : seul endroit qui parle à Tracktion
  src/ui/             interface : style (FdLookAndFeel), transport, pistes, règle,
                      navigateur de plugins, piano roll, tempo, fenêtre principale
docs/                 fiche technique
third_party/          sous-modules (JUCE, Tracktion), épinglés
```
