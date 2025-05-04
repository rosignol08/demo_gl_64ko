#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dm.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4dh.h>
#include <math.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

static void init(void);
static void draw(void);
static void drawLotus(float x, float y, float t);
static void drawKoiFish(float centerX, float centerY, float t);
/* Programme shader pour les lignes */
GLuint _pId_intro= 0;
/* Vertex Array Object pour stocker la géométrie */
GLuint _vao = 0;
/* Buffer pour les sommets */
GLuint _vbo = 0;

void intro_arabesque(int state) {
    switch(state) {
    case GL4DH_INIT:
        init();
        return;
    case GL4DH_FREE:
        if(_vao) {
            glDeleteVertexArrays(1, &_vao);
            _vao = 0;
        }
        if(_vbo) {
            glDeleteBuffers(1, &_vbo);
            _vbo = 0;
        }
        return;
    case GL4DH_UPDATE_WITH_AUDIO:
        return;
    default: /* GL4DH_DRAW */
        draw();
        return;
    }
}

void init(void) {
    //Nouvelle manière include direct avec imfs (merci julien)
    const char * imfs = "<imfs>intro.fs</imfs>\n"
    #ifdef __GLES4D__
        "#version 300 es\n"
        "precision mediump float;\n"
    #else
        "#version 330 core\n"
    #endif
        "uniform vec4 color;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    fragColor = color;\n"
        "}\n";

    const char * imvs = "<imvs>intro.vs</imvs>\n"
    #ifdef __GLES4D__
        "#version 300 es\n"
    #else
        "#version 330 core\n"
    #endif
        "in vec2 position;\n"
        "uniform mat4 modelview;\n"
        "uniform mat4 proj;\n"
        "void main() {\n"
        "    gl_Position = proj * modelview * vec4(position, 0.0, 1.0);\n"
        "}\n";
    _pId_intro = gl4duCreateProgram(imvs, imfs, NULL);

    /* Création du VAO et VBO pour dessiner des lignes */
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);

    //le viewport et les matrices de projection
    gl4duGenMatrix(GL_FLOAT, "proj");
    gl4duGenMatrix(GL_FLOAT, "modelview");
    gl4duBindMatrix("proj");
    gl4duLoadIdentityf();
    gl4duOrthof(-SCREEN_WIDTH/2, SCREEN_WIDTH/2, -SCREEN_HEIGHT/2, SCREEN_HEIGHT/2, -1, 1);
}

void draw(void) {
    static float t = 0;
    static double t0 = 0;
    double t1 = gl4dhGetTicks() / 1000.0;
    float dt = (float)(t1 - t0);
    t0 = t1;

    //noir
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //antialiasing
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.5f); //epaisseur des lignes

    //activation du shader
    glUseProgram(_pId_intro);

    //config la matric avec zoom
    gl4duBindMatrix("modelview");
    gl4duLoadIdentityf();
    //zoom 1.5f
    gl4duScalef(1.5f, 1.5f, 1.0f);
    gl4duSendMatrices();

    //fleur au centre
    drawLotus(0, 0, t);

    //d'autres fleurs autour
    for (int i = 0; i < 5; i++) {
        float angle = 2.0f * M_PI * i / 5;
        
        float radius = 120.0f; 
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        drawLotus(x, y, t + i * 0.5f);
    }
    //carpe koi
    drawKoiFish(0, 0, t);

    //desactivation du shader
    glUseProgram(0);

    //maj du temps
    t += dt;
}

