/*!\file eau_phy.c
 * \brief GL4Dummies, scene de physique de l'eau
 * \author Romaric chaffray
 * \date 28 04 2025
 */
#include <GL4D/gl4du.h>
#include <GL4D/gl4df.h>
#include <GL4D/gl4dh.h>
#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dm.h>
#include <GL4D/gl4dg.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define MAX_NEIGHBOURS 32
#define HASH_SIZE 2000
#define CELL_SIZE 0.1f // taille des cellules pour la grille spatiale

// pour la gravitée radiale
#define G_CONSTANT 6.67430e-3 // Constante gravitationnelle modifiée pour l'échelle de la simulation
#define MIN_DISTANCE 0.01f    // Distance minimale pour éviter les accélérations infinies

float vitesse = 1.0f;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// macro pour les expressions mathématiques complexes
#define POLY6 (315.0f / (64.0f * M_PI * powf(H, 9)))
#define SPIKY_GRAD (-45.0f / (M_PI * powf(H, 6)))
#define VISC_LAP (45.0f / (M_PI * powf(H, 6)))
#define SURFACE_TENSION 0.0728f

// pour les rectangles 3D
typedef struct
{
    float x, y, z;
    float w, h, d;
    float angle_x; // Angle de rotation sur l'axe x
} rect3d_t;

// pour les poissons ovales
typedef struct
{
    float x, y, z;
    float w, h, d;
    float angle_x; // Angle de rotation sur l'axe x
} oval3d_t;

// tableau dynamique global
static rect3d_t *_rects = NULL;
static int _nb_rects = 0;
static int _max_rects = 0;

// pour les poissons (des formes ovales)
static oval3d_t *_ovals = NULL;
static int _nb_poisson = 0;
static int _max_poisson = 0;

// struct pour la grille spatiale
typedef struct
{
    int start_index;
    int count;
} spatial_cell_t;

// variables globales pour l'optimisation
static spatial_cell_t *_grid_phy = NULL;
static int *_cell_indices = NULL;
static int *_particle_indices = NULL;

typedef struct vec3d_t vec3d_t;
typedef struct mobile_t mobile_t;

struct vec3d_t
{
    GLfloat x, y, z;
};

// temps de simulation
static float TIME_SCALE = 10.1f; // vitesse de simulation

static void init(void);
static void draw(void);

static void mobile_init(int n);
static void mobile_simu(void);
static void mobile_draw(void);
bool verify_initialization();

/* on créé une variable pour stocker l'identifiant du programme GPU */
GLuint _pId_phy = 0;
GLuint _pId_phy_particules = 0;
GLuint _pId_water = 0;
GLuint _quad_phy = 0;

// VBO et VAO pour les particules
GLuint particle_vbo;
GLuint particle_vao;

//pour les rectangles d'eau
GLuint _water_vao, _water_vbo;

// les poissons qui nagent
float poission_position[2] = {0.0f, 0.0f};
float poission_position_y = 0.0f;
/* gravité */
// static GLfloat _ig = 9.81f / 2.0f;
static vec3d_t _g = {0.0f, 0.0f, 0.0f}; // Modification ici: définir la gravité vers le bas à -9.81f
static const GLfloat e = 0.5f;          // 8.0f / 9.0f;

/* simulation d'eau de jsp qui */
// Ajouter ces paramètres SPH
static const float REST_DENSITY = 300.0f;  // Densité au repos du fluide
static const float GAS_CONSTANT = 2000.0f; // Constante des gaz parfaits
static const float VISCOSITY = 10.0f;      // Viscosité du fluide
static const float MASS = 1.0f;            // Masse d'une particule
static const float H = 0.11f;              // Rayon de lissage (smoothing radius)
static const float H2 = 0.0075f;           // H²

// Variables fluides à ajouter à la structure mobile_t
struct mobile_t
{
    vec3d_t p, v;
    GLfloat r;
    GLfloat color[4];
    float activation_time; //pour dessiner pas en meme temps
    bool is_active;         //pour activer/désactiver des particules
    // Variables SPH
    float density;
    float pressure;
    vec3d_t force;
    int cell_id;
};

/* tous les mobiles de ma scène */
static mobile_t *_mobiles = NULL;
static int _nb_mobiles = 0;
static double _scene_start_time = 0.0;
static float scene_time = 0.0f;
bool a_deja_avance = false;
//static double t0 = 0.0;
void eau_scene(int state)
{
    static int init_attempts = 0;
    const int MAX_INIT_ATTEMPTS = 3;
    
    switch (state)
    {
    case GL4DH_INIT:
            init_attempts = 0;
        do {
            // Si ce n'est pas la première tentative, nettoyer d'abord
            if (init_attempts > 0) {
                printf("Initialization attempt %d failed. Retrying...\n", init_attempts);
                // Free mobile particles
        // Free mobile particles
        if (_mobiles)
        {
            free(_mobiles);
            _mobiles = NULL;
            _nb_mobiles = 0;
        }

        // Free spatial grid structures
        if (_grid_phy)
        {
            free(_grid_phy);
            _grid_phy = NULL;
        }
        if (_cell_indices)
        {
            free(_cell_indices);
            _cell_indices = NULL;
        }
        if (_particle_indices)
        {
            free(_particle_indices);
            _particle_indices = NULL;
        }

        // Delete shader programs
        if (_pId_phy)
        {
            glDeleteProgram(_pId_phy);
            _pId_phy = 0;
        }
        if (_pId_phy_particules)
        {
            glDeleteProgram(_pId_phy_particules);
            _pId_phy_particules = 0;
        }
        if (_pId_water)
        {
            glDeleteProgram(_pId_water);
            _pId_water = 0;
        }

        // Delete OpenGL objects
        if (_quad_phy)
        {
            gl4dgDelete(_quad_phy);
            _quad_phy = 0;
        }
        if (particle_vao)
        {
            glDeleteVertexArrays(1, &particle_vao);
            particle_vao = 0;
        }
        if (particle_vbo)
        {
            glDeleteBuffers(1, &particle_vbo);
            particle_vbo = 0;
        }
        if (_water_vao)
        {
            glDeleteVertexArrays(1, &_water_vao);
            _water_vao = 0;
        }
        if (_water_vbo)
        {
            glDeleteBuffers(1, &_water_vbo);
            _water_vbo = 0;
        }

        // Free rectangles
        if (_rects)
        {
            free(_rects);
            _rects = NULL;
            _nb_rects = 0;
            _max_rects = 0;
        }
        
        // Free ovals (fish)
        if (_ovals)
        {
            free(_ovals);
            _ovals = NULL;
            _nb_poisson = 0;
            _max_poisson = 0;
        }
        
        // Reset other variables to default state
        _g.x = 0.0f;
        _g.y = 0.0f;
        _g.z = 0.0f;
        
        //printf("Resources freed for re-initialization attempt %d\n", init_attempts);
    }// Initialiser
    a_deja_avance = false;
            init();
            _scene_start_time = gl4dGetElapsedTime() / 1000.0;
            //printf("Scene started at time: %.2f seconds\n", _scene_start_time);//TODO enlever
            
            init_attempts++;
            
            // Vérifier si tout est correctement initialisé
            if (verify_initialization()) {
                printf("Initialization successful on attempt %d!\n", init_attempts);
                break;
            }
            
            // Si nous avons atteint le nombre maximum de tentatives, afficher un message d'erreur
            if (init_attempts >= MAX_INIT_ATTEMPTS) {
                fprintf(stderr, "ERROR: Failed to initialize after %d attempts. Continuing with partial initialization.\n", MAX_INIT_ATTEMPTS);
                break;
            }
            
        } while (init_attempts < MAX_INIT_ATTEMPTS);
        
        return;
    case GL4DH_FREE:
    {
        // Free mobile particles
        if (_mobiles)
        {
            free(_mobiles);
            _mobiles = NULL;
            _nb_mobiles = 0;
        }

        // Free spatial grid structures
        if (_grid_phy)
        {
            free(_grid_phy);
            _grid_phy = NULL;
        }
        if (_cell_indices)
        {
            free(_cell_indices);
            _cell_indices = NULL;
        }

        if (_particle_indices)
        {
            free(_particle_indices);
            _particle_indices = NULL;
        }

        // Free shader programs
        if (_pId_phy)
        {
            glDeleteProgram(_pId_phy);
            _pId_phy = 0;
        }

        if (_pId_phy_particules)
        {
            glDeleteProgram(_pId_phy_particules);
            _pId_phy_particules = 0;
        }

        // Free OpenGL objects
        if (_quad_phy)
        {
            //gl4dgDelete(_quad_phy);
            _quad_phy = 0;
        }

        if (particle_vao)
        {
            glDeleteVertexArrays(1, &particle_vao);
            particle_vao = 0;
        }

        if (particle_vbo)
        {
            glDeleteBuffers(1, &particle_vbo);
            particle_vbo = 0;
        }

        // Free rectangles
         if (_rects) {
             free(_rects);
             _rects = NULL;
             _nb_rects = 0;
             _max_rects = 0;
         }
        
        // Free ovals (fish)
         if (_ovals) {
             free(_ovals);
             _ovals = NULL;
             _nb_poisson = 0;
             _max_poisson = 0;
         }
         if (_water_vao) {
    glDeleteVertexArrays(1, &_water_vao);
    _water_vao = 0;
}
if (_water_vbo) {
    glDeleteBuffers(1, &_water_vbo);
    _water_vbo = 0;
}
if (_pId_water) {
    glDeleteProgram(_pId_water);
    _pId_water = 0;
}
    }
        return;
    case GL4DH_UPDATE_WITH_AUDIO:

        return;
    default: // GL4DH_DRAW
        draw();
        return;
    }
}
// Fonctions auxiliaires pour SPH
float kernel_poly6(float r2)
{
    if (r2 > H2)
        return 0.0f;
    float temp = H2 - r2;
    return POLY6 * temp * temp * temp;
}

