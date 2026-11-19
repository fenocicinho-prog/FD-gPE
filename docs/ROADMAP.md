# Feuille de route FD gPE (pour les assistants IA)

**Règles** : une tâche à la fois, dans l'ordre. Coche la case seulement quand la tâche est terminée
(code vérifié avec `scripts/verifier.sh`, CI verte, critère de réussite atteint par l'utilisateur).
Chaque tâche doit rester petite : 1 à 3 fichiers modifiés. Si une tâche est trop grosse, découpe-la.

**Inspiration** : les grandes fonctions d'un DAW grand public comme n-Track Studio (voir son manuel public
pour les idées). On reprend les idées et le flux de travail, jamais ses icônes, textes, noms, images ni code.

## Fait

- [x] M0 : fenêtre et moteur Tracktion.
- [x] M1 : pistes audio et MIDI, transport, enregistrement, mixage de base (VOL, PAN, MUTE, SOLO, vu-mètre),
      règle, clavier à l'écran, sauvegarde du projet.
- [x] M2.1 : forme d'onde dans les clips audio (miniatures Tracktion).
- [x] M3.2 : insertion de plugins en tuiles (nom, catégorie, badge INTÉGRÉ/VST3) et ouverture de l'éditeur.
- [x] M3.3 : effets intégrés proposés en tuiles (égaliseur, compresseur, réverbération, délai, filtre, etc.).

Vérification du 19/11/2026 : `bash scripts/verifier.sh --tout` → 10/10 sources OK, sans avertissement.
Compilation locale réussie avec `bash scripts/build_local.sh` (1 cœur, priorité basse, binaire 217 Mo).
Test visuel M2.1 fait : forme d'onde visible dans le clip audio (capture : `/tmp/opencode/capture_m21_finale.png`).
Le son, le matériel et le zoom à la souris restent à valider par l'utilisateur.

Correctifs du 19/11 (à valider par l'utilisateur) : l'arrêt de l'enregistrement ne jette plus le fichier
enregistré (`TransportControl::stop(false, false)`) ; le suivi de la tête de lecture peut être coupé par
un défilement manuel et `seek()` recentre la vue pour revenir en arrière ; les pistes MIDI reçoivent
les notes du clavier à l'écran via leur device MIDI propre, avec réparation des projets sans synthé ;
le piano roll s'ouvre intégré au niveau de la piste au double-clic sur un clip MIDI.

Ajouts du 19/11 (à valider) : le clavier horizontal du bas est retiré ; chaque piste MIDI a un bouton
**CLAVIER** qui ouvre un clavier jouable attaché à cette piste ; le clavier dessiné dans le piano roll
joue la note sur le synthé de la piste (aperçu audible). Les touches de l'ordinateur gardent leur
routage vers la piste MIDI sélectionnée.

Correctif du 19/11 : le clavier est désormais horizontal (paysage) et ses notes, ainsi que
celles du piano roll, passent par le device clavier virtuel affecté à la piste : le son
arrive même sur une piste vide, transport arrêté. Les devices MIDI propres aux pistes sont désactivés
(ils provoquaient un doublon de noeuds TrackMidiInputDeviceNode et un plantage au démarrage) ; seul le
device clavier virtuel, en monitorage permanent, alimente les synthés. Fenêtres de piste fermées avant
toute suppression (évite un use-after-free). À valider à l'écoute.

## M2 : édition (rendre l'enregistrement utile)

- [x] **M2.1 Forme d'onde** dans les clips audio (miniatures Tracktion). *Critère* : un clip enregistré montre sa
      forme d'onde, qui reste nette quand on zoome.
- [ ] **M2.2 Édition des clips** : sélection, déplacement à la souris, coupe à la tête de lecture (touche S),
      suppression, copier/coller, annuler/rétablir (Ctrl+Z, Ctrl+Y). *Critère* : on réorganise un
      enregistrement sans rien perdre, et Ctrl+Z revient en arrière. *Raccourcis S (coupe), Suppr et
      Ctrl+C/Ctrl+V ajoutés le 19/11 (collage à la tête sur la piste sélectionnée). Build local OK ;
      validation utilisateur en attente.*
      *Interface du 19/11 : police classique (sans-serif) au lieu de monospace ; bouton **+** unique
      proposant Piste audio / Piste MIDI ; outils regroupés dans **MENU** (bibliothèque, piano roll,
      carte de tempo, export, réglages audio, mixeur, magnétisme) avec **BIBLIOTHÈQUE** à côté.*
- [ ] **M2.3 Grille et boucle** : magnétisme sur la grille (mesures, temps), début et fin de boucle réglables
      sur la règle. *Critère* : un clip déplacé « accroche » les temps ; la boucle se règle à la souris.
      *Partiel : magnétisme fait, bornes de boucle pas encore réglables à la souris.*
- [ ] **M2.4 Piano roll** : ouvrir un clip MIDI, ajouter, déplacer, redimensionner, supprimer des notes,
      vélocité, quantification. *Critère* : on écrit un motif à la souris et on l'entend.
      *Partiel : création, déplacement, suppression, vélocité et quantification faits ; redimensionnement d'une note existante manquant.*
- [ ] **M2.5 Séquenceur de pas** : grille de 16 pas avec pads, pour les beats. *Critère* : on fait un beat de
      batterie en cliquant les pas.
- [ ] **M2.6 Sampler** : charger des WAV sur des pads, kits de batterie. *Critère* : on joue et on séquence ses
      propres sons.
- [ ] **M2.7 Thèmes de couleurs** : `Palette` devient un thème modifiable (sombre, clair, choix de la couleur
      d'accent), enregistré dans les préférences. *Critère* : on change la couleur d'accent sans recompiler.

## M3 : plugins

- [ ] **M3.1** Hébergement VST3 (LV2 à vérifier selon les versions de JUCE/Tracktion), scan hors processus,
      liste noire des plugins qui plantent. *Partiel : VST3 scanné et intégré, mais scan dans le processus et sans liste noire.*
- [x] **M3.2** Insérer un plugin sur une piste avec une tuile (image + nom) ; ouvrir sa fenêtre.
- [x] **M3.3** Effets intégrés (égaliseur, compresseur, réverbération, délai) proposés en tuiles.
- [ ] **M3.4** Bus d'envoi, piste master, mixeur vertical. *Partiel : volume général et mixeur vertical faits ; bus d'envoi manquants.*

## M4 : installation en un clic (en local)

- [ ] **M4.1** Manifeste JSON et vérification de signature (Ed25519, Monocypher), avec tests unitaires.
- [ ] **M4.2** `fd-installer` (petit exécutable séparé) : téléchargement, vérification SHA-256, décompression
      sans écraser, protection contre « zip slip », retour en arrière en cas d'échec.
- [ ] **M4.3** Dialogue « Ajouter » en tuiles (onglets Instruments, Effets, Extensions), images en cache
      et images par défaut.

## M5 à M7 : serveur et contenus

- [ ] **M5** Serveur de catalogue sur Vercel : recherche filtrée selon l'appareil, entrées signées, aucun binaire.
- [ ] **M6** Comptes vérifiés par e-mail, jeton hors ligne.
- [ ] **M7** Extensions : packs de sons et presets.

## Plus tard (par priorité)

Export FLAC, automation (courbes), fondus et enchaînés, étirement temporel et changement de hauteur,
raccourcis clavier personnalisables, MIDI learn, accordeur, détection de tempo, vidéo simple avec son mixé,
IA via API, format CLAP, pont Wine pour les plugins Windows.
