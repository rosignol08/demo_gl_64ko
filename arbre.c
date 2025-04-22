/*!\file test_arb.c
 * \brief GL4Dummies, système L pour générer un arbre
 * \author Modifié par vous
 * \date avril 20, 2025
 */
#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dm.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4dh.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

//#define MAX_STRING_LENGTH 3081
#define PI 3.14159265

static void init(void);
static void draw(void);
static void generateLSystem(void);

/* Variables pour le rendu OpenGL */
static GLuint _pId = 0;
static GLuint _lineVAO = 0;
static GLuint _lineVBO[2] = {0, 0}; /* position et couleur */

/* Variables pour le L-système */
static char* _lsystem = NULL;
static int _iterations = 6;
static float _angle = 25.0f;
static float _baseLength = 0.04f;
static float _branchRatio = 0.75f;
static int _season = 0; /* 0:été, 1:automne, 2:hiver */

/* Structure pour la pile d'états de la tortue */
typedef struct
{
    GLfloat x, y, z;
    GLfloat direction;
    GLfloat directionZ;
    GLfloat length;
    GLfloat thickness;
} TurtleState;

/* Pile d'états pour le système L */
static TurtleState _stateStack[100];
static int _stackTop = -1;

void arbre(int state)
{
    switch (state)
    {
    case GL4DH_INIT:
        init();
        return;
    case GL4DH_FREE:
        if (_lineVAO)
            glDeleteVertexArrays(1, &_lineVAO);
        if (_lineVBO[0])
            glDeleteBuffers(2, _lineVBO);
        if (_lsystem)
            free(_lsystem);
        return;
    case GL4DH_UPDATE_WITH_AUDIO:
        return;
    default: /* GL4DH_DRAW */
        draw();
        return;
    }
}