float kernel_spiky_gradient(float r)
{
    if (r > H)
        return 0.0f;
    float temp = H - r;
    return SPIKY_GRAD * temp * temp;
}

float kernel_viscosity_laplacian(float r)
{
    if (r > H)
        return 0.0f;
    return VISC_LAP * (H - r);
}
float kernel_viscosity_improved(float r, float h)
{
    if (r >= h)
        return 0.0f;
    float q = r / h;
    return (h / r) * (1.0f - q);
}

// les rectangles
void rect_init_list(int capacity)
{
    if (_rects)
        free(_rects);
    _max_rects = capacity;
    _rects = (rect3d_t *)malloc(_max_rects * sizeof(rect3d_t));
    _nb_rects = 0;
}
// Ajoute un rectangle à la liste
void rect_add(float x, float y, float z, float w, float h, float d, float angle_x)
{
    if (_nb_rects >= _max_rects)
        return; // ou agrandir dynamiquement
    _rects[_nb_rects].x = x;
    _rects[_nb_rects].y = y;
    _rects[_nb_rects].z = z;
    _rects[_nb_rects].w = w;
    _rects[_nb_rects].h = h;
    _rects[_nb_rects].d = d;
    _rects[_nb_rects].angle_x = angle_x; // Ajout de l'angle de rotation sur l'axe x
    _nb_rects++;
}

// Test collision (2D) d'une particule (mobile_t) avec un rectangle avec rotation
static void collide_with_rotated_rect(mobile_t *m, const rect3d_t *r, float e)
{
    // Translate the particle's position to the rectangle's local space
    float localX = m->p.x - r->x;
    float localY = m->p.y - r->y;

    // Apply rotation
    float cosAngle = cosf(r->angle_x);
    float sinAngle = sinf(r->angle_x);
    float rotatedX = cosAngle * localX + sinAngle * localY;
    float rotatedY = -sinAngle * localX + cosAngle * localY;

    // Check if the rotated coordinate is within the rectangle's bounds
    if (rotatedX >= 0.0f && rotatedX <= r->w &&
        rotatedY >= 0.0f && rotatedY <= r->h)
    {

        // Correct the particle's position to push it outside the rectangle
        if (rotatedY < r->h / 2.0f)
        {
            rotatedY = -m->r; // Push below
        }
        else
        {
            rotatedY = r->h + m->r; // Push above
        }

        // Apply inverse rotation to return to global space
        float correctedX = cosAngle * rotatedX - sinAngle * rotatedY;
        float correctedY = sinAngle * rotatedX + cosAngle * rotatedY;

        // Update the particle's position and velocity
        m->p.x = correctedX + r->x;
        m->p.y = correctedY + r->y;
        m->v.y = -m->v.y * e;
    }
}

// Dessine tous les rectangles en utilisant le shader program
void rect_draw_all(void)
{
    GLfloat *rect_data = malloc(4 * _nb_rects * sizeof *rect_data); // 4 pour x,y,w,h
    assert(rect_data);

    for (int i = 0; i < _nb_rects; ++i)
    {
        rect_data[4 * i + 0] = _rects[i].x;
        rect_data[4 * i + 1] = _rects[i].y;
        rect_data[4 * i + 2] = _rects[i].w;
        rect_data[4 * i + 3] = _rects[i].h;
    }
    // Set up an attribute for angles if needed
    glUseProgram(_pId_phy);
    // allocation séparée pour les angles
    float *angles = malloc(_nb_rects * sizeof(float));
    assert(angles);
    for (int i = 0; i < _nb_rects; ++i)
    {
        angles[i] = _rects[i].angle_x;
    }
    glUniform1fv(glGetUniformLocation(_pId_phy, "rect_angles"), _nb_rects, angles);
    glUseProgram(_pId_phy);
    glUniform4fv(glGetUniformLocation(_pId_phy, "rectangles"), _nb_rects, rect_data);
    glUniform1i(glGetUniformLocation(_pId_phy, "nb_rects"), _nb_rects);

    // set la couleur en blanc
    glUniform4f(glGetUniformLocation(_pId_phy, "rect_color"), 0.6f, 0.6f, 0.6f, 1.0f);
    for (int i = 0; i < _nb_mobiles; ++i)
    {
        for (int j = 0; j < _nb_rects; ++j)
        {
            collide_with_rotated_rect(&_mobiles[i], &_rects[j], e);
        }
    }
    gl4dgDraw(_quad_phy);
    glUseProgram(0);

    free(rect_data);
    free(angles);
}


// Appeler cette fonction dans mobile_simu après mise à jour des particules
void rect_collide_all(mobile_t *mobiles, int nb, float e)
{
    for (int i = 0; i < nb; i++)
    {
        for (int j = 0; j < _nb_rects; j++)
        {
            collide_with_rotated_rect(&mobiles[i], &_rects[j], e);
        }
    }
}

void oval_init_list(int capacity)
{
    if (_ovals)
        free(_ovals);
    _max_poisson = capacity;
    _ovals = (oval3d_t *)malloc(_max_poisson * sizeof(oval3d_t));
    _nb_poisson = 0;
}

// Ajoute un poisson à la liste
// x, y, z : position du poisson, w, h : largeur et hauteur de l'oval
void oval_add(float x, float y, float z, float w, float h, float d, float angle_x)
{
    if (_nb_poisson >= _max_poisson)
        return; // ou agrandir dynamiquement
    _ovals[_nb_poisson].x = x;
    _ovals[_nb_poisson].y = y;
    _ovals[_nb_poisson].z = z;
    _ovals[_nb_poisson].w = w;
    _ovals[_nb_poisson].h = h;
    _ovals[_nb_poisson].d = d;
    _ovals[_nb_poisson].angle_x = angle_x; // Ajout de l'angle de rotation sur l'axe x
    _nb_poisson++;
}

static void collide_with_rotated_oval(mobile_t *m, const oval3d_t *o, float e)
{
    // Translate the particle's position to the oval's local space
    float localX = m->p.x - o->x;
    float localY = m->p.y - o->y;

    // Apply rotation
    float cosAngle = cosf(o->angle_x);
    float sinAngle = sinf(o->angle_x);
    float rotatedX = cosAngle * localX + sinAngle * localY;
    float rotatedY = -sinAngle * localX + cosAngle * localY;

    // Check if the rotated coordinate is within the oval's bounds
    if (rotatedX >= 0.0f && rotatedX <= o->w &&
        rotatedY >= 0.0f && rotatedY <= o->h)
    {

        // Correct the particle's position to push it outside the oval
        if (rotatedY < o->h / 2.0f)
        {
            rotatedY = -m->r; // Push below
        }
        else
        {
            rotatedY = o->h + m->r; // Push above
        }

        // Apply inverse rotation to return to global space
        float correctedX = cosAngle * rotatedX - sinAngle * rotatedY;
        float correctedY = sinAngle * rotatedX + cosAngle * rotatedY;

        // Update the particle's position and velocity
        m->p.x = correctedX + o->x;
        m->p.y = correctedY + o->y;
        m->v.y = -m->v.y * e;
    }
}

