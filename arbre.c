/*!\file test_arb.c
 * \brief GL4Dummies, système L pour générer un arbre
 * \author Modifié par romaric de https://github.com/shortstheory/l-systems-opengl/tree/master
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
typedef struct{
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
        if (_pId)
            glDeleteProgram(_pId);
        if (_lineVBO[0])
            glDeleteBuffers(2, _lineVBO);
        if (_lineVAO)
            glDeleteVertexArrays(1, &_lineVAO);
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

    //le shader pour dessiner les lignes
    _pId = gl4duCreateProgram("<vs>shaders/arbre.vs", "<fs>shaders/arbre.fs", NULL);

    //le VAO et les VBOs pour dessiner les lignes
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

void drawLeaf3d(float x, float y, float z, float direction, float directionZ, int season)
{
    float leafSize = 0.03f;
    float r = 0.0f, g = 0.8f, b = 0.0f;
    float thickness = 2.5f;

    /* Ajuster la couleur selon la saison */
    if (season == 0) { /* Été */
        r = 1.0f ;// ((float)rand() / RAND_MAX) * 0.2f; // Légère variation
        g = 0.718f;// + ((float)rand() / RAND_MAX) * 0.3f; // Variation de vert
        b = 0.773f;
    }

    /* Angle pour l'orientation de la feuille */
    float angleRadXY = direction * PI / 180.0f;
    float angleRadZ = directionZ * PI / 180.0f;
    
    /* Créer un losange/diamant pour représenter la feuille */
    float xTip = x + leafSize * 1.5f * cos(angleRadXY) * cos(angleRadZ);
    float yTip = y + leafSize * 1.5f * sin(angleRadXY) * cos(angleRadZ);
    float zTip = z + leafSize * 1.5f * sin(angleRadZ);
    
    /* Points latéraux pour donner une forme de feuille */
    float xLeft = x + leafSize * 0.7f * cos(angleRadXY + PI/2) * cos(angleRadZ - PI/6);
    float yLeft = y + leafSize * 0.7f * sin(angleRadXY + PI/2) * cos(angleRadZ - PI/6);
    float zLeft = z + leafSize * 0.7f * sin(angleRadZ - PI/6);
    
    float xRight = x + leafSize * 0.7f * cos(angleRadXY - PI/2) * cos(angleRadZ + PI/6);
    float yRight = y + leafSize * 0.7f * sin(angleRadXY - PI/2) * cos(angleRadZ + PI/6);
    float zRight = z + leafSize * 0.7f * sin(angleRadZ + PI/6);
    
    /* Dessiner le contour de la feuille */
    drawLine3d(x, y, z, xTip, yTip, zTip, thickness, r, g, b);
    drawLine3d(x, y, z, xLeft, yLeft, zLeft, thickness, r, g, b);
    drawLine3d(x, y, z, xRight, yRight, zRight, thickness, r, g, b);
    drawLine3d(xTip, yTip, zTip, xLeft, yLeft, zLeft, thickness, r, g, b);
    drawLine3d(xTip, yTip, zTip, xRight, yRight, zRight, thickness, r, g, b);
}

