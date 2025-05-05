#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dm.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4dh.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// Définir les constantes et les structures nécessaires
#define PI 3.14159265

typedef struct {
    GLfloat x, y, z;
    GLfloat direction;
    GLfloat directionZ;
    GLfloat length;
    GLfloat thickness;
} TurtleState;

// Variables globales
static GLuint _pIdTerrain = 0;
static GLuint _pIdArbre = 0;
static GLuint _gridId = 0;
static GLuint _lineVAO = 0;
static GLuint _lineVBO[2] = {0, 0};
static char* _lsystem = NULL;
static int _iterations = 6;
static float _angle = 25.0f;
static float _baseLength = 0.04f;
static float _branchRatio = 0.75f;
static int _season = 0;
static TurtleState _stateStack[100];
static int _stackTop = -1;

// Prototypes des fonctions
static void init(void);
static void draw(void);
static void generateLSystem(void);
static void pushState3d(float x, float y, float z, float direction, float directionZ, float length, float thickness);
static TurtleState popState(void);
static void drawLine3d(float x1, float y1, float z1, float x2, float y2, float z2, float thickness, float r, float g, float b);
static void drawLeaf3d(float x, float y, float z, float direction, float directionZ, int season);
static float getTerrainHeight(float x, float z);

void montagne_arbre(int state) {
    switch (state) {
    case GL4DH_INIT:
        init();
        return;
    case GL4DH_FREE:
        if (_pIdTerrain) glDeleteProgram(_pIdTerrain);
        if (_pIdArbre) glDeleteProgram(_pIdArbre);
        if (_gridId) glDeleteVertexArrays(1, &_gridId);
        if (_lineVAO) glDeleteVertexArrays(1, &_lineVAO);
        if (_lineVBO[0]) glDeleteBuffers(2, _lineVBO);
        if (_lsystem) free(_lsystem);
        return;
    case GL4DH_UPDATE_WITH_AUDIO:
        return;
    default:
        draw();
        return;
    }
}