// fonction pour mettre à jour la position des poissons
void update_fish_positions(float dt) {
    //static float scene_time = 0.0f;
    static int phase = 0; // 0: move right, 1: move up, 2: move right again
    static float initial_y_phase2 = 0.0f;
    //printf("position du poisson de base: %f %f\n", poission_position[0], poission_position[1]);
    //if (a_deja_avance == false){
    //    //printf("position du poisson avant init: %f %f\n", poission_position[0], poission_position[1]);
    //    poission_position[0] *= -1.0f;
    //    poission_position[1] = -0.5f;
    //    a_deja_avance = true;
    //    //printf("position du poisson apres init: %f %f\n", poission_position[0], poission_position[1]);
    //}
    //printf("position du poisson courrante: %f %f\n", poission_position[0], poission_position[1]);
    scene_time += dt;

    // Changement de phase
    if (phase == 0 && poission_position[0] >= 0.8f) {
        phase = 1;
    } else if (phase == 1 && poission_position[1] >= 0.7f) {
        phase = 2;
        initial_y_phase2 = poission_position[1]; // Fixe Y au moment de la transition
    }

    // === Déplacements selon la phase ===
    if (phase == 0) {
        // Phase 0 : avancer vers la droite avec montée linéaire
        poission_position[0] = -0.8f + scene_time * 0.1f;
        poission_position[1] = -0.5f + scene_time * 0.02f; // Starting low and rising linearly
        // Small vertical oscillation added to the linear upward movement
        //poission_position[1] += ((0.02f * sinf(time)) * dt )* 0.00f;
    }
    else if (phase == 1) {
        // Phase 1 : monter avec légère oscillation horizontale
        poission_position[0] = 0.8f + 0.01f * sinf(scene_time * 2.0f);
        poission_position[1] += dt * 0.15f; // montée linéaire
    }
    else if (phase == 2) {
        // Phase 2 : avancer vers la droite, sans changer Y
        poission_position[0] += dt * 0.05f;
        poission_position[1] = initial_y_phase2;
    }

    // === Orientation ===
    float angle = 0.0f;
    if (phase == 0 || phase == 2) {
        // rotate the angle through full 360 degrees
        angle = 90.0f; // orientation horizontale
        //printf("Fish angle: %.1f degrees\n", angle * (180.0f / M_PI));//angle = 90.0f; // orientation horizontale
    } else if (phase == 1) {
        angle = 160.0f; // orientation verticale
    }


    // === Mise à jour des données du poisson ===
    if (_nb_poisson > 0) {
        _ovals[0].x = poission_position[0];
        _ovals[0].y = poission_position[1];
        _ovals[0].angle_x = angle;
    }
}

//fonction où le poisson avance et recule 
void update_fish_positions_stagne(float dt) {
    
    static int phase = 0; // 0: move right, 1: move up, 2: move down
    static float target_y = 0.0f;
    scene_time += dt;
    // Update fish state based on position
    if (phase == 0 && poission_position[0] >= 0.8f) {
        phase = 1; // Switch to moving up
    }
    else if (phase == 1 && poission_position[1] >= 0.5f) {
        phase = 2; // Switch to moving down
        target_y = 0.0f;
    }
    else if (phase == 2 && poission_position[1] <= -0.20f) {
        phase = 1; // Switch back to moving up
    }
    // Update position based on current phase
    if (phase == 0) {
        // Phase 0: Move right with a slight oscillation in Y until x = 0.5
        poission_position[0] = -0.8f + scene_time * 0.05f;//+= 0.01f; // Move right
        // Add oscillation in Y
        static float local_y_offset = 0.0f;
        local_y_offset += 0.0085f * dt + (0.001f * sinf(scene_time));
        poission_position[1] = -0.5f + local_y_offset; // Move up and down
    }
    else if (phase == 1) {
        // Phase 1: Move up + oscillate in X
        poission_position[0] = 0.8f + 0.01f * sinf(scene_time * 2.0f); // Oscillate in X
        poission_position[1] += 0.08f * dt; // Move up slowly
    }
    else if (phase == 2) {
        // Phase 2: Move down quickly
        poission_position[1] -= 0.03f * dt; // Move down fast
        poission_position[0] = 0.8f + 0.01f * sinf(scene_time * 3.0f); // Slight oscillation
    }
    
    // Update fish orientation based on movement direction
    float angle = 0.0f;
    if (phase == 0 || phase == 2) {
        // rotate the angle through full 360 degrees
        angle = 90.0f; // orientation horizontale
        //printf("Fish angle: %.1f degrees\n", angle * (180.0f / M_PI));//angle = 90.0f; // orientation horizontale
    } else if (phase == 1) {
        angle = 160.0f; // orientation verticale
    }
    
    // Update fish in the oval array
    if (_nb_poisson > 0) {
        _ovals[0].x = poission_position[0];
        _ovals[0].y = poission_position[1];
        _ovals[0].angle_x = angle;
    }
}




void oval_collide_all(mobile_t *mobiles, int nb, float e)
{
    for (int i = 0; i < nb; i++)
    {
        for (int j = 0; j < _nb_poisson; j++)
        {
            collide_with_rotated_oval(&mobiles[i], &_ovals[j], e);
        }
    }
}

// Fonction pour construire la grille spatiale
void build_spatial_grid_phy()
{
    // Initialiser la grille
    if (!_grid_phy)
    {
        _grid_phy = (spatial_cell_t *)malloc(HASH_SIZE * sizeof(spatial_cell_t));
        _cell_indices = (int *)malloc(_nb_mobiles * sizeof(int));
        _particle_indices = (int *)malloc(_nb_mobiles * sizeof(int));
    }

    // Réinitialiser les cellules
    for (int i = 0; i < HASH_SIZE; i++)
    {
        _grid_phy[i].start_index = -1;
        _grid_phy[i].count = 0;
    }

    // Attribuer les particules aux cellules
    for (int i = 0; i < _nb_mobiles; i++)
    {
        int cellX = (int)(((_mobiles[i].p.x + 1.0f) / 2.0f) / CELL_SIZE);
        int cellY = (int)(((_mobiles[i].p.y + 1.0f) / 2.0f) / CELL_SIZE);

        int cell_id = (cellY * 32 + cellX) % HASH_SIZE; // Hachage simple
        _mobiles[i].cell_id = cell_id;

        // Compter combien de particules par cellule
        _grid_phy[cell_id].count++;
    }

    // Calculer les indices de départ
    int start = 0;
    for (int i = 0; i < HASH_SIZE; i++)
    {
        _grid_phy[i].start_index = start;
        start += _grid_phy[i].count;
        _grid_phy[i].count = 0; // Réinitialiser pour la prochaine étape
    }

    // Remplir les indices
    for (int i = 0; i < _nb_mobiles; i++)
    {
        int cell_id = _mobiles[i].cell_id;
        int index = _grid_phy[cell_id].start_index + _grid_phy[cell_id].count;
        _cell_indices[index] = i;
        _grid_phy[cell_id].count++;
    }
}

// Fonction pour calculer les forces gravitationnelles entre particules
void compute_gravity_forces()
{
    // reset les forces
    for (int i = 0; i < _nb_mobiles; i++)
    {
        _mobiles[i].force.x = 0.0f;
        _mobiles[i].force.y = 0.0f;
        _mobiles[i].force.z = 0.0f;
    }

    // calcule des forces gravitationnelles entre chaque paire de particules
    for (int i = 0; i < _nb_mobiles; i++)
    {
        for (int j = i + 1; j < _nb_mobiles; j++)
        {
            float dx = _mobiles[j].p.x - _mobiles[i].p.x;
            float dy = _mobiles[j].p.y - _mobiles[i].p.y;
            // float dz = _mobiles[j].p.z - _mobiles[i].p.z;

            float r2 = dx * dx + dy * dy; // + dz*dz;
            float r = sqrtf(r2);

            // Éviter division par zéro et forces trop grandes entre particules proches
            if (r < MIN_DISTANCE)
                r = MIN_DISTANCE;

            // force gravitationnelle: F = G * m1 * m2 / r^2
            // toutes les particules ont la même masse (MASS)
            float force = G_CONSTANT * MASS * MASS / r2;

            // direction de la force (vecteur unitaire)
            float fx = force * dx / r;
            float fy = force * dy / r;
            // float fz = force * dz / r;

            // la force aux deux particules (action-réaction)
            _mobiles[i].force.x += fx;
            _mobiles[i].force.y += fy;
            //_mobiles[i].force.z += fz;

            _mobiles[j].force.x -= fx;
            _mobiles[j].force.y -= fy;
            //_mobiles[j].force.z -= fz;

            // Réduire la vitesse des particules pour simuler un amortissement
            _mobiles[i].v.x *= 0.9f; // Réduction de 80% par itération
            _mobiles[i].v.y *= 0.9f;
            //_mobiles[i].v.z *= 0.9f;
            _mobiles[j].v.x *= 0.9f;
            _mobiles[j].v.y *= 0.9f;
            //_mobiles[j].v.z *= 0.9f;
            _mobiles[i].force.x *= 0.1f;
            _mobiles[i].force.y *= 0.1f;
        }
    }
}

