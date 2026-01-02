#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <windows.h>
#include <mmsystem.h> // API Windows Multimedia pour la lecture de son (PlaySound, MCI)
#include <time.h>

#define CODE_SIZE 5
#define MAX_ATTEMPTS 20
#define JOKER_THRESHOLD 10
#define FILENAME "scores.txt"
#define CONSOLE_WIDTH 60

// Codes de couleurs ANSI
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD    "\033[1m"
#define BG_RED     "\033[41m"
#define BG_GREEN   "\033[42m"
#define BG_YELLOW  "\033[43m"
#define BG_BLUE    "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN    "\033[46m"

// Caractères Unicode pour les boîtes
#define BOX_HORIZONTAL "\u2500"
#define BOX_VERTICAL   "\u2502"
#define BOX_TOP_LEFT   "\u250C"
#define BOX_TOP_RIGHT  "\u2510"
#define BOX_BOT_LEFT   "\u2514"
#define BOX_BOT_RIGHT  "\u2518"
#define BOX_CROSS      "\u253C"
#define BOX_T_LEFT     "\u251C"
#define BOX_T_RIGHT    "\u2524"
#define BOX_T_TOP      "\u252C"
#define BOX_T_BOT      "\u2534"

// Structure pour stocker les scores
typedef struct {
    char nom[50];
    int essais;
} Score;

// Fonction pour activer les couleurs ANSI sur Windows
void enable_colors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    
    // Configurer la console pour UTF-8
    SetConsoleOutputCP(65001);
}

// ==========================================================
// NOUVELLES FONCTIONS MCI POUR LE MP3
// ==========================================================

// Fonction pour arrêter la lecture MCI et libérer le fichier
void arreter_fichier_mci() {
    mciSendString("stop musique", NULL, 0, NULL);
    mciSendString("close musique", NULL, 0, NULL);
}

// Fonction pour jouer un fichier audio (MP3, WAV) avec l'API MCI
// Le fichier doit se trouver dans le même répertoire que l'exécutable.
void jouer_fichier_mci(const char *chemin_fichier) {
    // Arrêter et fermer toute instance MCI précédente
    arreter_fichier_mci(); 
    
    // Commande pour ouvrir le fichier
    char commande_ouverte[256];
    // Utilisation de "MPEGVIDEO" pour les MP3
    sprintf(commande_ouverte, "open \"%s\" alias musique type mpegvideo", chemin_fichier);
    mciSendString(commande_ouverte, NULL, 0, NULL);

    // Commande pour jouer de manière asynchrone (non bloquant)
    mciSendString("play musique", NULL, 0, NULL);
}

// ==========================================================

// Fonction pour jouer un son simple
void jouer_son(int frequence, int duree) {
    Beep(frequence, duree);
}

// NOTE: L'ancienne fonction jouer_melodie_victoire() a été supprimée.

// Fonction pour jouer une melodie d'erreur (utilisée lors d'aucun match)
void jouer_melodie_erreur() {
    Beep(200, 300);
    Sleep(100);
    Beep(150, 400);
}

// Fonction pour jouer une melodie de succes (utilisée lors d'un ou plusieurs bien placés)
void jouer_melodie_succes() {
    Beep(600, 150);
    Sleep(50);
    Beep(800, 200);
}

// Fonction pour jouer une melodie de joker
void jouer_melodie_joker() {
    Beep(800, 200);
    Sleep(50);
    Beep(1000, 200);
    Sleep(50);
    Beep(1200, 300);
}

// Fonction pour centrer le texte
void centrer_texte(const char *texte, int largeur) {
    int len = strlen(texte);
    int padding = (largeur - len) / 2;
    for (int i = 0; i < padding; i++) {
        printf(" ");
    }
    printf("%s", texte);
}

// Fonction pour afficher une ligne horizontale
void afficher_ligne_horizontale(int largeur, const char *style) {
    for (int i = 0; i < largeur; i++) {
        printf("%s", style);
    }
    printf("\n");
}

// Fonction pour calculer la longueur réelle du texte (sans codes ANSI)
int longueur_texte_reelle(const char *texte) {
    int len = 0;
    int dans_code = 0;
    for (int i = 0; texte[i] != '\0'; i++) {
        if (texte[i] == '\033') {
            dans_code = 1;
        } else if (dans_code && texte[i] == 'm') {
            dans_code = 0;
        } else if (!dans_code) {
            len++;
        }
    }
    return len;
}