void draw(void)
{
    static double t0 = 0;
    double t = gl4dhGetTicks() / 1000.0, dt = t - t0;
    static float rotation = 0.0f;
    //pour qu'il grandisse

    /* La scène dure 5000ms = 5s*/
    double sceneTime = gl4dhGetTicks() % 12000 / 12000.0;
    static int lastIteration = 0;
    int targetIteration = (int)(sceneTime * 7.0); //max d'itérations
    
    /* Ne régénérer que si une nouvelle itération est atteinte */
    if(targetIteration > lastIteration && targetIteration <= 7) {
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

    /* Configurer la lumière directionnelle */
    //float lightAngle = t * 0.2f; // Rotation lente de la lumière
    //GLfloat lightDir[3] = {
    //    cosf(lightAngle),
    //    0.8f,               // Toujours un peu vers le haut
    //    sinf(lightAngle)
    //};
    //// Normaliser le vecteur de direction
    //float lightLength = sqrtf(lightDir[0] * lightDir[0] + lightDir[1] * lightDir[1] + lightDir[2] * lightDir[2]);
    //lightDir[0] /= lightLength;
    //lightDir[1] /= lightLength;
    //lightDir[2] /= lightLength;

    // Envoyer la direction de la lumière au shader
    //glUniform3fv(glGetUniformLocation(_pId, "lightDir"), 1, lightDir);
    /* Faire osciller la direction de la lumière en cercle */
    float lightAngle = t * 0.5f; // Vitesse de rotation
    GLfloat lightDir[3] = {
        cosf(lightAngle),         // oscillation sur l'axe X
        0.5f + 0.3f * sinf(t),    // légère oscillation verticale
        sinf(lightAngle)          // oscillation sur l'axe Z
    };
    // Normaliser le vecteur de direction
    float lightLength = sqrtf(lightDir[0] * lightDir[0] + lightDir[1] * lightDir[1] + lightDir[2] * lightDir[2]);
    lightDir[0] /= lightLength;
    lightDir[1] /= lightLength;
    lightDir[2] /= lightLength;

    // Envoyer la direction de la lumière au shader
    glUniform3fv(glGetUniformLocation(_pId, "lightDir"), 1, lightDir);


    /* Configurer les matrices */
    gl4duBindMatrix("modelViewMatrix");
    gl4duLoadIdentityf();
    gl4duScalef(30.5f, 30.5f, 30.5f);
    gl4duTranslatef(0.0f, -0.8f, -2.0f);
    gl4duRotatef(rotation, 0, 1, 0);//TODO
    //gl4duRotatef(5.5f, 0, 0, 1);//TODO
    gl4duBindMatrix("projectionMatrix");
    gl4duSendMatrices();


/* Activer le depth test pour le rendu 3D */
glEnable(GL_DEPTH_TEST);

/* Envoyer le temps au shader */
glUniform1f(glGetUniformLocation(_pId, "time"), t * 0.1f);
glUniform1i(glGetUniformLocation(_pId, "season"), _season);

    /* Interpréter le système L */
    float x = 0.0f, y = -0.20f, z = 0.0f; /* Position de départ */
    float direction = 90.0f; /* Direction en degrés (vers le haut) */
    float directionZ = 0.0f; /* Direction Z (vers le haut) */
    float length = _baseLength * 0.4f;//TODO reduire
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
    //on dessine la branche
    glUniform1i(glGetUniformLocation(_pId, "isLeaf"), 0);
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
            if (i + 1 < strlen(_lsystem) && _lsystem[i + 1] != '['){
                //on dit que c'est une feuille
                glUniform1i(glGetUniformLocation(_pId, "isLeaf"), 1);
                drawLeaf3d(x, y, z, direction, directionZ, _season);
                //on dit que c'est pas une feuille
                glUniform1i(glGetUniformLocation(_pId, "isLeaf"), 0);
            }
            break;
        }
    }

    /* Rotation lente de l'arbre */
    rotation += 10.0f * dt;

    t0 = t;
    //envoie t au vertex shader
    glUniform1f(glGetUniformLocation(_pId, "temps"), t * 0.1f);
    /* Dessiner plusieurs arbres */
    int numTrees = 3; // Nombre d'arbres à dessiner
    float positionX[] = {0.0f, 0.6f, -0.5f}; // Positions X
    float positionZ[] = {-2.0f, -2.5f, -2.3f}; // Positions Z
    float rotationOffset[] = {0.0f, 30.0f, 60.0f}; // Rotations différentes
    float scaleFactors[] = {1.0f, 0.9f, 1.1f}; // Facteurs d'échelle

    for (int tree = 0; tree < numTrees; tree++) {
        gl4duBindMatrix("modelViewMatrix");
        gl4duLoadIdentityf();
        gl4duScalef(30.5f * scaleFactors[tree], 30.5f * scaleFactors[tree], 30.5f * scaleFactors[tree]);
        gl4duTranslatef(positionX[tree], -0.8f, positionZ[tree]);
        gl4duRotatef(rotation + rotationOffset[tree], 0, 1, 0);
        gl4duBindMatrix("projectionMatrix");
        gl4duSendMatrices();

        /* Envoyer le temps au shader avec un léger décalage */
        glUniform1f(glGetUniformLocation(_pId, "time"), t * 0.1f + tree * 0.2f);
        glUniform1i(glGetUniformLocation(_pId, "season"), _season);

        /* Paramètres pour cet arbre */
        float x2 = 0.0f, y2 = -0.20f, z2 = 0.0f;
        float direction2 = 90.0f;
        float directionZ2 = 0.0f;
        float length2 = _baseLength * (0.4f + tree * 0.05f);
        float thickness2 = 10.0f;

        _stackTop = -1;  // Réinitialiser la pile

        for (int i = 0; i < strlen(_lsystem); i++) {
            char current = _lsystem[i];

            switch (current) {
            case 'F': {
                float angleRadXY = direction2 * PI / 180.0f;
                float angleRadZ = directionZ2 * PI / 180.0f;
                
                float newX = x2 + length2 * cos(angleRadXY) * cos(angleRadZ);
                float newY = y2 + length2 * sin(angleRadXY) * cos(angleRadZ);
                float newZ = z2 + length2 * sin(angleRadZ);
                
                float r = 0.6f - tree * 0.05f;
                float g = 0.4f - tree * 0.05f;
                float b = 0.2f;
                glUniform1i(glGetUniformLocation(_pId, "isLeaf"), 0);
                drawLine3d(x2, y2, z2, newX, newY, newZ, thickness2, r, g, b);
                
                x2 = newX;
                y2 = newY;
                z2 = newZ;
                
                thickness2 *= 0.99f;
                break;
            }
            case '+':
                direction2 += _angle * (1.0f + tree * 0.02f);
                break;
            case '-':
                direction2 -= _angle * (1.0f + tree * 0.02f);
                break;
            case '^':
                directionZ2 += _angle;
                directionZ2 = clampAngle(directionZ2, -60.0f, 60.0f);
                break;
            case '&':
                directionZ2 -= _angle;
                directionZ2 = clampAngle(directionZ2, -60.0f, 60.0f);
                break;
            case '[':
                pushState3d(x2, y2, z2, direction2, directionZ2, length2, thickness2);
                length2 *= _branchRatio * (1.0f + tree * 0.01f);
                thickness2 *= 0.8f;
                break;
            case ']':
                if (_stackTop >= 0) {
                    TurtleState state = popState();
                    x2 = state.x;
                    y2 = state.y;
                    z2 = state.z;
                    direction2 = state.direction;
                    directionZ2 = state.directionZ;
                    length2 = state.length;
                    thickness2 = state.thickness;
                }
                break;
            case 'X':
                if (i + 1 < strlen(_lsystem) && _lsystem[i + 1] != '[') {
                    glUniform1i(glGetUniformLocation(_pId, "isLeaf"), 1);
                    drawLeaf3d(x2, y2, z2, direction2, directionZ2, _season);
                    glUniform1i(glGetUniformLocation(_pId, "isLeaf"), 0);
                }
                break;
            }
        }
    }
    /* Désactiver le shader */
    glUseProgram(0);

    //desactiver le depth test ici ça sert à rien
    glDisable(GL_DEPTH_TEST);

    /* Mettre à jour la rotation */
    rotation += 15.0f * dt;
    if (rotation > 360.0f)
        rotation -= 360.0f;
}