// Fonction principale SPH
void compute_sph_forces()
{
    // Construire la grille spatiale
    build_spatial_grid_phy();
    // Calculer les densités et pressions
    for (int i = 0; i < _nb_mobiles; i++)
    {
        _mobiles[i].density = 0.0f;

        // Auto-contribution à la densité
        _mobiles[i].density += MASS * kernel_poly6(0.0f);

        // Contribution des voisins
        // int cell_id = _mobiles[i].cell_id;

        // Parcourir les 9 cellules voisines (en 2D)
        for (int offsetY = -1; offsetY <= 1; offsetY++)
        {
            for (int offsetX = -1; offsetX <= 1; offsetX++)
            {
                int cellX = (int)(((_mobiles[i].p.x + 1.0f) / 2.0f) / CELL_SIZE) + offsetX;
                int cellY = (int)(((_mobiles[i].p.y + 1.0f) / 2.0f) / CELL_SIZE) + offsetY;

                if (cellX < 0 || cellY < 0 || cellX >= 32 || cellY >= 32)
                    continue;

                int neighbor_cell_id = (cellY * 32 + cellX) % HASH_SIZE;

                if (_grid_phy[neighbor_cell_id].start_index == -1)
                    continue;

                // Parcourir les particules de cette cellule
                for (int k = 0; k < _grid_phy[neighbor_cell_id].count; k++)
                {
                    int j = _cell_indices[_grid_phy[neighbor_cell_id].start_index + k];
                    if (i == j)
                        continue;

                    float dx = _mobiles[j].p.x - _mobiles[i].p.x;
                    float dy = _mobiles[j].p.y - _mobiles[i].p.y;
                    float r2 = dx * dx + dy * dy;

                    if (r2 < H2)
                    {
                        _mobiles[i].density += MASS * kernel_poly6(r2);
                    }
                }
            }
        }

        // Calculer la pression avec l'équation d'état
        //_mobiles[i].pressure = GAS_CONSTANT * (_mobiles[i].density - REST_DENSITY);
        float density_ratio = _mobiles[i].density / REST_DENSITY;
        if (density_ratio > 1.0f)
        {
            // Pression positive (répulsive) si densité > REST_DENSITY
            _mobiles[i].pressure = GAS_CONSTANT * (powf(density_ratio, 4) - 1.0f);
        }
        else
        {
            // Pression négative (attractive) si densité < REST_DENSITY, mais plus faible
            _mobiles[i].pressure = GAS_CONSTANT * 2.0f * (density_ratio - 1.0f);
        }
    }

    // Calculer les forces
    for (int i = 0; i < _nb_mobiles; i++)
    {

        // int cell_id = _mobiles[i].cell_id;

        // Parcourir les 9 cellules voisines
        for (int offsetY = -1; offsetY <= 1; offsetY++)
        {
            for (int offsetX = -1; offsetX <= 1; offsetX++)
            {
                int cellX = (int)(((_mobiles[i].p.x + 1.0f) / 2.0f) / CELL_SIZE) + offsetX;
                int cellY = (int)(((_mobiles[i].p.y + 1.0f) / 2.0f) / CELL_SIZE) + offsetY;

                if (cellX < 0 || cellY < 0 || cellX >= 32 || cellY >= 32)
                    continue;

                int neighbor_cell_id = (cellY * 32 + cellX) % HASH_SIZE;

                if (_grid_phy[neighbor_cell_id].start_index == -1)
                    continue;

                // Parcourir les particules de cette cellule
                for (int k = 0; k < _grid_phy[neighbor_cell_id].count; k++)
                {
                    int j = _cell_indices[_grid_phy[neighbor_cell_id].start_index + k];
                    if (i == j)
                        continue;

                    float dx = _mobiles[j].p.x - _mobiles[i].p.x;
                    float dy = _mobiles[j].p.y - _mobiles[i].p.y;
                    float r2 = dx * dx + dy * dy;
                    float r = sqrtf(r2);

                    if (r < H && r > 0.0001f)
                    {
                        // Force de pression
                        float pressure_factor = -MASS * (_mobiles[i].pressure + _mobiles[j].pressure) /
                                                (2.0f * _mobiles[j].density) * kernel_spiky_gradient(r);
                        // printf("Pressure factor: %f\n", pressure_factor);
                        _mobiles[i].force.x += pressure_factor * dx / r;
                        _mobiles[i].force.y += pressure_factor * dy / r;

                        // Force de viscosité
                        float visc_factor = VISCOSITY * MASS *
                                            (_mobiles[j].v.x - _mobiles[i].v.x) *
                                            // kernel_viscosity_laplacian(r) / _mobiles[j].density;
                                            kernel_viscosity_improved(r, H);

                        _mobiles[i].force.x += visc_factor * 0.1f;

                        visc_factor = VISCOSITY * MASS *
                                      (_mobiles[j].v.y - _mobiles[i].v.y) *
                                      // kernel_viscosity_laplacian(r) / _mobiles[j].density;
                                      kernel_viscosity_improved(r, H);

                        _mobiles[i].force.y += visc_factor;
                        float min_distance = _mobiles[i].r + _mobiles[j].r;
                        // Force de répulsion à courte distance (deux-couches)
                        if (r < min_distance * 1.5f)
                        {
                            // Première couche - très forte répulsion si presque en contact
                            if (r < min_distance * 1.1f)
                            {
                                float repulsion_strength = 50.0f * (min_distance * 1.1f - r) / min_distance;
                                _mobiles[i].force.x += repulsion_strength * dx / r;
                                _mobiles[i].force.y += repulsion_strength * dy / r;
                            }
                            // Deuxième couche - répulsion plus douce
                            else
                            {
                                float repulsion_strength = 5.0f * (min_distance * 1.5f - r) / min_distance;
                                _mobiles[i].force.x += repulsion_strength * dx / r;
                                _mobiles[i].force.y += repulsion_strength * dy / r;
                            }
                        }
                        min_distance = _mobiles[i].r + _mobiles[j].r;
                        if (r < min_distance)
                        {
                            float repulsion_strength = 10.0f * (min_distance - r) / min_distance;
                            _mobiles[i].force.x += repulsion_strength * dx / r;
                            _mobiles[i].force.y += repulsion_strength * dy / r;
                        }

                        // Limiter la densité
                        if (_mobiles[i].density > REST_DENSITY * 2.0f)
                        {
                            _mobiles[i].v.x *= 0.5f;
                            _mobiles[i].v.y *= 0.5f;
                        }
                        if (r < min_distance * 1.5f)
                        {
                            float repulsion_strength = 2.0f * (min_distance * 1.5f - r) / min_distance;
                            _mobiles[i].force.x -= repulsion_strength * dx / r;
                            _mobiles[i].force.y -= repulsion_strength * dy / r;
                        }
                    }
                }
            }
        }
        // limiter leur magnitude
        float force_magnitude = sqrtf(_mobiles[i].force.x * _mobiles[i].force.x +
                                      _mobiles[i].force.y * _mobiles[i].force.y);
        const float max_force = 1000.0f;

        if (force_magnitude > max_force)
        {
            float scale = max_force / force_magnitude;
            _mobiles[i].force.x *= scale;
            _mobiles[i].force.y *= scale;
        }
        //_mobiles[i].force.x *= 0.1f;
        //_mobiles[i].force.y *= 0.1f;
        _mobiles[i].force.x += _g.x * _mobiles[i].density * 0.70f;
        _mobiles[i].force.y += _g.y * _mobiles[i].density * 0.70f;
        //_mobiles[i].force.x += _g.x;  // Indépendant de la densité
        //_mobiles[i].force.y += _g.y;  // Indépendant de la densité
        // printf("Force: (%f, %f)\n", _mobiles[i].force.x, _mobiles[i].force.y);
    }
}
/*fin fonction de ouf*/

// Ajouter cette fonction après init()

bool verify_initialization() {
    bool everything_ok = true;
    
    // Vérifier les shaders
    if (!_pId_phy || !_pId_phy_particules || !_pId_water) {
        printf("ERROR: One or more shaders failed to initialize!\n");
        everything_ok = false;
    }
    
    // Vérifier les buffers des particules
    if (!particle_vao || !particle_vbo) {
        printf("ERROR: Particle buffers not initialized correctly!\n");
        everything_ok = false;
    }
    
    // Vérifier les buffers de l'eau
    if (!_water_vao || !_water_vbo) {
        printf("ERROR: Water buffers not initialized correctly!\n");
        everything_ok = false;
    }
    
    // Vérifier l'allocation des mobiles
    if (!_mobiles || _nb_mobiles <= 0) {
        printf("ERROR: Mobile particles not initialized correctly!\n");
        everything_ok = false;
    }
    
    // Vérifier la grille spatiale
    if (!_grid_phy || !_cell_indices || !_particle_indices) {
        printf("ERROR: Spatial grid structures not initialized correctly!\n");
        everything_ok = false;
    }
    
    // Vérifier les rectangles et poissons
    if (!_rects || _nb_rects <= 0) {
        printf("ERROR: Rectangles not initialized correctly!\n");
        everything_ok = false;
    }
    
    if (!_ovals || _nb_poisson <= 0) {
        printf("ERROR: Fish not initialized correctly!\n");
        everything_ok = false;
    }
    
    return everything_ok;
}