// Fonction pour afficher une boite avec texte
void afficher_boite(const char *texte, const char *couleur, int largeur) {
    int len = longueur_texte_reelle(texte);
    int padding = (largeur - len - 2) / 2;
    
    printf("%s", couleur);
    printf(BOX_TOP_LEFT);
    for (int i = 0; i < largeur - 2; i++) printf(BOX_HORIZONTAL);
    printf(BOX_TOP_RIGHT RESET "\n");
    
    printf("%s" BOX_VERTICAL RESET, couleur);
    for (int i = 0; i < padding; i++) printf(" ");
    printf("%s%s%s", couleur, texte, RESET);
    for (int i = 0; i < largeur - len - padding - 2; i++) printf(" ");
    printf("%s" BOX_VERTICAL RESET "\n", couleur);
    
    printf("%s", couleur);
    printf(BOX_BOT_LEFT);
    for (int i = 0; i < largeur - 2; i++) printf(BOX_HORIZONTAL);
    printf(BOX_BOT_RIGHT RESET "\n");
}

// Fonction pour afficher un texte avec animation
void afficher_texte_anime(const char *texte, int delai) {
    for (int i = 0; texte[i] != '\0'; i++) {
        printf("%c", texte[i]);
        fflush(stdout);
        Sleep(delai);
    }
}

// Fonction pour afficher une barre de progression
void afficher_barre_progression(int actuel, int maximum, int largeur) {
    int rempli = (actuel * largeur) / maximum;
    printf(CYAN "[");
    for (int i = 0; i < largeur; i++) {
        if (i < rempli) {
            printf(GREEN BOLD "=" RESET);
        } else {
            printf(" ");
        }
    }
    printf(CYAN "] %d/%d\n" RESET, actuel, maximum);
}

// Fonction pour effacer l'ecran
void clear_screen() {
    system("cls");
}

// Fonction pour afficher le titre
void afficher_titre() {
    printf("\n");
    printf(CYAN BOLD);
    afficher_boite("* M A S T E R M I N D  *", MAGENTA BOLD, CONSOLE_WIDTH);
    printf(RESET "\n");
}

// Fonction pour verifier si tous les chiffres sont distincts
bool tous_distincts(const char *code) {
    for (int i = 0; i < CODE_SIZE; i++) {
        for (int j = i + 1; j < CODE_SIZE; j++) {
            if (code[i] == code[j]) {
                return false;
            }
        }
    }
    return true;
}

// Fonction pour valider un code selon le niveau
bool valider_code(const char *code, int niveau) {
    // Verifier la longueur
    if (strlen(code) != CODE_SIZE) {
        return false;
    }
    
    // Verifier que ce sont des chiffres
    for (int i = 0; i < CODE_SIZE; i++) {
        if (!isdigit(code[i])) {
            return false;
        }
    }
    
    // Pour le niveau 1, verifier que tous les chiffres sont distincts
    if (niveau == 1) {
        return tous_distincts(code);
    }
    
    return true;
}