void init(void) {
    // Initialiser le générateur de nombres aléatoires
    srand(time(NULL));
    _lsystem = (char *)malloc(128 * sizeof(char));
    if (!_lsystem) {
        exit(EXIT_FAILURE);
    }
    strcpy(_lsystem, "X"); // Axiome de départ
    generateLSystem();

    // Charger les shaders pour le terrain et l'arbre
    _pIdTerrain = gl4duCreateProgram("<vs>shaders/terrain.vs", "<fs>shaders/terrain.fs", NULL);
    _pIdArbre = gl4duCreateProgram("<vs>shaders/arbre.vs", "<fs>shaders/arbre.fs", NULL);

    // Générer le terrain
    GLfloat *heightmap = gl4dmTriangleEdge(33, 33, 0.6f);
    _gridId = gl4dgGenGrid2dFromHeightMapf(33, 33, heightmap);
    free(heightmap);

    // Initialiser les VAO et VBO pour l'arbre
    glGenVertexArrays(1, &_lineVAO);
    glGenBuffers(2, _lineVBO);

    // Initialiser les matrices GL4D
    gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");
    gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
    gl4duBindMatrix("projectionMatrix");
    gl4duLoadIdentityf();
    gl4duFrustumf(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);

    // Configurer les paramètres OpenGL
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

float getTerrainHeight(float x, float z) {
    float scale = 0.1f;
    float y = 0.0f;
    y += 0.5f * sinf(x * 1.1f + z * 0.9f) * cosf(x * 0.7f - z * 1.3f);
    y += 0.25f * sinf(x * 2.3f + z * 1.7f) * cosf(x * 1.9f - z * 2.1f);
    y += 0.125f * sinf(x * 4.5f + z * 3.1f) * cosf(x * 3.7f - z * 4.3f);
    y = (y + 1.0f) * 0.5f * 4.0f;
    return y;
}

void generateLSystem(void) {
    char* temp = NULL;
    size_t tempSize = 0;
    char newChar[3];
    tempSize = strlen(_lsystem) * 30;
    if (tempSize < 128) tempSize = 128;
    temp = (char *)malloc(tempSize * sizeof(char));
    if (!temp) {
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < _iterations; i++) {
        temp[0] = '\0';
        for (int j = 0; j < strlen(_lsystem); j++) {
            if (strlen(temp) >= tempSize - 30) {
                tempSize *= 2;
                temp = (char *)realloc(temp, tempSize * sizeof(char));
                if (!temp) {
                    exit(EXIT_FAILURE);
                }
            }
            if (_lsystem[j] == 'X') {
                int choice = rand() % 4;
                switch (choice) {
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
            } else if (_lsystem[j] == 'F') {
                strcat(temp, "FF");
            } else {
                newChar[0] = _lsystem[j];
                newChar[1] = '\0';
                strcat(temp, newChar);
            }
        }
        char* newSystem = (char *)malloc((strlen(temp) + 1) * sizeof(char));
        if (!newSystem) {
            free(temp);
            exit(EXIT_FAILURE);
        }
        strcpy(newSystem, temp);
        free(_lsystem);
        _lsystem = newSystem;
    }
    free(temp);
}

void pushState3d(float x, float y, float z, float direction, float directionZ, float length, float thickness) {
    if (_stackTop >= 99) return;
    _stackTop++;
    _stateStack[_stackTop].x = x;
    _stateStack[_stackTop].y = y;
    _stateStack[_stackTop].z = z;
    _stateStack[_stackTop].direction = direction;
    _stateStack[_stackTop].directionZ = directionZ;
    _stateStack[_stackTop].length = length;
    _stateStack[_stackTop].thickness = thickness;
}

TurtleState popState(void) {
    TurtleState state = _stateStack[_stackTop];
    if (_stackTop > -1) _stackTop--;
    return state;
}

void drawLine3d(float x1, float y1, float z1, float x2, float y2, float z2, float thickness, float r, float g, float b) {
    GLfloat vertices[6] = {x1, y1, z1, x2, y2, z2};
    GLfloat colors[8] = {r, g, b, 1.0f, r, g, b, 1.0f};
    glBindVertexArray(_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _lineVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, _lineVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    glLineWidth(thickness);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

void drawLeaf3d(float x, float y, float z, float direction, float directionZ, int season) {
    float leafSize = 0.03f;
    float r = 0.0f, g = 0.8f, b = 0.0f;
    float thickness = 2.5f;
    if (season == 0) {
        r = 1.0f;
        g = 0.718f;
        b = 0.773f;
    }
    float angleRadXY = direction * PI / 180.0f;
    float angleRadZ = directionZ * PI / 180.0f;
    float xTip = x + leafSize * 1.5f * cos(angleRadXY) * cos(angleRadZ);
    float yTip = y + leafSize * 1.5f * sin(angleRadXY) * cos(angleRadZ);
    float zTip = z + leafSize * 1.5f * sin(angleRadZ);
    float xLeft = x + leafSize * 0.7f * cos(angleRadXY + PI/2) * cos(angleRadZ - PI/6);
    float yLeft = y + leafSize * 0.7f * sin(angleRadXY + PI/2) * cos(angleRadZ - PI/6);
    float zLeft = z + leafSize * 0.7f * sin(angleRadZ - PI/6);
    float xRight = x + leafSize * 0.7f * cos(angleRadXY - PI/2) * cos(angleRadZ + PI/6);
    float yRight = y + leafSize * 0.7f * sin(angleRadXY - PI/2) * cos(angleRadZ + PI/6);
    float zRight = z + leafSize * 0.7f * sin(angleRadZ + PI/6);
    drawLine3d(x, y, z, xTip, yTip, zTip, thickness, r, g, b);
    drawLine3d(x, y, z, xLeft, yLeft, zLeft, thickness, r, g, b);
    drawLine3d(x, y, z, xRight, yRight, zRight, thickness, r, g, b);
    drawLine3d(xTip, yTip, zTip, xLeft, yLeft, zLeft, thickness, r, g, b);
    drawLine3d(xTip, yTip, zTip, xRight, yRight, zRight, thickness, r, g, b);
}

void draw(void) {
    static double t0 = 0;
    double t = gl4dhGetTicks() / 1000.0, dt = t - t0;
    static float rotation = 0.0f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Dessiner le terrain
    glUseProgram(_pIdTerrain);
    gl4duBindMatrix("modelViewMatrix");
    gl4duLoadIdentityf();
    gl4duLookAtf(0.0f, 3.5f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    gl4duBindMatrix("projectionMatrix");
    gl4duLoadIdentityf();
    gl4duPerspectivef(55.0f, 1.0f, 0.1f, 100.0f);
    gl4duBindMatrix("modelMatrix");
    gl4duLoadIdentityf();
    gl4duScalef(8.0f, 4.0f, 8.0f);
    gl4duSendMatrices();
    gl4dgDraw(_gridId);

    // Dessiner l'arbre
    glUseProgram(_pIdArbre);
    gl4duBindMatrix("modelViewMatrix");
    gl4duLoadIdentityf();
    gl4duLookAtf(0.0f, 3.5f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    gl4duBindMatrix("projectionMatrix");
    gl4duLoadIdentityf();
    gl4duPerspectivef(55.0f, 1.0f, 0.1f, 100.0f);
    gl4duBindMatrix("modelMatrix");
    gl4duLoadIdentityf();
    gl4duTranslatef(0.0f, getTerrainHeight(0.0f, 0.0f), 0.0f);
    gl4duScalef(1.0f, 1.0f, 1.0f);
    gl4duSendMatrices();

    float x = 0.0f, y = -0.20f, z = 0.0f;
    float direction = 90.0f;
    float directionZ = 0.0f;
    float length = _baseLength * 0.4f;
    float thickness = 10.0f;
    _stackTop = -1;

    for (int i = 0; i < strlen(_lsystem); i++) {
        char current = _lsystem[i];
        switch (current) {
        case 'F': {
            float angleRadXY = direction * PI / 180.0f;
            float angleRadZ = directionZ * PI / 180.0f;
            float newX = x + length * cos(angleRadXY) * cos(angleRadZ);
            float newY = y + length * sin(angleRadXY) * cos(angleRadZ);
            float newZ = z + length * sin(angleRadZ);
            float r = 0.6f, g = 0.4f, b = 0.2f;
            drawLine3d(x, y, z, newX, newY, newZ, thickness, r, g, b);
            x = newX;
            y = newY;
            z = newZ;
            thickness *= 0.99f;
            break;
        }
        case '+':
            direction += _angle;
            break;
        case '-':
            direction -= _angle;
            break;
        case '^':
            directionZ += _angle;
            directionZ = clampAngle(directionZ, -60.0f, 60.0f);
            break;
        case '&':
            directionZ -= _angle;
            directionZ = clampAngle(directionZ, -60.0f, 60.0f);
            break;
        case '[':
            pushState3d(x, y, z, direction, directionZ, length, thickness);
            length *= _branchRatio;
            thickness *= 0.8f;
            break;
        case ']':
            if (_stackTop >= 0) {
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
            if (i + 1 < strlen(_lsystem) && _lsystem[i + 1] != '[') {
                drawLeaf3d(x, y, z, direction, directionZ, _season);
            }
            break;
        }
    }

    rotation += 10.0f * dt;
    t0 = t;
    glUseProgram(0);
}