/* initialise des paramètres GL et GL4D */
void init(void)
{
    // Initialiser les structures de la grille spatiale en premier
    _grid_phy = (spatial_cell_t *)malloc(HASH_SIZE * sizeof(spatial_cell_t));
    assert(_grid_phy);
    
    // Comme _nb_mobiles n'est pas encore défini, allouez une taille fixe pour l'instant
    _cell_indices = (int *)malloc(2000 * sizeof(int)); // Taille initiale pour 2000 particules
    assert(_cell_indices);
    
    _particle_indices = (int *)malloc(2000 * sizeof(int));
    assert(_particle_indices);
    
    // Initialiser toutes les cellules
    for (int i = 0; i < HASH_SIZE; i++) {
        _grid_phy[i].start_index = -1;
        _grid_phy[i].count = 0;
    }
    
    _quad_phy = gl4dgGenQuadf();
    /* set la couleur d'effacement OpenGL */
    TIME_SCALE = 10.0f;
    vitesse = 10.0f;
    _g.y = -9.81f;
    _quad_phy = gl4dgGenQuadf();


const char * imfs = "<imfs>water_phy.fs</imfs>\n"
#ifdef __GLES4D__
    "#version 300 es\n"
    "precision mediump float;\n"
#else
    "#version 330 core\n"
#endif
    "uniform float time;\n"
    "uniform vec2 resolution;\n"
    "uniform int dore;\n" // Nouvel uniform pour contrôler l'apparence dorée
    "out vec4 fragColor;\n"
    "\n"
    "// Fonction de bruit simple 2D\n"
    "float hash(vec2 p) {\n"
    "    p = fract(p * vec2(123.34, 345.45));\n"
    "    p += dot(p, p + 34.345);\n"
    "    return fract(p.x * p.y);\n"
    "}\n"
    "\n"
    "// Fonction de bruit Perlin-like simplifiée\n"
    "float noise(vec2 p) {\n"
    "    vec2 i = floor(p);\n"
    "    vec2 f = fract(p);\n"
    "    \n"
    "    float a = hash(i);\n"
    "    float b = hash(i + vec2(1.0, 0.0));\n"
    "    float c = hash(i + vec2(0.0, 1.0));\n"
    "    float d = hash(i + vec2(1.0, 1.0));\n"
    "    \n"
    "    vec2 u = f * f * (3.0 - 2.0 * f); // Fonction de lissage\n"
    "    \n"
    "    return mix(mix(a, b, u.x), \n"
    "               mix(c, d, u.x), u.y);\n"
    "}\n"
    "\n"
    "// Fonction de bruit fractale (FBM - Fractal Brownian Motion)\n"
    "float fbm(vec2 p) {\n"
    "    float value = 0.0;\n"
    "    float amplitude = 0.5;\n"
    "    float frequency = 3.0;\n"
    "    \n"
    "    // Ajoutez plusieurs octaves de bruit\n"
    "    for (int i = 0; i < 5; i++) {\n"
    "        value += amplitude * noise(p * frequency);\n"
    "        frequency *= 2.0;\n"
    "        amplitude *= 0.5;\n"
    "    }\n"
    "    \n"
    "    return value;\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec2 uv = gl_FragCoord.xy / resolution.xy;\n"
    "    \n"
    "    // Bruit de base pour la forme des vagues\n"
    "    float baseNoise = fbm(vec2(uv.x * 4.0 + time * 0.2, uv.y * 4.0));\n"
    "    \n"
    "    // Bruit pour la déformation temporelle\n"
    "    float timeNoise = fbm(vec2(uv.x * 2.0 - time * 0.1, uv.y * 2.0 + time * 0.15));\n"
    "    \n"
    "    // Combiner les bruits pour l'animation des vagues\n"
    "    float wavePattern = baseNoise * 0.6 + timeNoise * 0.4;\n"
    "    wavePattern = sin(wavePattern * 6.28 + time) * 0.5 + 0.5;\n"
    "    \n"
    "    // Couleur de base (bleue ou dorée selon la valeur de l'uniform dore)\n"
    "    vec4 waterColor;\n"
    "    if (dore == 1) {\n"
    "        // Couleur de base dorée si dore == 1\n"
    "        waterColor = vec4(1.0, 0.843, 0.0, 0.5);\n"
    "        \n"
    "        // Variations de couleur dorée basées sur le motif de vagues\n"
    "        waterColor.r += wavePattern * 0.2;\n"
    "        waterColor.g += wavePattern * 0.1;\n"
    "        \n"
    "        // Création d'une brillance dorée\n"
    "        float goldHighlight = pow(fbm(vec2(uv.x * 12.0 + time * 0.4, uv.y * 12.0 - time * 0.3)), 4.0) * 0.7;\n"
    "        goldHighlight += pow(fbm(vec2(uv.y * 18.0 - time * 0.5, uv.x * 9.0 + time * 0.6)), 3.0) * 0.4;\n"
    "        \n"
    "        // Ajouter des détails plus fins seulement en surface\n"
    "        goldHighlight *= smoothstep(0.3, 0.7, wavePattern);\n"
    "        \n"
    "        // Ajouter des reflets dorés plus intenses\n"
    "        waterColor.rg += vec2(goldHighlight * 0.5);\n"
    "        waterColor.b += goldHighlight * 0.3; // Un peu de bleu pour des reflets plus réalistes\n"
    "    } else {\n"
    "        // Couleur bleue d'origine si dore == 0\n"
    "        waterColor = vec4(0.0, 0.3, 0.7, 0.4);\n"
    "        waterColor.b += wavePattern * 0.3;\n"
    "        waterColor.g += wavePattern * 0.1;\n"
    "        \n"
    "        // Création d'une brillance de surface aléatoire\n"
    "        float highlight = pow(fbm(vec2(uv.x * 10.0 + time * 0.3, uv.y * 10.0 - time * 0.2)), 8.0) * 0.5;\n"
    "        highlight += pow(fbm(vec2(uv.y * 15.0 - time * 0.4, uv.x * 7.0 + time * 0.5)), 4.0) * 0.3;\n"
    "        \n"
    "        // Ajouter des détails plus fins seulement en surface\n"
    "        highlight *= smoothstep(0.4, 0.6, wavePattern);\n"
    "        \n"
    "        // Ajouter des reflets aléatoires\n"
    "        waterColor.rgb += vec3(highlight);\n"
    "    }\n"
    "    \n"
    "    fragColor = waterColor;\n"
    "}\n";
const char * imvs = "<imvs>water_phy.vs</imvs>\n"
#ifdef __GLES4D__
    "#version 300 es\n"
#else
    "#version 330 core\n"
#endif
    "in vec2 position;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "}\n";

// Créer le programme de shader pour l'eau
_pId_water = gl4duCreateProgram(imvs, imfs, NULL);
imfs = "<imfs>calculs.fs</imfs>\n"
#ifdef __GLES4D__
    "#version 300 es\n"
    "precision mediump float;\n"
#else
    "#version 330 core\n"
#endif
"out vec4 fragColor;\n"
"in vec2 fcoord;\n"
"uniform vec4 rectangles[10],rect_color;\n"
"uniform int nb_rects;\n"
"uniform float rect_angles[10];\n"
"uniform vec4 poisson[8],poisson_color;\n"
"uniform int nb_poisson;\n"
"uniform float poisson_angles;\n"
"void main()\n"
"{\n"
"  for(int i=0;i<nb_rects;i++)\n"
"    {\n"
"      vec4 rect=rectangles[i];\n"
"      float angle=rect_angles[i];\n"
"      vec2 localCoord=fcoord-rect.xy;\n"
"      float cosAngle=cos(angle);\n"
"      angle=sin(angle);\n"
"      localCoord=vec2(cosAngle*localCoord.x+angle*localCoord.y,-angle*localCoord.x+cosAngle*localCoord.y);\n"
"      if(localCoord.x>=0.&&localCoord.x<=rect.z&&localCoord.y>=0.&&localCoord.y<=rect.w)\n"
"        {\n"
"          fragColor=rect_color;\n"
"          return;\n"
"        }\n"
"    }\n"
"  for(int i=0;i<nb_poisson;i++)\n"
"    {\n"
"      vec4 fish=poisson[i];\n"
"      float angle=poisson_angles;\n"
"      vec2 localCoord=fcoord-fish.xy;\n"
"      float cosAngle=cos(angle);\n"
"      angle=sin(angle);\n"
"      localCoord=vec2(cosAngle*localCoord.x+angle*localCoord.y,-angle*localCoord.x+cosAngle*localCoord.y);\n"
"      if(localCoord.x*localCoord.x/(fish.z*fish.z/4.)+localCoord.y*localCoord.y/(fish.w*fish.w/4.)<=1.)\n"
"        {\n"
"          fragColor=poisson_color;\n"
"          if(length(localCoord-vec2(fish.z*.2,-fish.z*.35))<fish.w*.1)\n"
"            fragColor=vec4(0,0,0,1);\n"
"          vec2 finBase1=vec2(-fish.z*.1,0),v0=vec2(fish.z*.2,0)-finBase1,v1=vec2(fish.z*.05,fish.w*.5)-finBase1;\n"
"          finBase1=localCoord-finBase1;\n"
"          float dot00=dot(v0,v0),dot01=dot(v0,v1),dot02=dot(v0,finBase1),dot11=dot(v1,v1),dot12=dot(v1,finBase1),invDenom=1./(dot00*dot11-dot01*dot01);\n"
"          dot11=(dot11*dot02-dot01*dot12)*invDenom;\n"
"          dot00=(dot00*dot12-dot01*dot02)*invDenom;\n"
"          if(dot11>=0.&&dot00>=0.&&dot11+dot00<=1.)\n"
"            fragColor=poisson_color*.6;\n"
"          return;\n"
"        }\n"
"    }\n"
"  fragColor=vec4(0);\n"
"}\n";
imvs = "<imvs>identity.vs</imvs>\n"
#ifdef __GLES4D__
    "#version 300 es\n"