void init(void)
{
    /* Initialiser le générateur de nombres aléatoires */
    srand(time(NULL));
    _lsystem = (char *)malloc(128 * sizeof(char));
    if (!_lsystem) {
        //fprintf(stderr, "Erreur d'allocation mémoire\n");
        exit(EXIT_FAILURE);
    }
    strcpy(_lsystem, "X"); // Axiome de départ
    /* Générer le système L */
    generateLSystem();

    /* Créer le shader pour dessiner les lignes */
    _pId = gl4duCreateProgram("<vs>shaders/arbre.vs", "<fs>shaders/arbre.fs", NULL);

    /* Créer le VAO et les VBOs pour dessiner les lignes */
    glGenVertexArrays(1, &_lineVAO);
    glGenBuffers(2, _lineVBO);

    /* Initialiser les matrices GL4D */
    gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");
    gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
    gl4duBindMatrix("projectionMatrix");
    gl4duLoadIdentityf();
    gl4duFrustumf(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
}

// Fonction pour limiter un angle dans une plage
float clampAngle(float angle, float min, float max) {
    if (angle < min) return min;
    if (angle > max) return max;
    return angle;
}

void generateLSystem(void){
    //printf("Début de generateLSystem, _lsystem = \"%s\", longueur = %zu\n", _lsystem, strlen(_lsystem));
    //char temp[MAX_STRING_LENGTH];
    char* temp = NULL;
    size_t tempSize = 0;
    
    char newChar[3];


    //la mémoire initiale pour temp
    tempSize = strlen(_lsystem) * 30; // Un peu plus que 21 pour avoir de la marge
    if (tempSize < 128) tempSize = 128;

    temp = (char *)malloc(tempSize * sizeof(char));

    if (!temp) {
        fprintf(stderr, "Erreur d'allocation mémoire\n");
        exit(EXIT_FAILURE);
    }
    /*les règles de production pour chaque itération */
    for (int i = 0; i < _iterations; i++){
        //printf("  Itération %d, longueur de _lsystem = %zu\n", i, strlen(_lsystem));

        temp[0] = '\0';

        for (int j = 0; j < strlen(_lsystem); j++){
            // Vérifier si nous approchons de la limite de taille
            if (strlen(temp) >= tempSize - 30) {
                tempSize *= 2;
                temp = (char *)realloc(temp, tempSize * sizeof(char));
                if (!temp) {
                    fprintf(stderr, "Erreur de réallocation mémoire\n");
                    exit(EXIT_FAILURE);
                }
                printf("    Réallocation de temp à %zu octets\n", tempSize);
            }

            if (_lsystem[j] == 'X')
            {
                /* Règles de production pour X, avec variations aléatoires */
                int choice = rand() % 4;
                switch (choice)
               {
                case 0:
                    strcat(temp, "F-[[X]^+X]+F^[[X]&+X]-X");
                    break;
                case 1:
                    strcat(temp, "F-[[X]&+X]+F[-FX^]+X");
                    break;
                case 2:
                    strcat(temp, "F[+X^][-X&]FX");
                    break;
                case 3:
                    strcat(temp, "F[+X^]F[-X&]+X");
                    break;
                }
            }
            else if (_lsystem[j] == 'F')
            {
                /* Règle de production pour F */
                strcat(temp, "FF");
            }
            else
            {
                /* Garder les autres caractères tels quels */
                newChar[0] = _lsystem[j];
                newChar[1] = '\0';
                strcat(temp, newChar);
            }
        }
        // Après chaque itération, réallouer _lsystem à la taille exacte
        char* newSystem = (char *)malloc((strlen(temp) + 1) * sizeof(char));
        if (!newSystem) {
            fprintf(stderr, "Erreur de réallocation mémoire pour _lsystem\n");
            free(temp);
            exit(EXIT_FAILURE);
        }
        
        strcpy(newSystem, temp);
        free(_lsystem);
        _lsystem = newSystem;
        
        //printf("  Fin itération %d, nouvelle longueur = %zu\n", i, strlen(_lsystem));
    }
    // Libérer la mémoire temporaire
    free(temp);
    //printf("Fin de generateLSystem, longueur finale = %zu\n", strlen(_lsystem));
}

void pushState3d(float x, float y, float z, float direction, float directionZ, float length, float thickness)
{
    if (_stackTop >= 99)
        return;

    _stackTop++;
    _stateStack[_stackTop].x = x;
    _stateStack[_stackTop].y = y;
    _stateStack[_stackTop].z = z;
    _stateStack[_stackTop].direction = direction;
    _stateStack[_stackTop].directionZ = directionZ;
    _stateStack[_stackTop].length = length;
    _stateStack[_stackTop].thickness = thickness;
}

TurtleState popState(void)
{
    TurtleState state = _stateStack[_stackTop];
    if (_stackTop > -1)
        _stackTop--;
    return state;
}

void drawLine3d(float x1, float y1, float z1, float x2, float y2, float z2, float thickness, float r, float g, float b)
{
    GLfloat vertices[6] = {
        x1, y1, z1,
        x2, y2, z2};

    GLfloat colors[8] = {
        r, g, b, 1.0f,
        r, g, b, 1.0f};

    glBindVertexArray(_lineVAO);

    /* Position */
    glBindBuffer(GL_ARRAY_BUFFER, _lineVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    /* Couleur */
    glBindBuffer(GL_ARRAY_BUFFER, _lineVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    /* l'épaisseur de la ligne */
    glLineWidth(thickness);

    /* Dessiner la ligne */
    glDrawArrays(GL_LINES, 0, 2);

    /* Nettoyer */
    glBindVertexArray(0);
}

void drawLeaf3d(float x, float y, float z, float direction, float directionZ, float season)
{
    float length = 0.02f;
    float r = 0.0f, g = 0.8f, b = 0.0f;

    /* Ajuster la couleur selon la saison */
    if (season == 0)
    { /* Été */
        r = 0.0f;
        g = 0.8f + ((float)rand() / RAND_MAX) * 0.2f;
        b = 0.0f;
    }
    float angleRadXY = direction * PI / 180.0f;
    float angleRadZ = directionZ * PI / 180.0f;
    
    float x2 = x + length * cos(angleRadXY) * cos(angleRadZ);
    float y2 = y + length * sin(angleRadXY) * cos(angleRadZ);
    float z2 = z + length * sin(angleRadZ);
    
    drawLine3d(x, y, z, x2, y2, z, 2.0f, r, g, b);
}

void draw(void)
{
    static double t0 = 0;
    double t = gl4dhGetTicks() / 1000.0, dt = t - t0;
    static float rotation = 0.0f;
    //pour qu'il grandisse

    /* La scène dure 5000ms = 5s*/
    double sceneTime = gl4dhGetTicks() % 5000 / 5000.0;
    static int lastIteration = 0;
    int targetIteration = (int)(sceneTime * 6.0); //max d'itérations
    
    /* Ne régénérer que si une nouvelle itération est atteinte */
    if(targetIteration > lastIteration && targetIteration <= 6) {
        lastIteration = targetIteration;
        
        /* Réinitialiser le L-system pour partir de l'axiome X */
        strcpy(_lsystem, "X");
        /* Définir le nombre d'itérations */
        _iterations = targetIteration;
        /* Régénérer le système L */
        generateLSystem();
    }
    /* Effacer l'écran */
    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Activer le shader */
    glUseProgram(_pId);

    /* Configurer les matrices */
    gl4duBindMatrix("modelViewMatrix");
    gl4duLoadIdentityf();
    gl4duTranslatef(0.0f, -0.8f, -2.0f);
    gl4duRotatef(rotation, 0, 1, 0);//TODO

    gl4duBindMatrix("projectionMatrix");
    gl4duSendMatrices();

    /* Interpréter le système L */
    float x = 0.0f, y = -0.20f, z = 0.0f; /* Position de départ */
    float direction = 90.0f; /* Direction en degrés (vers le haut) */
    float directionZ = 0.0f; /* Direction Z (vers le haut) */
    float length = _baseLength;
    float thickness = 10.0f;

    _stackTop = -1; /* Réinitialiser la pile */

    for (int i = 0; i < strlen(_lsystem); i++)
    {
        char current = _lsystem[i];

        switch (current)
        {
        case 'F':
        {
                /* Calculer la nouvelle position en 3D */
    float angleRadXY = direction * PI / 180.0f;
    float angleRadZ = directionZ * PI / 180.0f;
    
    /* Utiliser les coordonnées sphériques pour le calcul 3D */
    float newX = x + length * cos(angleRadXY) * cos(angleRadZ);
    float newY = y + length * sin(angleRadXY) * cos(angleRadZ);
    float newZ = z + length * sin(angleRadZ);
    
    /* Dessiner la branche */
    float r = 0.6f, g = 0.4f, b = 0.2f; /* Couleur marron pour le tronc/branches */
    drawLine3d(x, y, z, newX, newY, newZ, thickness, r, g, b);
    
    /* Mettre à jour la position */
    x = newX;
    y = newY;
    z = newZ;
    
    /* Réduire légèrement l'épaisseur pour un effet de branche qui s'affine */
    thickness *= 0.99f;
    break;
        }
        case '+':
            /* Tourner à gauche */
            direction += _angle;
            break;
        case '-':
            /* Tourner à droite */
            direction -= _angle;
            break;
        case '^':  // Rotation vers le haut (dans l'espace 3D)
            directionZ += _angle;
            directionZ = clampAngle(directionZ, -60.0f, 60.0f); //limitation de la rotation
            break;
        case '&':  // Rotation vers le bas (dans l'espace 3D)
            directionZ -= _angle;
            directionZ = clampAngle(directionZ, -60.0f, 60.0f); //limitation de la rotation
            break;
        case '[':
            /* Sauvegarder l'état actuel */
            //pushState(x, y, direction, length, thickness);
            pushState3d(x, y, z, direction,directionZ, length, thickness);
            /* Réduire la longueur pour les branches suivantes */
            length *= _branchRatio;
            thickness *= 0.8f;
            break;
        case ']':
            /* Restaurer l'état précédent */
            if (_stackTop >= 0)
            {
                TurtleState state = popState();
                x = state.x;
                y = state.y;
                z = state.z;
                direction = state.direction;
                length = state.length;
                thickness = state.thickness;
            }
            break;
        case 'X':
            /* Si X est suivi de caractères autres que '[', c'est une feuille */
            if (i + 1 < strlen(_lsystem) && _lsystem[i + 1] != '[')
            {
                drawLeaf3d(x, y, z, direction, directionZ, _season);
                //drawLeaf(x, y, direction, _season);
            }
            break;
        }
    }

    /* Rotation lente de l'arbre */
    rotation += 10.0f * dt;

    t0 = t;
    /* Désactiver le shader */
    glUseProgram(0);

    //desactiver le depth test ici ça sert à rien
    glDisable(GL_DEPTH_TEST);

    /* Mettre à jour la rotation */
    rotation += 15.0f * dt;
    if (rotation > 360.0f)
        rotation -= 360.0f;
}
