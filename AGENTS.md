# FD gPE : consignes pour les assistants IA (opencode, Claude Code, etc.)

Lis ce fichier en entier avant de toucher au code. Il évite de tout ré-explorer à chaque session.

## Le projet

FD gPE est un DAW de musique **gratuit et open source** (Linux et Windows), pensé pour les beatmakers
et les mixeurs audio, avec à terme un navigateur de plugins en un clic. Langage : **C++20**.
Moteur : **Tracktion Engine + JUCE**, épinglés en sous-modules git (`third_party/`).
Réponds en **français**, de façon concise.

Objectif : offrir les grandes fonctions d'un DAW grand public (comme n-Track Studio) avec une interface
**originale** et l'installation de plugins en un clic. On s'inspire des fonctions et du flux de travail ;
on ne copie **jamais** d'icônes, de textes, de noms, d'images ni de code d'un logiciel existant.

Avancement et prochaines tâches : **`docs/ROADMAP.md`** (une tâche à la fois, dans l'ordre).
Fiche technique complète : `docs/`. Prompt et rôles des agents : `docs/PROMPT_OPENCODE.md`, `.opencode/agents/`.

## Contrainte matérielle : très importante

La machine de développement a **4 Go de RAM, un CPU à 2,6 GHz, pas de GPU**, et une connexion limitée
(1 Go par jour). Donc :

- **Ne lance jamais la compilation complète** (`cmake --build`, `ninja`) : elle sature la mémoire.
  Vérifie ton code avec **`bash scripts/verifier.sh`** (syntaxe seulement, quelques secondes par fichier).
  La vraie compilation se fait sur GitHub Actions (voir plus bas) ou à la main par l'utilisateur.
- **Ne modifie pas** les options de compilation, les `target_compile_definitions`, la version de C++
  ni les sous-modules sans demander : cela force à tout recompiler (des heures).
- N'ajoute pas de dépendance sans demander. Ne touche pas à `third_party/`.
- Ne lis que les fichiers dont tu as besoin. Ne colle pas de longs journaux : donne les 20 lignes utiles.
- Fais des modifications **petites et ciblées**. Une tâche = un sujet.

## Ne jamais inventer une API

Avant d'utiliser une fonction de Tracktion ou de JUCE, cherche sa signature dans les en-têtes de `third_party/`
(`grep -rn "nomDeLaFonction" third_party/tracktion_engine/modules third_party/JUCE/modules | head`).
Si tu ne la trouves pas, n'écris pas de code qui l'utilise : dis-le et propose une alternative.
Tracktion change entre les versions : seule la version épinglée dans `third_party/` fait foi.

## Boucle de travail

1. Modifier le code (fichiers complets et cohérents).
2. `bash scripts/verifier.sh` (contrôle les .cpp modifiés ; `--tout` pour tous).
3. Corriger jusqu'à « Tout compile. »
4. `git commit` puis `git push` : GitHub Actions compile sous Linux et Windows (`.github/workflows/build.yml`).
5. Si la compilation échoue sur GitHub, lis le journal, corrige, recommence.

## Architecture (app/src/)

| Dossier | Rôle |
|---|---|
| `engine/EngineHost.*` | **Seul endroit qui parle à Tracktion** : projet, pistes, armement, transport, clavier virtuel. Le reste du code n'appelle pas Tracktion directement (sauf `te::AudioTrack&` passé aux pistes). |
| `ui/FdLookAndFeel.*`, `ui/Palette.h` | Apparence : thème sombre plat, couleurs, boutons/icônes, curseurs. Toutes les couleurs passent par `Palette.h`. |
| `ui/TransportBar.*` | Barre du haut compacte (une rangée, 48 px) : transport, position, tempo, statut. |
| `ui/TrackRow.*` | Une piste compacte (en-tête 196x72) : ARM, MUTE, SOLO, VOL, vu-mètre, bouton CLAV./PAS, clips colorés par piste. |
| `ui/MixerDock.*` | Mixeur ancré en bas : tranches par piste (nom, effets verticaux, pan, fader, vu-mètre, M/S). `mixerDock->clear()` avant toute suppression/reconstruction de piste. |
| `ui/TimelineRuler.*`, `ui/TimelineModel.h` | Règle, tête de lecture, échelle partagée. |
| `ui/MainComponent.*` | Fenêtre principale, mise en page, barre d'état en bas, minuteur 20 Hz, éditeurs intégrés. |

Le projet utilisateur est `~/Documents/FD-gPE/Projet.tracktionedit`. Le type d'une piste (audio ou MIDI)
est stocké dans la propriété `fdgpeKind` de l'état de la piste.

## Règles de code

- **Thread audio** : aucune allocation, aucun verrou, aucun accès disque ni réseau. Le chargement d'images,
  de fichiers et les requêtes réseau se font ailleurs.
- Les textes de l'interface sont en français, en UTF-8, via `tr("...")` (`ui/Text.h`).
  Les libellés de boutons sont écrits **en majuscules à la main** (JUCE ne met pas les accents en majuscules).
- Détruire les lignes de pistes (`rows.clear()`) **avant** de supprimer une piste (vu-mètres branchés dessus).
- Fichiers et chemins utilisateur : `juce::File`, pas `std::filesystem`.
- Code compatible **Linux et Windows**. Pas de code propre à un système sans garde `#if JUCE_LINUX` / `JUCE_WINDOWS`.
- Avertissements traités sérieusement (le projet compile sans avertissement).

## Style de l'interface

- **Thème sombre à plat** (maquette `original-3ea3acc16cd6fe9c43c50cd78a7fdde0.webp`) : aplats, **sans dégradé ni ombre**. Accent vert clair ou jaune clair (choix utilisateur), rouge pour l'enregistrement, vert pour les vu-mètres.
- **Barre du haut compacte** : une seule rangée de 48 px (transport, position, tempo, statut/menu).
- **Pistes compactes** : en-tête 196x72, clips colorés par piste, bouton CLAV./PAS selon le type (rythme = séquenceur de pas).
- **MixerDock ancré en bas** : tranches par piste, effets en liste verticale, pan, fader, vu-mètre, MUTE/SOLO. Toujours appeler `mixerDock->clear()` avant de supprimer/reconstruire les pistes.
- **Barre d'état en bas** : messages courts d'état.
- Boutons/icônes plats, petits et nets ; pas de listes de texte brut. Zones cliquables d'au moins 44 px quand c'est possible.
- Toutes les couleurs passent par `ui/Palette.h`.

## Règle permanente : fidélité à la maquette

Toute l'interface doit ressembler de plus en plus à la maquette `original-3ea3acc16cd6fe9c43c50cd78a7fdde0.webp` (racine du projet), et ne jamais s'en éloigner :

- Avant chaque tâche qui touche à l'interface, ouvre la maquette et repère l'élément concerné (forme, couleurs, tailles, espacements, icônes, typographie).
- Après chaque tâche, fais la liste courte « ce qui ressemble » et « ce qui diffère encore » par rapport à la maquette, et rapproche l'interface de la maquette sur ce qui diffère.
- Tout nouvel élément (navigateur de fichiers, fenêtres de plugins, groupes de pistes, envois/bus, tranche Master, etc.) suit le style de la maquette : sombre plat, accent vert clair, aplats sans dégradé ni ombre, petites icônes nettes, mixeur façon console.
- Si un choix de la maquette est ambigu, pose-moi la question au lieu d'inventer.
- Aucun changement ne doit rendre l'interface moins proche de la maquette qu'avant.

## Sécurité et licence

- Projet open source (copyleft, compatible JUCE/Tracktion). Jamais de clé d'API, mot de passe ni donnée
  personnelle dans le dépôt.
- Le futur catalogue de plugins ne stocke aucun binaire : métadonnées signées uniquement (voir la fiche).

## Remise des fichiers

Quand on te demande une correction, renvoie le **fichier complet** corrigé (pas un extrait ni un diff),
et seulement les fichiers qui changent.