#else
    "#version 330 core\n"
#endif
    "layout(location=0)in vec3 pos;\n"
    "layout(location=1)in vec3 normal;\n"
    "layout(location=2)in vec2 texCoord;\n"
    "out vec2 fcoord;\n"
    "void main()\n"
    "{\n"
    "  gl_Position=vec4(pos,1);\n"
    "  fcoord=texCoord*2.-1.;\n"
    "}\n";
    /* créer un programme GPU pour OpenGL (en GL4D) */
    _pId_phy = gl4duCreateProgram(imvs, imfs, NULL);
    
    /* créer un programme GPU pour OpenGL (en GL4D) */
    _pId_phy_particules = gl4duCreateProgram("<vs>shaders/parti.vs", "<fs>shaders/parti.fs", NULL);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
// Réinitialiser le temps d'activation des particules
    double current_time = _scene_start_time;
    
    // Réinitialiser les particules et les rendre immédiatement actives
    if (_mobiles) {
        for (int i = 0; i < _nb_mobiles; i++) {
            _mobiles[i].is_active = false; // Forcer l'activation pas immédiate
            _mobiles[i].activation_time = current_time;
            
            // Repositionner à la zone de spawn
            float min_x = 0.5f;
            float max_x = 0.9f;
            float min_y = 0.5f;
            float max_y = 0.9f;
            _mobiles[i].p.x = min_x + (max_x - min_x) * gl4dmSURand();
            _mobiles[i].p.y = min_y + (max_y - min_y) * gl4dmSURand();
            
            // Réinitialiser les propriétés physiques
            _mobiles[i].v.x = 0.0f;
            _mobiles[i].v.y = 0.0f;
            _mobiles[i].density = REST_DENSITY;
            _mobiles[i].pressure = 0.0f;
            _mobiles[i].force.x = 0.0f;
            _mobiles[i].force.y = 0.0f;
        }
    } else {
        // Initialiser les particules normalement si elles n'existent pas encore
        mobile_init(2000);
    }
    //mobile_init(2000);
    // les rectangles
    rect_init_list(7); // la liste de rectangles
    // 0.99f droite -0.99f gauche
    rect_add(0.90f, 0.4f, 0.0f, 0.4f, 0.2f, 0.0f, 20* M_PI / 180.0f);
    //rect_add(0.8f, -0.55f, 0.0f, 0.2f, 0.2f, 0.0f, 60* M_PI / 180.0f);
    rect_add(0.2f, -0.7f, 0.0f, 01.0f, 0.3f, 0.0f, 10* M_PI / 180.0f);
    rect_add(-0.2f, -0.1f-0.65f, 0.0f, 0.4f, 0.3f, 0.0f, 0.1f);
    rect_add(-0.5f, -0.1f-0.7f, 0.0f, 0.4f, 0.3f, 0.0f, 0.1f);
    rect_add(-0.8f, -0.1f-0.75f, 0.0f, 0.4f, 0.3f, 0.0f, 0.1f);
    rect_add(-1.1f, -0.1f-0.8f, 0.0f, 0.4f, 0.3f, 0.0f, 0.1f);
    rect_add(-1.4f, -0.1f-0.85f, 0.0f, 0.4f, 0.3f, 0.0f, 0.1f);
    // rect_add(0.0f, -0.90f, 0.0f, 0.9f, 0.5f, 0.0f, 0.3f);
    oval_init_list(1);                                                                     // la liste de poissons
    oval_add(poission_position[0], poission_position[1], 0.0f, 0.10f, 0.10f, 0.0f, -0.0f); // Poisson 1

    glGenVertexArrays(1, &particle_vao);
    glBindVertexArray(particle_vao);

    glGenBuffers(1, &particle_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, particle_vbo);

    // initiation avec une taille fixe
    glBufferData(GL_ARRAY_BUFFER, 2000 * sizeof(float) * 7, NULL, GL_DYNAMIC_DRAW);

    // configuration des attributs (position, rayon, couleur)
    glEnableVertexAttribArray(0); // position (vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1); // rayon (float)
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(2); // couleur (vec4)
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnable(GL_PROGRAM_POINT_SIZE);

    glBindVertexArray(0); // clean


GLfloat water_vertices[] = {
    // 1. Triangle rectangle plus grand touchant la bordure gauche de l'écran et élevé de 10%
    -1.0f, -0.6f,   // Point en bas à gauche (élevé de 10% de la hauteur de l'écran de -1.0f à -0.8f)
     1.0f, -0.6f,   // Point en bas à droite (également élevé de 10%)
     1.0f, -0.2f,   // Point en haut à droite (également élevé de 10%)
        
    // Rectangle qui couvre le bas de l'écran (100% largeur, 20% hauteur)
    -1.0f, -1.0f,   // Bas gauche
     1.0f, -1.0f,   // Bas droite
     1.0f, -0.6f,  // Haut droite (augmenté de -0.8 à -0.65)
        
    -1.0f, -1.0f,   // Bas gauche  
     1.0f, -0.6f,  // Haut droite (augmenté de -0.8 à -0.65)
    -1.0f, -0.6f,  // Haut gauche (augmenté de -0.8 à -0.65)
    // 2. Bande verticale fine à droite (rectangle inchangé)
    0.9f, -0.6f,    // Bas gauche
    1.0f, -0.6f,    // Bas droite 
    1.0f,  0.4f,    // Haut droite
    
    0.9f, -0.6f,    // Bas gauche
    1.0f,  0.4f,    // Haut droite
    0.9f,  0.4f,    // Haut gauche
    //bande horizontale en haut à droite (plus courte et plus basse)
    0.85f, 0.4f,  // Bas gauche (réduit horizontalement: 0.85 au lieu de 0.8)
    1.0f, 0.4f,  // Bas droite
    1.0f, 0.6f,  // Haut droite
    
    0.85f, 0.4f,  // Bas gauche
    1.0f, 0.6f,  // Haut droite
    0.85f, 0.6f   // Haut gauche
};
    glGenVertexArrays(1, &_water_vao);
    glBindVertexArray(_water_vao);

    glGenBuffers(1, &_water_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _water_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(water_vertices), water_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    printf("Initializing shader programs...\n");
    if (_pId_phy) 
        printf(" - Physics shader: OK (id: %u)\n", _pId_phy);
    else
        printf(" - Physics shader: FAILED\n");
        
    if (_pId_phy_particules) 
        printf(" - Particle shader: OK (id: %u)\n", _pId_phy_particules);
    else
        printf(" - Particle shader: FAILED\n");
        
    if (_pId_water) 
        printf(" - Water shader: OK (id: %u)\n", _pId_water);
    else
        printf(" - Water shader: FAILED\n");
        
    printf("Initializing OpenGL resources...\n");
    if (_quad_phy)
        printf(" - Quad: OK (id: %u)\n", _quad_phy);
    else
        printf(" - Quad: FAILED\n");
        
    if (particle_vbo && particle_vao)
        printf(" - Particle buffers: OK (VAO: %u, VBO: %u)\n", particle_vao, particle_vbo);
    else
        printf(" - Particle buffers: %s%s\n", 
               particle_vao ? "" : "VAO FAILED ", 
               particle_vbo ? "" : "VBO FAILED");
               
    printf("Initializing spatial grid...\n");
    if (_grid_phy && _cell_indices && _particle_indices)
        printf(" - Spatial grid structures: OK\n");
    else
        printf(" - Spatial grid structures: %s%s%s\n",
               _grid_phy ? "" : "GRID FAILED ",
               _cell_indices ? "" : "CELL_INDICES FAILED ",
               _particle_indices ? "" : "PARTICLE_INDICES FAILED");
     
    if (_water_vao && _water_vbo)
        printf(" - Water buffers: OK (VAO: %u, VBO: %u)\n", _water_vao, _water_vbo);
    else
        printf(" - Water buffers: %s%s\n",
               _water_vao ? "" : "VAO FAILED ",
               _water_vbo ? "" : "VBO FAILED");
}