// Fonction pour lire un code depuis le clavier
void lire_code(char *code, const char *message) {
    printf("%s", message);
    scanf("%s", code);
    // Nettoyer le buffer si necessaire
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Fonction pour generer un code secret aleatoire
void generer_code_secret(char *code_secret, int niveau) {
    srand((unsigned int)time(NULL));
    
    if (niveau == 1) {
        // Niveau debutant: tous les chiffres distincts
        int chiffres_disponibles[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        int nb_disponibles = 10;
        
        for (int i = 0; i < CODE_SIZE; i++) {
            int index = rand() % nb_disponibles;
            code_secret[i] = '0' + chiffres_disponibles[index];
            
            // Retirer le chiffre utilise
            for (int j = index; j < nb_disponibles - 1; j++) {
                chiffres_disponibles[j] = chiffres_disponibles[j + 1];
            }
            nb_disponibles--;
        }
    } else {
        // Niveau expert: chiffres peuvent se repeter
        for (int i = 0; i < CODE_SIZE; i++) {
            code_secret[i] = '0' + (rand() % 10);
        }
    }
    
    code_secret[CODE_SIZE] = '\0';
}

// Fonction pour compter les chiffres bien places et mal places
void comparer_codes(const char *code_secret, const char *code_essai, int *bien_places, int *mal_places) {
    *bien_places = 0;
    *mal_places = 0;
    
    // Tableaux pour marquer les chiffres deja utilises
    bool secret_utilise[CODE_SIZE];
    bool essai_utilise[CODE_SIZE];
    
    // Initialiser les tableaux
    for (int i = 0; i < CODE_SIZE; i++) {
        secret_utilise[i] = false;
        essai_utilise[i] = false;
    }
    
    // Compter les chiffres bien places
    for (int i = 0; i < CODE_SIZE; i++) {
        if (code_secret[i] == code_essai[i]) {
            (*bien_places)++;
            secret_utilise[i] = true;
            essai_utilise[i] = true;
        }
    }
    
    // Compter les chiffres mal places
    for (int i = 0; i < CODE_SIZE; i++) {
        if (!essai_utilise[i]) {
            for (int j = 0; j < CODE_SIZE; j++) {
                if (!secret_utilise[j] && code_essai[i] == code_secret[j]) {
                    (*mal_places)++;
                    secret_utilise[j] = true;
                    break;
                }
            }
        }
    }
}

// Fonction pour afficher le resultat d'une tentative
void afficher_resultat(int essai, const char *code_essai, int bien_places, int mal_places) {
    printf("\n");
    printf(CYAN BOX_TOP_LEFT);
    for (int i = 0; i < CONSOLE_WIDTH - 2; i++) printf(BOX_HORIZONTAL);
    printf(BOX_TOP_RIGHT RESET "\n");
    
    printf(CYAN BOX_VERTICAL RESET);
    printf(BOLD " Essai %d : " RESET, essai);
    printf(YELLOW BOLD "%s" RESET, code_essai);
    printf(" -> ");
    
    if (bien_places > 0) {
        printf(GREEN BOLD "[+] %d bien place(s)" RESET, bien_places);
        jouer_melodie_succes();
    }
    if (mal_places > 0) {
        if (bien_places > 0) printf(" | ");
        printf(YELLOW BOLD "[O] %d mal place(s)" RESET, mal_places);
        jouer_son(500, 150);
    }
    if (bien_places == 0 && mal_places == 0) {
        printf(RED BOLD "[X] Aucun chiffre correct" RESET);
        jouer_melodie_erreur(); // Melodie d'erreur pour feedback immédiat
    }
    
    // Calculer la longueur reelle du texte affiche
    int len = 7; // " Essai "
    len += (essai >= 10) ? 2 : 1; // nombre de chiffres de essai
    len += 2; // " : "
    len += strlen(code_essai); // longueur du code
    len += 4; // " -> "
    if (bien_places > 0) {
        len += 3; // "[+] "
        len += (bien_places >= 10) ? 2 : 1; // nombre de chiffres
        len += 15; // " bien place(s)"
    }
    if (mal_places > 0) {
        if (bien_places > 0) len += 3; // " | "
        len += 3; // "[O] "
        len += (mal_places >= 10) ? 2 : 1; // nombre de chiffres
        len += 13; // " mal place(s)"
    }
    if (bien_places == 0 && mal_places == 0) {
        len += 22; // "[X] Aucun chiffre correct"
    }
    for (int i = len; i < CONSOLE_WIDTH - 2; i++) printf(" ");
    printf(CYAN BOX_VERTICAL RESET "\n");
    
    printf(CYAN BOX_BOT_LEFT);
    for (int i = 0; i < CONSOLE_WIDTH - 2; i++) printf(BOX_HORIZONTAL);
    printf(BOX_BOT_RIGHT RESET "\n");
}

// Fonction pour afficher le meilleur score
void afficher_meilleur_score() {
    clear_screen();
    afficher_titre();
    
    FILE *fichier = fopen(FILENAME, "r");
    if (fichier == NULL) {
        afficher_boite("MEILLEUR SCORE", CYAN BOLD, CONSOLE_WIDTH);
        printf("\n" RED BOLD "  Aucun score enregistre pour le moment.\n\n" RESET);
        printf(CYAN BOLD "  Appuyez sur une touche pour continuer..." RESET);
        getchar();
        return;
    }
    
    Score meilleur_score;
    meilleur_score.essais = MAX_ATTEMPTS + 1;
    bool trouve = false;
    
    Score score_actuel;
    while (fscanf(fichier, "%s %d", score_actuel.nom, &score_actuel.essais) == 2) {
        trouve = true;
        if (score_actuel.essais < meilleur_score.essais) {
            meilleur_score = score_actuel;
        }
    }
    
    fclose(fichier);
    
    if (trouve) {
        afficher_boite("MEILLEUR SCORE", YELLOW BOLD, CONSOLE_WIDTH);
        printf("\n");
        printf(GREEN BOLD "  Nom: %s\n" RESET, meilleur_score.nom);
        printf(YELLOW BOLD "  Essais: %d\n\n" RESET, meilleur_score.essais);
        jouer_melodie_succes();
    } else {
        printf(RED BOLD "  Aucun score enregistre.\n\n" RESET);
    }
    
    printf(CYAN BOLD "  Appuyez sur une touche pour continuer..." RESET);
    getchar();
}

// Fonction pour sauvegarder un score
void sauvegarder_score(const char *nom, int essais) {
    FILE *fichier = fopen(FILENAME, "a");
    if (fichier == NULL) {
        printf(RED BOLD "Erreur: impossible de sauvegarder le score.\n" RESET);
        return;
    }
    
    fprintf(fichier, "%s %d\n", nom, essais);
    fclose(fichier);
    printf(GREEN BOLD "Score sauvegarde avec succes!\n" RESET);
}

// Fonction pour le joker J1: afficher un chiffre
void joker_j1(const char *code_secret, char *chiffre_affiche) {
    int position;
    clear_screen();
    afficher_boite("JOKER J1", MAGENTA BOLD, CONSOLE_WIDTH);
    printf("\n" CYAN BOLD "  Entrez la position du chiffre a afficher (1-5): " RESET);
    scanf("%d", &position);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    if (position >= 1 && position <= CODE_SIZE) {
        *chiffre_affiche = code_secret[position - 1];
        printf("\n" GREEN BOLD "  [*] Le chiffre a la position %d est: " RESET, position);
        printf(YELLOW BOLD "%c\n\n" RESET, *chiffre_affiche);
        jouer_melodie_joker();
    } else {
        printf("\n" RED BOLD "  Position invalide. Joker non utilise.\n" RESET);
        *chiffre_affiche = '\0';
        jouer_melodie_erreur();
    }
}

// Fonction pour le joker J2: exclure un chiffre
void joker_j2(const char *code_secret, int *chiffre_exclu) {
    char chiffre_str[2];
    clear_screen();
    afficher_boite("JOKER J2", MAGENTA BOLD, CONSOLE_WIDTH);
    printf("\n" CYAN BOLD "  Entrez le chiffre a verifier (0-9): " RESET);
    scanf("%s", chiffre_str);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    if (isdigit(chiffre_str[0])) {
        *chiffre_exclu = chiffre_str[0] - '0';
        // Verifier si le chiffre est dans le code
        bool present = false;
        for (int i = 0; i < CODE_SIZE; i++) {
            if (code_secret[i] == chiffre_str[0]) {
                present = true;
                break;
            }
        }
        
        if (present) {
            printf("\n" RED BOLD "  [X] Le chiffre %c EST dans le code.\n\n" RESET, chiffre_str[0]);
            jouer_son(400, 300);
        } else {
            printf("\n" GREEN BOLD "  [OK] Le chiffre %c N'EST PAS dans le code.\n\n" RESET, chiffre_str[0]);
            jouer_son(1000, 200);
        }
    } else {
        printf("\n" RED BOLD "  Chiffre invalide. Joker non utilise.\n" RESET);
        *chiffre_exclu = -1;
        jouer_melodie_erreur();
    }
}

// Fonction pour selectionner le mode de jeu
int choisir_mode() {
    int mode;
    clear_screen();
    afficher_titre();
    afficher_boite("CHOIX DU MODE", CYAN BOLD, CONSOLE_WIDTH);
    printf("\n");
    printf(GREEN "  1. " RESET);
    printf(WHITE "Mode Solo (l'ordinateur genere le code)\n" RESET);
    printf(BLUE "  2. " RESET);
    printf(WHITE "Mode Duel (deux joueurs)\n" RESET);
    printf("\n" YELLOW BOLD "  Votre choix: " RESET);
    scanf("%d", &mode);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    if (mode != 1 && mode != 2) {
        printf(RED BOLD "  Mode invalide. Utilisation du mode Solo par defaut.\n" RESET);
        mode = 1;
        jouer_melodie_erreur();
    } else {
        jouer_melodie_succes();
    }
    return mode;
}

// Fonction pour selectionner le niveau
int choisir_niveau() {
    int niveau;
    clear_screen();
    afficher_titre();
    afficher_boite("CHOIX DU NIVEAU", CYAN BOLD, CONSOLE_WIDTH);
    printf("\n");
    printf(GREEN "  1. " RESET);
    printf(WHITE "Debutant (tous les chiffres distincts)\n" RESET);
    printf(RED "  2. " RESET);
    printf(WHITE "Expert (chiffres peuvent se repeter)\n" RESET);
    printf("\n" YELLOW BOLD "  Votre choix: " RESET);
    scanf("%d", &niveau);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    if (niveau != 1 && niveau != 2) {
        printf(RED BOLD "  Niveau invalide. Utilisation du niveau 1 par defaut.\n" RESET);
        niveau = 1;
        jouer_melodie_erreur();
    } else {
        jouer_melodie_succes();
    }
    return niveau;
}

// Fonction pour obtenir le code secret du joueur 1 (mode duel)
void obtenir_code_secret(char *code_secret, int niveau) {
    do {
        printf("\n" CYAN BOLD "Joueur 1, entrez le code secret (5 chiffres): " RESET);
        scanf("%s", code_secret);
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        
        if (!valider_code(code_secret, niveau)) {
            if (niveau == 1) {
                printf(RED BOLD "[!] Code invalide! Le code doit contenir 5 chiffres tous distincts.\n" RESET);
            } else {
                printf(RED BOLD "[!] Code invalide! Le code doit contenir 5 chiffres.\n" RESET);
            }
        }
    } while (!valider_code(code_secret, niveau));
    
    clear_screen();
    afficher_titre();
    printf(GREEN BOLD "[OK] Code secret enregistre!\n" RESET);
    jouer_son(600, 200);
}

// Fonction pour obtenir le nom du joueur
void obtenir_nom_joueur(char *nom_joueur, int mode) {
    if (mode == 1) {
        // Mode solo
        printf("\n" CYAN BOLD "Entrez votre nom: " RESET);
    } else {
        // Mode duel
        printf("\n" CYAN BOLD "Joueur 2, entrez votre nom: " RESET);
    }
    fgets(nom_joueur, 50, stdin);
    // Supprimer le retour a la ligne
    nom_joueur[strcspn(nom_joueur, "\n")] = 0;
}

// Fonction pour afficher les informations de debut de jeu
void afficher_debut_jeu(int niveau, int mode) {
    clear_screen();
    afficher_titre();
    afficher_boite("DEBUT DU JEU", GREEN BOLD, CONSOLE_WIDTH);
    printf("\n");
    printf(WHITE "  Vous avez " RESET);
    printf(YELLOW BOLD "%d essais maximum.\n\n" RESET, MAX_ATTEMPTS);
    
    if (niveau == 1) {
        printf(CYAN BOLD "  Niveau: " RESET);
        printf(GREEN "Debutant (tous les chiffres distincts)\n" RESET);
    } else {
        printf(CYAN BOLD "  Niveau: " RESET);
        printf(RED "Expert (chiffres peuvent se repeter)\n" RESET);
    }
    
    if (mode == 1) {
        printf(BLUE BOLD "  Mode: Solo\n" RESET);
    } else {
        printf(BLUE BOLD "  Mode: Duel\n" RESET);
    }
    
    printf("\n" YELLOW BOLD "  Le joker sera disponible a partir du 10eme essai.\n\n" RESET);
    jouer_son(600, 200);
    Sleep(300);
    jouer_son(800, 200);
    Sleep(500);
}

// Fonction pour gerer l'utilisation du joker
bool utiliser_joker(const char *code_secret, char *chiffre_affiche, int *chiffre_exclu, int *essais_restants) {
    char choix_joker;
    printf("\n" YELLOW BOLD "[!] Voulez-vous utiliser le joker? (o/n): " RESET);
    scanf(" %c", &choix_joker);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    if (choix_joker == 'o' || choix_joker == 'O') {
        printf("\n" MAGENTA BOLD "Choisissez le type de joker:\n" RESET);
        printf(GREEN "  1. " RESET);
        printf(WHITE "J1: Afficher un chiffre de votre choix (perte de 3 essais)\n" RESET);
        printf(GREEN "  2. " RESET);
        printf(WHITE "J2: Verifier si un chiffre est dans le code (perte de 5 essais)\n" RESET);
        printf(YELLOW BOLD "Votre choix: " RESET);
        int type_joker;
        scanf("%d", &type_joker);
        while ((c = getchar()) != '\n' && c != EOF);
        
        if (type_joker == 1) {
            joker_j1(code_secret, chiffre_affiche);
            *essais_restants -= 3;
            printf(RED BOLD "[!] Vous perdez 3 essais. Essais restants: %d\n" RESET, *essais_restants);
            return true;
        } else if (type_joker == 2) {
            joker_j2(code_secret, chiffre_exclu);
            *essais_restants -= 5;
            printf(RED BOLD "[!] Vous perdez 5 essais. Essais restants: %d\n" RESET, *essais_restants);
            return true;
        } else {
            printf(RED BOLD "Choix invalide. Joker non utilise.\n" RESET);
        }
    }
    return false;
}

// Fonction pour gerer la fin de partie (victoire)
void fin_victoire(const char *code_secret, const char *nom_joueur, int essais) {
    clear_screen();
    printf("\n");
    afficher_boite("FELICITATIONS!", GREEN BOLD, CONSOLE_WIDTH);
    printf("\n");
    
    printf(WHITE "  Vous avez trouve le code en " RESET);
    printf(YELLOW BOLD "%d essai(s)!\n\n" RESET, essais);
    
    printf(CYAN "  Code secret: " RESET);
    printf(YELLOW BOLD "%s\n" RESET, code_secret);
    printf(CYAN "  Joueur: " RESET);
    printf(GREEN BOLD "%s\n\n" RESET, nom_joueur);
    
    // Son de victoire (MP3)
    jouer_fichier_mci("congratulations.mp3");
    Sleep(500); // Laisser le temps au son de démarrer
    
    // Sauvegarder le score
    sauvegarder_score(nom_joueur, essais);
    
    // Proposer d'afficher le meilleur score
    char voir_meilleur;
    printf("\n" CYAN BOLD "  Voulez-vous voir le meilleur score? (o/n): " RESET);
    scanf(" %c", &voir_meilleur);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    if (voir_meilleur == 'o' || voir_meilleur == 'O') {
        afficher_meilleur_score();
    }

    arreter_fichier_mci(); // Arrêter le son de victoire avant de quitter la fonction
}

// Fonction pour gerer la fin de partie (defaite)
void fin_defaite(const char *code_secret, const char *nom_joueur) {
    clear_screen();
    printf("\n");
    afficher_boite("GAME OVER", RED BOLD, CONSOLE_WIDTH);
    printf("\n");
    printf(WHITE "  Vous avez epuise tous vos essais!\n\n" RESET);
    printf(CYAN "  Le code secret etait: " RESET);
    printf(YELLOW BOLD "%s\n" RESET, code_secret);
    printf(CYAN "  Joueur: " RESET);
    printf(RED BOLD "%s\n\n" RESET, nom_joueur);
    
    // Son de défaite (MP3)
    jouer_fichier_mci("fail.mp3");
    Sleep(1500); // Laisser le temps au son de démarrer
    
    arreter_fichier_mci(); // Arrêter le son de défaite
}

// Fonction pour jouer en mode solo
void jouer_mode_solo() {
    char code_secret[CODE_SIZE + 1];
    char code_essai[CODE_SIZE + 1];
    char nom_joueur[50];
    int niveau;
    int essais = 0;
    bool joker_utilise = false;
    char chiffre_affiche = '\0';
    int chiffre_exclu = -1;
    int essais_restants = MAX_ATTEMPTS;
    
    // Selectionner le niveau
    niveau = choisir_niveau();
    
    // Generer le code secret aleatoirement
    generer_code_secret(code_secret, niveau);
    
    clear_screen();
    afficher_titre();
    printf(GREEN BOLD "[OK] Code secret genere par l'ordinateur!\n" RESET);
    jouer_son(600, 200);
    
    // Obtenir le nom du joueur
    obtenir_nom_joueur(nom_joueur, 1);
    
    // Afficher les informations de debut
    afficher_debut_jeu(niveau, 1);
    
    // Boucle principale du jeu
    while (essais < MAX_ATTEMPTS && essais_restants > 0) {
        essais++;
        printf("\n");
        printf(CYAN BOX_TOP_LEFT);
        for (int i = 0; i < CONSOLE_WIDTH - 2; i++) printf(BOX_HORIZONTAL);
        printf(BOX_TOP_RIGHT RESET "\n");
        
        printf(CYAN BOX_VERTICAL RESET);
        printf(CYAN BOLD " Essai %d/%d" RESET, essais, MAX_ATTEMPTS);
        printf(YELLOW BOLD " | Essais restants: %d" RESET, essais_restants);
        // Calculer la longueur reelle du texte affiche
        int len = 7; // " Essai "
        len += (essais >= 10) ? 2 : 1; // nombre de chiffres de essais
        len += 1; // "/"
        len += (MAX_ATTEMPTS >= 10) ? 2 : 1; // nombre de chiffres de MAX_ATTEMPTS
        len += 20; // " | Essais restants: "
        len += (essais_restants >= 10) ? 2 : 1; // nombre de chiffres de essais_restants
        for (int i = len; i < CONSOLE_WIDTH - 2; i++) printf(" ");
        printf(CYAN BOX_VERTICAL RESET "\n");
        
        printf(CYAN BOX_BOT_LEFT);
        for (int i = 0; i < CONSOLE_WIDTH - 2; i++) printf(BOX_HORIZONTAL);
        printf(BOX_BOT_RIGHT RESET "\n");
        
        // Afficher barre de progression
        printf("  ");
        afficher_barre_progression(essais, MAX_ATTEMPTS, 40);
        
        // Proposer le joker a partir du 10eme essai
        if (essais >= JOKER_THRESHOLD && !joker_utilise) {
            if (utiliser_joker(code_secret, &chiffre_affiche, &chiffre_exclu, &essais_restants)) {
                joker_utilise = true;
            }
        }
        
        // Afficher l'aide du joker J1 si utilise
        if (chiffre_affiche != '\0') {
            printf(YELLOW BOLD "[Rappel J1] Un chiffre a ete revele: %c\n" RESET, chiffre_affiche);
        }
        
        // Joueur entre son code
        do {
            printf(MAGENTA BOLD "Proposez votre code (5 chiffres): " RESET);
            scanf("%s", code_essai);
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            
            if (!valider_code(code_essai, niveau)) {
                if (niveau == 1) {
                    printf(RED BOLD "[!] Code invalide! Le code doit contenir 5 chiffres tous distincts.\n" RESET);
                } else {
                    printf(RED BOLD "[!] Code invalide! Le code doit contenir 5 chiffres.\n" RESET);
                }
                jouer_son(300, 200);
            }
        } while (!valider_code(code_essai, niveau));
        
        // Comparer les codes
        int bien_places, mal_places;
        comparer_codes(code_secret, code_essai, &bien_places, &mal_places);
        
        // Afficher le resultat
        afficher_resultat(essais, code_essai, bien_places, mal_places);
        
        // Verifier si le code est trouve
        if (bien_places == CODE_SIZE) {
            fin_victoire(code_secret, nom_joueur, essais);
            return;
        }
        
        essais_restants--;
    }
    
    // Si on arrive ici, le joueur a perdu
    fin_defaite(code_secret, nom_joueur);
}

// Fonction pour jouer en mode duel
void jouer_mode_duel() {
    char code_secret[CODE_SIZE + 1];
    char code_essai[CODE_SIZE + 1];
    char nom_joueur[50];
    int niveau;
    int essais = 0;
    bool joker_utilise = false;
    char chiffre_affiche = '\0';
    int chiffre_exclu = -1;
    int essais_restants = MAX_ATTEMPTS;
    
    // Selectionner le niveau
    niveau = choisir_niveau();
    
    // Obtenir le code secret
    obtenir_code_secret(code_secret, niveau);
    
    // Obtenir le nom du joueur 2
    obtenir_nom_joueur(nom_joueur, 2);
    
    // Afficher les informations de debut
    afficher_debut_jeu(niveau, 2);
    
    // Boucle principale du jeu
    while (essais < MAX_ATTEMPTS && essais_restants > 0) {
        essais++;
        printf("\n");
        printf(CYAN BOX_TOP_LEFT);
        for (int i = 0; i < CONSOLE_WIDTH - 2; i++) printf(BOX_HORIZONTAL);
        printf(BOX_TOP_RIGHT RESET "\n");
        
        printf(CYAN BOX_VERTICAL RESET);
        printf(CYAN BOLD " Essai %d/%d" RESET, essais, MAX_ATTEMPTS);
        printf(YELLOW BOLD " | Essais restants: %d" RESET, essais_restants);
        // Calculer la longueur reelle du texte affiche
        int len = 7; // " Essai "
        len += (essais >= 10) ? 2 : 1; // nombre de chiffres de essais
        len += 1; // "/"
        len += (MAX_ATTEMPTS >= 10) ? 2 : 1; // nombre de chiffres de MAX_ATTEMPTS
        len += 20; // " | Essais restants: "
        len += (essais_restants >= 10) ? 2 : 1; // nombre de chiffres de essais_restants
        for (int i = len; i < CONSOLE_WIDTH - 2; i++) printf(" ");
        printf(CYAN BOX_VERTICAL RESET "\n");
        
        printf(CYAN BOX_BOT_LEFT);
        for (int i = 0; i < CONSOLE_WIDTH - 2; i++) printf(BOX_HORIZONTAL);
        printf(BOX_BOT_RIGHT RESET "\n");
        
        // Afficher barre de progression
        printf("  ");
        afficher_barre_progression(essais, MAX_ATTEMPTS, 40);
        
        // Proposer le joker a partir du 10eme essai
        if (essais >= JOKER_THRESHOLD && !joker_utilise) {
            if (utiliser_joker(code_secret, &chiffre_affiche, &chiffre_exclu, &essais_restants)) {
                joker_utilise = true;
            }
        }
        
        // Afficher l'aide du joker J1 si utilise
        if (chiffre_affiche != '\0') {
            printf(YELLOW BOLD "[Rappel J1] Un chiffre a ete revele: %c\n" RESET, chiffre_affiche);
        }
        
        // Joueur 2 entre son code
        do {
            printf(MAGENTA BOLD "Joueur 2, proposez votre code (5 chiffres): " RESET);
            scanf("%s", code_essai);
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            
            if (!valider_code(code_essai, niveau)) {
                if (niveau == 1) {
                    printf(RED BOLD "[!] Code invalide! Le code doit contenir 5 chiffres tous distincts.\n" RESET);
                } else {
                    printf(RED BOLD "[!] Code invalide! Le code doit contenir 5 chiffres.\n" RESET);
                }
                jouer_son(300, 200);
            }
        } while (!valider_code(code_essai, niveau));
        
        // Comparer les codes
        int bien_places, mal_places;
        comparer_codes(code_secret, code_essai, &bien_places, &mal_places);
        
        // Afficher le resultat
        afficher_resultat(essais, code_essai, bien_places, mal_places);
        
        // Verifier si le code est trouve
        if (bien_places == CODE_SIZE) {
            fin_victoire(code_secret, nom_joueur, essais);
            return;
        }
        
        essais_restants--;
    }
    
    // Si on arrive ici, le joueur a perdu
    fin_defaite(code_secret, nom_joueur);
}

int main() {
    enable_colors();
    clear_screen();
    afficher_titre();
    
    printf(YELLOW BOLD "  Bienvenue dans le jeu MasterMind!\n\n" RESET);
    jouer_son(800, 200);
    Sleep(300);
    jouer_son(1000, 200);
    Sleep(500);
    
    int mode = choisir_mode();
    
    // Utiliser un switch pour gerer les differents modes
    switch (mode) {
        case 1:
            // Mode Solo
            jouer_mode_solo();
            break;
            
        case 2:
            // Mode Duel
            jouer_mode_duel();
            break;
            
        default:
            printf(RED BOLD "  Mode invalide!\n" RESET);
            break;
    }
    
    printf("\n" CYAN BOLD "  Appuyez sur une touche pour quitter..." RESET);
    getchar();

    arreter_fichier_mci(); // S'assurer que le son s'arrête avant de quitter
    
    return 0;
}