static void drawKoiFish(float centerX, float centerY, float t) {
    // La carpe koi se déplace en cercle
    float radius = 80.0f;
    float x = centerX + radius * cos(t * 0.5f);
    float y = centerY + radius * sin(t * 0.5f);

    //calcule de l'angle de direction
    float angle = t * 0.5f + M_PI / 2;

    //longueur et largeur du corps du poisson
    float fishLength = 70.0f;
    float fishWidth = 30.0f;

    //points pour le corps ovale
    const int numBodyPoints = 24;  //plus de points pour un ovale plus lisse
    float vertices[numBodyPoints * 2];

    //centre
    vertices[0] = x;
    vertices[1] = y;

    //un ovale pour le corps
    for (int i = 0; i < numBodyPoints - 1; i++) {
        float circleAngle = 2.0f * M_PI * i / (numBodyPoints - 2);
        // Ovale: x = a*cos(t), y = b*sin(t) mais orienté selon angle
        float dx = fishLength/2 * cos(circleAngle);
        float dy = fishWidth/2 * sin(circleAngle);

        //rotate du point selon l'angle de direction
        vertices[(i+1)*2] = x + dx * cos(angle) - dy * sin(angle);
        vertices[(i+1)*2+1] = y + dx * sin(angle) + dy * cos(angle);
    }

    //queue (éventail à l'arrière)
    float tailVertices[8];
    // Point d'attache de la queue
    tailVertices[0] = x - (fishLength/2 - 5) * cos(angle);
    tailVertices[1] = y - (fishLength/2 - 5) * sin(angle);

    //points de la queue en forme d'éventail (comme vue du dessus)
    float tailWidth = fishWidth * 1.2f;
    tailVertices[2] = x - (fishLength/2 + 25) * cos(angle) + tailWidth/2 * sin(angle) * sin(t * 5);
    tailVertices[3] = y - (fishLength/2 + 25) * sin(angle) - tailWidth/2 * cos(angle) * sin(t * 5);

    tailVertices[4] = x - (fishLength/2 + 30) * cos(angle);
    tailVertices[5] = y - (fishLength/2 + 30) * sin(angle);

    tailVertices[6] = x - (fishLength/2 + 25) * cos(angle) - tailWidth/2 * sin(angle) * sin(t * 5);
    tailVertices[7] = y - (fishLength/2 + 25) * sin(angle) + tailWidth/2 * cos(angle) * sin(t * 5);

    //draw le corps
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, numBodyPoints * 2 * sizeof(float), vertices, GL_DYNAMIC_DRAW);

    GLint posLoc = glGetAttribLocation(_pId_intro, "position");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLint colorLoc = glGetUniformLocation(_pId_intro, "color");
    if (colorLoc >= 0) {
        // Couleur blanche pour la carpe koi
        glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, numBodyPoints);

    //draw la queue
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), tailVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    //draw des yeux (plus petits, sur les côtés de la tête)
    float eyeOffset = fishWidth * 0.3f;
    float eyeForward = fishLength * 0.35f;
    float eyeSize = 3.0f;

    // Œil gauche
    float leftEyeX = x + eyeForward * cos(angle) - eyeOffset * sin(angle);
    float leftEyeY = y + eyeForward * sin(angle) + eyeOffset * cos(angle);

    // Œil droit
    float rightEyeX = x + eyeForward * cos(angle) + eyeOffset * sin(angle);
    float rightEyeY = y + eyeForward * sin(angle) - eyeOffset * cos(angle);

    const int numEyePoints = 8;
    float eyeVertices[numEyePoints * 2];

    //draw l'œil gauche
    eyeVertices[0] = leftEyeX;
    eyeVertices[1] = leftEyeY;

    for (int i = 0; i < numEyePoints - 1; i++) {
        float eyeAngle = 2.0f * M_PI * i / (numEyePoints - 2);
        eyeVertices[(i+1)*2] = leftEyeX + eyeSize * cos(eyeAngle);
        eyeVertices[(i+1)*2 + 1] = leftEyeY + eyeSize * sin(eyeAngle);
    }

    glBufferData(GL_ARRAY_BUFFER, numEyePoints * 2 * sizeof(float), eyeVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    if (colorLoc >= 0) {
        glUniform4f(colorLoc, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, numEyePoints);

    //draw  l'œil droit
    eyeVertices[0] = rightEyeX;
    eyeVertices[1] = rightEyeY;

    for (int i = 0; i < numEyePoints - 1; i++) {
        float eyeAngle = 2.0f * M_PI * i / (numEyePoints - 2);
        eyeVertices[(i+1)*2] = rightEyeX + eyeSize * cos(eyeAngle);
        eyeVertices[(i+1)*2 + 1] = rightEyeY + eyeSize * sin(eyeAngle);
    }

    glBufferData(GL_ARRAY_BUFFER, numEyePoints * 2 * sizeof(float), eyeVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLE_FAN, 0, numEyePoints);

    //draw  une seule tache rouge sur la tête
    const int numSpotPoints = 12;
    float spotVertices[numSpotPoints * 2];

    //tache rouge au milieu de la tête
    float spotX = x + fishLength * 0.3f * cos(angle);
    float spotY = y + fishLength * 0.3f * sin(angle);
    float spotSize = 8.0f;

    spotVertices[0] = spotX;
    spotVertices[1] = spotY;

    for (int i = 0; i < numSpotPoints - 1; i++) {
        float spotAngle = 2.0f * M_PI * i / (numSpotPoints - 2);
        spotVertices[(i+1)*2] = spotX + spotSize * cos(spotAngle);
        spotVertices[(i+1)*2 + 1] = spotY + spotSize * sin(spotAngle);
    }

    glBufferData(GL_ARRAY_BUFFER, numSpotPoints * 2 * sizeof(float), spotVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    if (colorLoc >= 0) {
        //tache rouge
        glUniform4f(colorLoc, 1.0f, 0.0f, 0.0f, 1.0f);
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, numSpotPoints);

    //draw  les nageoires
    const int numFinPoints = 6;
    float finVertices[numFinPoints * 2];

    //nageoire gauche - reculée par rapport au corps du poisson
    float leftFinX = x + fishLength * 0.1f * cos(angle) - fishWidth * 0.4f * sin(angle);
    float leftFinY = y + fishLength * 0.1f * sin(angle) + fishWidth * 0.4f * cos(angle);
    float finSize = 10.0f;

    finVertices[0] = leftFinX;
    finVertices[1] = leftFinY;

    for (int i = 0; i < numFinPoints - 1; i++) {
        float finAngle = M_PI * i / (numFinPoints - 2);
        finVertices[(i+1)*2] = leftFinX + finSize * cos(finAngle + angle - M_PI/2);
        finVertices[(i+1)*2 + 1] = leftFinY + finSize * sin(finAngle + angle - M_PI/2);
    }

    glBufferData(GL_ARRAY_BUFFER, numFinPoints * 2 * sizeof(float), finVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    if (colorLoc >= 0) {
        glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, numFinPoints);

    //nageoire droite - reculée par rapport au corps du poisson
    float rightFinX = x + fishLength * 0.1f * cos(angle) + fishWidth * 0.4f * sin(angle);
    float rightFinY = y + fishLength * 0.1f * sin(angle) - fishWidth * 0.4f * cos(angle);

    finVertices[0] = rightFinX;
    finVertices[1] = rightFinY;

    for (int i = 0; i < numFinPoints - 1; i++) {
        float finAngle = M_PI * i / (numFinPoints - 2);
        finVertices[(i+1)*2] = rightFinX + finSize * cos(finAngle + angle - M_PI/2);
        finVertices[(i+1)*2 + 1] = rightFinY + finSize * sin(finAngle + angle - M_PI/2);
    }

    glBufferData(GL_ARRAY_BUFFER, numFinPoints * 2 * sizeof(float), finVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLE_FAN, 0, numFinPoints);

    //nettoyage
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
static void drawLotus(float x, float y, float t) {
    const int numPoints = 240; // Augmenter le nombre de points pour des pétales plus fins
    const int numPetals = 12;  // Plus de pétales
    const float baseRadius = 50.0f;
    const float radiusVar = 15.0f;

    // Allouer de la mémoire pour les points
    float *vertices = malloc(numPoints * 2 * sizeof(float));
    if (!vertices) return;

    // Générer les points des pétales de lotus avec longueur uniforme
    for (int i = 0; i < numPoints; i++) {
        float angle = 2.0f * M_PI * i / (float)numPoints;
        // Rotation basée sur le temps (on garde uniquement la rotation)
        angle += t * 0.3f;

        // Calculer la position angulaire dans le motif de pétale
        // Enlevé la variable t ici pour éviter que les pétales changent de taille
        float petalAngle = angle * numPetals;
        
        // Utiliser une fonction qui maintient un rayon constant mais module la forme
        // abs(sin) donne des pétales pointus de même longueur
        float petalFactor = 1.0f - 0.3f * fabs(sin(petalAngle));
        
        // Rayon constant modulé par le facteur de pétale
        float r = baseRadius * petalFactor;
        
        // Calculer la position
        vertices[i*2] = x + r * cos(angle);
        vertices[i*2+1] = y + r * sin(angle);
    }

    // Dessiner les pétales de lotus
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, numPoints * 2 * sizeof(float), vertices, GL_DYNAMIC_DRAW);

    GLint posLoc = glGetAttribLocation(_pId_intro, "position");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLint colorLoc = glGetUniformLocation(_pId_intro, "color");

    // Dessiner la première couche de pétales
    if (colorLoc >= 0) {
        // Couleur bleue pour les pétales de lotus
        glUniform4f(colorLoc, 0.0f, 0.5f, 1.0f, 1.0f);
    }
    glDrawArrays(GL_LINE_LOOP, 0, numPoints);

    // Dessiner des lignes croisées pour remplir le motif
    //for (int i = 0; i < numPoints / 2; i++) {
    //    float lines[4];
    //    int opposite = (i + numPoints / 2) % numPoints;
//
    //    lines[0] = vertices[i*2];
    //    lines[1] = vertices[i*2+1];
    //    lines[2] = vertices[opposite*2];
    //    lines[3] = vertices[opposite*2+1];
//
    //    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), lines, GL_DYNAMIC_DRAW);
    //    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
//
    //    if (colorLoc >= 0) {
    //        glUniform4f(colorLoc, 0.0f, 0.5f, 1.0f, 0.3f);  // Transparence pour voir les lignes superposées
    //    }
//
    //    if (i % 5 == 0) { // Dessiner seulement quelques lignes pour ne pas surcharger
    //        glDrawArrays(GL_LINES, 0, 2);
    //    }
    //}

    // Dessiner les couches supplémentaires de pétales
    for (int layer = 1; layer < 3; layer++) {
        float layerRadius = baseRadius - layer * 15.0f;
        float layerRadiusVar = radiusVar - layer * 5.0f;

        for (int i = 0; i < numPoints; i++) {
            float angle = 2.0f * M_PI * i / (float)numPoints;
            angle += t * 0.3f;

            float r = layerRadius + layerRadiusVar * sin(angle * numPetals + t);
            r += 10.0f * sin(angle * 4.0f + t * 2.0f);

            vertices[i*2] = x + r * cos(angle);
            vertices[i*2+1] = y + r * sin(angle);
        }

        glBufferData(GL_ARRAY_BUFFER, numPoints * 2 * sizeof(float), vertices, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

        if (colorLoc >= 0) {
            // Couleur plus claire pour les couches supérieures
            glUniform4f(colorLoc, 0.5f - layer * 0.2f, 0.7f - layer * 0.2f, 1.0f, 1.0f);
        }

        glDrawArrays(GL_LINE_LOOP, 0, numPoints);
    }

    // Dessiner le centre jaune
    const int numCenterPoints = 13;  // Augmenté à 13 pour avoir un cercle complet
    float centerVertices[numCenterPoints * 2];
    float centerRadius = 10.0f;

    centerVertices[0] = x;
    centerVertices[1] = y;

    for (int i = 0; i < numCenterPoints - 1; i++) {
        float angle = 2.0f * M_PI * i / (float)(numCenterPoints - 2);  // Divisé par numCenterPoints-2 pour fermer le cercle
        centerVertices[(i+1)*2] = x + centerRadius * cos(angle);
        centerVertices[(i+1)*2+1] = y + centerRadius * sin(angle);
    }

    glBufferData(GL_ARRAY_BUFFER, numCenterPoints * 2 * sizeof(float), centerVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    if (colorLoc >= 0) {
        // Couleur jaune pour le centre
        glUniform4f(colorLoc, 1.0f, 1.0f, 0.0f, 1.0f);
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, numCenterPoints);

    // Nettoyer
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    free(vertices);
}