void draw(void){
     // Vérifier que les éléments critiques sont initialisés avant de dessiner
    if (!_pId_water || !_water_vao || !particle_vao || !_pId_phy_particules) {
        printf("Critical rendering components missing, skipping frame!\n");
        return;
    }
    static double t0 = 0.0;
    double t = gl4dGetElapsedTime() / 1000.0, dt = (t - t0);
    t0 = t;
    double scene_time = t - _scene_start_time; // Temps écoulé depuis le début de la scène
    /* effacer le buffer de couleur (image) et le buffer de profondeur d'OpenGL */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    mobile_simu();

    /* utiliser le programme GPU "_pId_phy" */
    glUseProgram(_pId_phy);
    
    // Dessiner les rectangles
    GLfloat *rect_data = malloc(4 * _nb_rects * sizeof *rect_data);
    assert(rect_data);

    for (int i = 0; i < _nb_rects; ++i) {
        rect_data[4 * i + 0] = _rects[i].x;
        rect_data[4 * i + 1] = _rects[i].y;
        rect_data[4 * i + 2] = _rects[i].w;
        rect_data[4 * i + 3] = _rects[i].h;
    }

    float *rect_angles = malloc(_nb_rects * sizeof(float));
    assert(rect_angles);
    for (int i = 0; i < _nb_rects; ++i) {
        rect_angles[i] = _rects[i].angle_x;
    }

    glUniform1fv(glGetUniformLocation(_pId_phy, "rect_angles"), _nb_rects, rect_angles);
    glUniform4fv(glGetUniformLocation(_pId_phy, "rectangles"), _nb_rects, rect_data);
    glUniform1i(glGetUniformLocation(_pId_phy, "nb_rects"), _nb_rects);
    glUniform4f(glGetUniformLocation(_pId_phy, "rect_color"), 0.6f, 0.6f, 0.6f, 1.0f);

    // Dessiner les poissons (ovals)
    GLfloat *oval_data = malloc(4 * _nb_poisson * sizeof *oval_data);
    assert(oval_data);
    float *oval_angles = malloc(_nb_poisson * sizeof(float));
    assert(oval_angles);

    for (int i = 0; i < _nb_poisson; ++i) {
        oval_data[4 * i + 0] = _ovals[i].x;
        oval_data[4 * i + 1] = _ovals[i].y;
        oval_data[4 * i + 2] = _ovals[i].w;
        oval_data[4 * i + 3] = _ovals[i].h;
        oval_angles[i] = _ovals[i].angle_x;
    }
    if (!(t0 < 83.0f)){
    //update_fish_positions_stagne(0.30f);
    
    update_fish_positions(0.05f);
    } else {
        //update_fish_positions_stagne(0.20f);
    }
    
    // Update the oval position with the current fish position
    if (_nb_poisson > 0) {
        _ovals[0].x = poission_position[0];
        _ovals[0].y = poission_position[1];
        //printf("Fish position updated: x=%f, y=%f, angle=%f\n", 
        //       poission_position[0], poission_position[1], _ovals[0].angle_x);
    }
    glUniform4fv(glGetUniformLocation(_pId_phy, "poisson"), _nb_poisson, oval_data);
    glUniform1fv(glGetUniformLocation(_pId_phy, "poisson_angles"), _nb_poisson, oval_angles);
    glUniform1i(glGetUniformLocation(_pId_phy, "nb_poisson"), _nb_poisson);
    if (!(t0 < 83.0f)){
    glUniform4f(glGetUniformLocation(_pId_phy, "poisson_color"), 1.0f, 1.0f, 1.0f, 1.0f);
    } else {
        glUniform4f(glGetUniformLocation(_pId_phy, "poisson_color"), 0.0f, 0.0f, 0.0f, 0.0f);
    }    

    // Dessiner les objets
    gl4dgDraw(_quad_phy);

    // Gestion des collisions
    for (int i = 0; i < _nb_mobiles; ++i) {
        for (int j = 0; j < _nb_rects; ++j) {
            collide_with_rotated_rect(&_mobiles[i], &_rects[j], e);
        }
        for (int j = 0; j < _nb_poisson; ++j) {
            collide_with_rotated_oval(&_mobiles[i], &_ovals[j], e);
        }
    }

    // Libérer la mémoire
    free(rect_data);
    free(rect_angles);
    free(oval_data);
    free(oval_angles);
    
    int water_dore = 0; // 0: eau normale, 1: eau dorée
    glUseProgram(_pId_water);
    if (!(t0 < 83.0f)){
        water_dore = 1; // Activer l'effet doré après 83 secondes
    } else {
        water_dore = 0; // Eau normale avant 83 secondes
    }
// Passer le temps pour animer l'eau
glUniform1f(glGetUniformLocation(_pId_water, "time"), (float)t0);
// Passer la résolution de l'écran
glUniform2f(glGetUniformLocation(_pId_water, "resolution"), (float)1920, (float)1080);
glUniform1i(glGetUniformLocation(_pId_water, "dore"), water_dore);


// Activer le blending pour la transparence
//glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

// Dessiner le rectangle d'eau
glBindVertexArray(_water_vao);
glDisable(GL_DEPTH_TEST);  // Désactiver le depth test pour l'eau

glDrawArrays(GL_TRIANGLES, 0, 24);
glDrawArrays(GL_TRIANGLES, 0, 24);
//printf("Drawing water...\n");
glBindVertexArray(0);

// Désactiver le blending
//glDisable(GL_BLEND);
    glUseProgram(_pId_phy_particules); // shader pour les particules
    mobile_draw();
    
    if (!(t0 < 83.0f))
    {
        // Sélectionner une balle aléatoire comme point de départ
        int randomStartIndex = rand() % (_nb_mobiles - 5 > 0 ? _nb_mobiles - 5 : 1);

        // Changer la couleur de la balle sélectionnée et des 5 suivantes
        for (int offset = 0; offset < 16; offset++)
        {
            int currentIndex = randomStartIndex + offset;

            // Vérifier que l'index ne dépasse pas le nombre de balles
            if (currentIndex < _nb_mobiles)
            {
                // Changer uniquement si ce n'est pas déjà une balle dorée
                if (_mobiles[currentIndex].color[0] < 0.1f)
                {
                    _mobiles[currentIndex].color[0] = 1.0f;
                    _mobiles[currentIndex].color[1] = 0.843f;
                    _mobiles[currentIndex].color[2] = 0.0f;
                }
            }
        }
    }
    glBindVertexArray(particle_vao);
    glDrawArrays(GL_POINTS, 0, _nb_mobiles);
    glBindVertexArray(0);

    

    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

vec3d_t spawn_point = { 1.0f, 0.8f, 0.0f };

void mobile_init(int n)
{
    assert(_mobiles == NULL);
    _nb_mobiles = n;
    _mobiles = malloc(_nb_mobiles * sizeof *_mobiles);
    assert(_mobiles);
    // Créer une forme en ligne (une seule rangée) en haut à droite
    float min_x = 0.5f; // Limite gauche de la zone de spawn
    float max_x = 0.9f; // Limite droite de la zone de spawn
    float min_y = 0.5f; // Limite basse de la zone de spawn
    float max_y = 0.9f; // Limite haute de la zone de spawn
    double start_time = gl4dGetElapsedTime() / 1000.0 - _scene_start_time; // Temps écoulé depuis le début de la scène
    
    for (int i = 0; i < n; i++){
        // Make particles appear randomly across the top portion of the screen
        _mobiles[i].activation_time = start_time + i * 0.01; // progressive delay
        _mobiles[i].is_active = false;

        // Randomize position across the entire width of the screen, but only in the top area
        _mobiles[i].p.x = -0.9f + 1.8f * gl4dmSURand(); // Random x from -0.9 to 0.9
        _mobiles[i].p.y = 0.7f + 0.3f * gl4dmSURand();  // Random y from 0.7 to 1.0
        _mobiles[i].activation_time = start_time + i * 0.01; // délai progressif
        _mobiles[i].is_active = false;

        // Position aléatoire dans la zone définie
        _mobiles[i].p.x = min_x + (max_x - min_x) * gl4dmSURand();
        _mobiles[i].p.y = min_y + (max_y - min_y) * gl4dmSURand();
        _mobiles[i].p.z = 0.0f;

        // Vitesse initiale nulle
        _mobiles[i].v.x = 0.0f;
        _mobiles[i].v.y = 0.0f;
        _mobiles[i].v.z = 0.0f;

        // Rayon légèrement variable pour plus de naturel
        _mobiles[i].r = 0.02f; // + 0.01f * gl4dmSURand(); //TODOOO

        // Initialisation des propriétés SPH
        _mobiles[i].density = REST_DENSITY;
        _mobiles[i].pressure = 0.0f;
        _mobiles[i].force.x = 0.0f;
        _mobiles[i].force.y = 0.0f;
        _mobiles[i].force.z = 0.0f;

        // Couleur bleue pour les particules d'eau
        _mobiles[i].color[0] = 0.0f;
        _mobiles[i].color[1] = 0.4f;
        _mobiles[i].color[2] = 0.9f;
        _mobiles[i].color[3] = 1.0f;
    }
}

void mobile_simu(void){
    static double t0 = 0.0;
    double t = gl4dGetElapsedTime() / 1000.0;
    double dt = (t - t0) * 30.0;
    t0 = t;

    if (dt > 0.03)
        dt = 0.03; // Limiter le pas de temps à 30 ms


    // Calculer les forces SPH
    compute_sph_forces();

    // Précalculer les valeurs constantes
    float dt_time_scale = dt * TIME_SCALE;
    float dt_vitesse = dt * vitesse;
    float max_speed_squared = 0.0f;
    //if ((t0 < 83.0f))
    //{
    //    // TIME_SCALE = 1.0f*dt;
    //    max_speed_squared = 0.0f;
    //    // printf("t = %f\n", t0);
    //}
    //else
    //{
        TIME_SCALE = 10.10f;
        max_speed_squared = 10.0f;
    //}
    float min_distance_squared = MIN_DISTANCE * MIN_DISTANCE;
    double scene_time = t - _scene_start_time;

    for (int i = 0; i < _nb_mobiles; ++i){
        

    if (! _mobiles[i].is_active && scene_time >= _mobiles[i].activation_time) {
    _mobiles[i].is_active = true;
        // Randomize position across the entire width of the screen, but only in the top area
        _mobiles[i].p.x = -0.9f + 1.8f * gl4dmSURand(); // Random x from -0.9 to 0.9
        _mobiles[i].p.y = 0.7f + 0.3f * gl4dmSURand();  // Random y from 0.7 to 1.0
    //printf("remise au point de depart");
    }
if (! _mobiles[i].is_active){
        continue; // pas encore active
    }
        //si une balle touche le mur de gauche elle revient au spawnpoint
        if (_mobiles[i].p.x <= -1.0f)
        {
            _mobiles[i].p = spawn_point;
        }
        _mobiles[i].force.x *= dt_time_scale;
        _mobiles[i].force.y *= dt_time_scale;

        // Intégration explicite d'Euler
        float accel_x = _mobiles[i].force.x / _mobiles[i].density;
        float accel_y = _mobiles[i].force.y / _mobiles[i].density;

        _mobiles[i].v.x += accel_x * dt_vitesse;
        _mobiles[i].v.y += accel_y * dt_vitesse;

        // Mettre à jour position
        _mobiles[i].p.x += _mobiles[i].v.x * dt;
        _mobiles[i].p.y += _mobiles[i].v.y * dt;

        // Collision avec les murs avec rebond
        if (_mobiles[i].p.x - _mobiles[i].r <= -1.0f)
        {
            _mobiles[i].v.x = -_mobiles[i].v.x * e;
            _mobiles[i].p.x = -1.0f + _mobiles[i].r;
        }
        else if (_mobiles[i].p.x + _mobiles[i].r >= 1.0f)
        {
            _mobiles[i].v.x = -_mobiles[i].v.x * e;
            _mobiles[i].p.x = 1.0f - _mobiles[i].r;
        }

        if (_mobiles[i].p.y - _mobiles[i].r <= -1.0f)
        {
            _mobiles[i].v.y = -_mobiles[i].v.y * e;
            _mobiles[i].p.y = -1.0f + _mobiles[i].r;
        }
        else if (_mobiles[i].p.y + _mobiles[i].r >= 1.0f)
        {
            _mobiles[i].v.y = -_mobiles[i].v.y * e;
            _mobiles[i].p.y = 1.0f - _mobiles[i].r;
        }

        // Mettre à jour la couleur en fonction de la pression (visualisation) faut decommenter
        // float pressure_ratio = (_mobiles[i].pressure / (GAS_CONSTANT * REST_DENSITY)) * 0.01f;
        // pressure_ratio = fmaxf(0.0f, fminf(1.0f, pressure_ratio * 0.1f));
        //_mobiles[i].color[0] = 0.0f;
        //_mobiles[i].color[1] = 0.4f;
        //_mobiles[i].color[2] = 0.8f;

        // Calculer la vitesse actuelle
        float speed_squared = _mobiles[i].v.x * _mobiles[i].v.x + _mobiles[i].v.y * _mobiles[i].v.y;

        // Si la vitesse dépasse le maximum, la réduire
        if (speed_squared > max_speed_squared)
        {
            float scale = 0.50f / sqrtf(speed_squared);
            _mobiles[i].v.x *= scale;
            _mobiles[i].v.y *= scale;
        }
    }

    // Détection et résolution des collisions entre mobiles
    for (int i = 0; i < _nb_mobiles; ++i)
    {
        for (int j = i + 1; j < _nb_mobiles; ++j)
        {
            float dx = _mobiles[j].p.x - _mobiles[i].p.x;
            float dy = _mobiles[j].p.y - _mobiles[i].p.y;
            float dist2 = dx * dx + dy * dy;

            // Somme des rayons
            float sumRadii = _mobiles[i].r + _mobiles[j].r;
            float sumRadiiSquared = sumRadii * sumRadii;

            // Test de collision
            if (dist2 < sumRadiiSquared)
            {
                float dist = sqrtf(dist2);
                if (dist < 0.0001f)
                    dist = 0.0001f; // éviter la division par zéro

                // Vecteur unitaire i -> j
                float nx = dx / dist;
                float ny = dy / dist;

                // Écarter les particules pour corriger le chevauchement
                float overlap = 0.5f * (sumRadii - dist);
                _mobiles[i].p.x -= nx * overlap;
                _mobiles[i].p.y -= ny * overlap;
                _mobiles[j].p.x += nx * overlap;
                _mobiles[j].p.y += ny * overlap;

                // Vitesse relative dans la direction de la collision
                float vx = _mobiles[j].v.x - _mobiles[i].v.x;
                float vy = _mobiles[j].v.y - _mobiles[i].v.y;
                float dot = vx * nx + vy * ny;

                // Si dot > 0, elles s'éloignent déjà ⇒ pas de correction supplémentaire
                if (dot > 0.0f)
                    continue;

                // Coefficient de restitution (rebond)
                float restitution = 0.5f; // Ajustez selon l'effet rebond désiré

                // Impulsion
                float impulse = -(1.0f + restitution) * dot;
                impulse *= 0.5f; // Répartition égale si les masses sont égales

                // Appliquer l’impulsion
                _mobiles[i].v.x -= nx * impulse;
                _mobiles[i].v.y -= ny * impulse;
                _mobiles[j].v.x += nx * impulse;
                _mobiles[j].v.y += ny * impulse;
            }
        }
    }

    rect_collide_all(_mobiles, _nb_mobiles, e);
    if (!(t0 < 50.0f))
    {
        oval_collide_all(_mobiles, _nb_mobiles, e);
    }
}
void mobile_draw(void)
{
    GLfloat *tmp = malloc(sizeof(float) * 7 * _nb_mobiles);
    assert(tmp);

    for (int i = 0; i < _nb_mobiles; ++i)
    {
        if (!_mobiles[i].is_active) continue;
        //        _mobiles[i].is_active = true;  // FORCER l'activation

        tmp[i * 7 + 0] = _mobiles[i].p.x;
        tmp[i * 7 + 1] = _mobiles[i].p.y;
        tmp[i * 7 + 2] = _mobiles[i].r;
        tmp[i * 7 + 3] = _mobiles[i].color[0];
        tmp[i * 7 + 4] = _mobiles[i].color[1];
        tmp[i * 7 + 5] = _mobiles[i].color[2];
        tmp[i * 7 + 6] = _mobiles[i].color[3];
    }
    glBindBuffer(GL_ARRAY_BUFFER, particle_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 7 * _nb_mobiles, tmp);
    free(tmp);
}
// void mobile_draw(void){
//	GLfloat *tmp = malloc(4 * _nb_mobiles * sizeof *tmp);
//	assert(tmp);
//	for (int i = 0; i < _nb_mobiles; ++i){
//		tmp[4 * i + 0] = _mobiles[i].p.x;
//		tmp[4 * i + 1] = _mobiles[i].p.y;
//		tmp[4 * i + 2] = _mobiles[i].r;
//	}
//	glUniform4fv(glGetUniformLocation(_pId_phy, "positions"), _nb_mobiles, tmp);
//	for (int i = 0; i < _nb_mobiles; ++i)
//		for (int j = 0; j < 4; ++j)
//			tmp[4 * i + j] = _mobiles[i].color[j];
//	glUniform4fv(glGetUniformLocation(_pId_phy, "couleurs"), _nb_mobiles, tmp);
//	glUniform1i(glGetUniformLocation(_pId_phy, "nbe"), _nb_mobiles);
//	//TODO met le scatering et blur
//	gl4dgDraw(_quad_phy);
//	free(tmp);
// }
