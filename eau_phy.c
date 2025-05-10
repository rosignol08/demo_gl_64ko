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

#define MAX_NEIGHBOURS 32
#define HASH_SIZE 2000
#define CELL_SIZE 0.1f  //taille des cellules pour la grille spatiale

//pour la gravitée radiale
#define G_CONSTANT 6.67430e-3  // Constante gravitationnelle modifiée pour l'échelle de la simulation
#define MIN_DISTANCE 0.01f     // Distance minimale pour éviter les accélérations infinies

float vitesse = 1.0f;
#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

//macro pour les expressions mathématiques complexes
#define POLY6 (315.0f / (64.0f * M_PI * powf(H, 9)))
#define SPIKY_GRAD (-45.0f / (M_PI * powf(H, 6)))
#define VISC_LAP (45.0f / (M_PI * powf(H, 6)))
#define SURFACE_TENSION 0.0728f

//pour les rectangles 3D
typedef struct {
    float x, y, z;
    float w, h, d;
    float angle_x; // Angle de rotation sur l'axe x
} rect3d_t;

//pour les poissons ovales
typedef struct {
    float x, y, z;
    float w, h, d;
    float angle_x; // Angle de rotation sur l'axe x
} oval3d_t;

//tableau dynamique global
static rect3d_t* _rects = NULL;  
static int _nb_rects   = 0;     
static int _max_rects  = 0;     

//pour les poissons (des formes ovales)
static oval3d_t* _ovals = NULL;
static int _nb_poisson = 0;
static int _max_poisson = 0;

//struct pour la grille spatiale
typedef struct {
    int start_index;
    int count;
} spatial_cell_t;


//variables globales pour l'optimisation
static spatial_cell_t* _grid = NULL;
static int* _cell_indices = NULL;
static int* _particle_indices = NULL;


typedef struct vec3d_t vec3d_t;
typedef struct mobile_t mobile_t;

struct vec3d_t
{
	GLfloat x, y, z;
};

//temps de simulation
static float TIME_SCALE = 0.1f;  //vitesse de simulation


static void init(void);
static void draw(void);

static void mobile_init(int n);
static void mobile_simu(void);
static void mobile_draw(void);

/* on créé une variable pour stocker l'identifiant du programme GPU */
GLuint _pId = 0;
GLuint _pId_particules = 0;

GLuint _quad = 0;

// VBO et VAO pour les particules
GLuint particle_vbo;
GLuint particle_vao;

//les poissons qui nagent
float poission_position[2] = {0.0f, 0.0f};
float poission_position_y = 0.0f;
/* gravité */
//static GLfloat _ig = 9.81f / 2.0f;
static vec3d_t _g = {0.0f, 0.0f, 0.0f}; // Modification ici: définir la gravité vers le bas à -9.81f
static const GLfloat e = 0.5f; //8.0f / 9.0f;

/* simulation d'eau de jsp qui */
// Ajouter ces paramètres SPH
static const float REST_DENSITY = 300.0f;  // Densité au repos du fluide
static const float GAS_CONSTANT = 2000.0f;  // Constante des gaz parfaits
static const float VISCOSITY = 10.0f;      // Viscosité du fluide
static const float MASS = 1.0f;             // Masse d'une particule
static const float H = 0.11f;                // Rayon de lissage (smoothing radius)
static const float H2 = 0.0075f;              // H²
//static const float POLY6 = 315.0f / (64.0f * M_PI * powf(H, 9));
//static const float SPIKY_GRAD = -45.0f / (M_PI * powf(H, 6));
//static const float VISC_LAP = 45.0f / (M_PI * powf(H, 6));
//static const float SURFACE_TENSION = 0.0728f;

// Variables fluides à ajouter à la structure mobile_t
struct mobile_t {
    vec3d_t p, v;
    GLfloat r;
    GLfloat color[4];
    
    // Variables SPH
    float density;
    float pressure;
    vec3d_t force;
    int cell_id;
};


/* tous les mobiles de ma scène */
static mobile_t *_mobiles = NULL;
static int _nb_mobiles = 0;

void eau_scene(int state) {
    switch(state) {
    case GL4DH_INIT:
        init();
        return;
    case GL4DH_FREE:
    {
        if (_mobiles) {
            free(_mobiles);
            _mobiles = NULL;
            _nb_mobiles = 0;
        }
        
        if (_grid) {
            free(_grid);
            _grid = NULL;
        }
        
        if (_cell_indices) {
            free(_cell_indices);
            _cell_indices = NULL;
        }
        
        if (_particle_indices) {
            free(_particle_indices);
            _particle_indices = NULL;
        }
        if (_pId) {
            glDeleteProgram(_pId);
            _pId = 0;
        }
        if (_quad) {
            glDeleteVertexArrays(1, &_quad);
            _quad = 0;
        }
        if (_rects) {
            free(_rects);
            _rects = NULL;
        }
        if (_rects) {
            free(_rects);
            _rects = NULL;
        }
        _nb_rects = 0;
        _max_rects = 0;
        if (_ovals) {
            free(_ovals);
            _ovals = NULL;
        }
        _nb_poisson = 0;
        _max_poisson = 0;
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
float kernel_poly6(float r2) {
    if (r2 > H2) return 0.0f;
    float temp = H2 - r2;
    return POLY6 * temp * temp * temp;
}

float kernel_spiky_gradient(float r) {
    if (r > H) return 0.0f;
    float temp = H - r;
    return SPIKY_GRAD * temp * temp;
}

float kernel_viscosity_laplacian(float r) {
    if (r > H) return 0.0f;
    return VISC_LAP * (H - r);
}
float kernel_viscosity_improved(float r, float h) {
    if (r >= h) return 0.0f;
    float q = r / h;
    return (h / r) * (1.0f - q);
}

//les rectangles
void rect_init_list(int capacity) {
    if (_rects) free(_rects);
    _max_rects = capacity;
    _rects = (rect3d_t*)malloc(_max_rects * sizeof(rect3d_t));
    _nb_rects = 0;
}
// Ajoute un rectangle à la liste
void rect_add(float x, float y, float z, float w, float h, float d, float angle_x) {
    if (_nb_rects >= _max_rects) return; // ou agrandir dynamiquement
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
static void collide_with_rotated_rect(mobile_t* m, const rect3d_t* r, float e) {
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
        rotatedY >= 0.0f && rotatedY <= r->h) {
        
        // Correct the particle's position to push it outside the rectangle
        if (rotatedY < r->h / 2.0f) {
            rotatedY = -m->r; // Push below
        } else {
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
void rect_draw_all(void) {
    GLfloat *rect_data = malloc(4 * _nb_rects * sizeof *rect_data); //4 pour x,y,w,h
    assert(rect_data);
    
    for (int i = 0; i < _nb_rects; ++i) {
        rect_data[4 * i + 0] = _rects[i].x;
        rect_data[4 * i + 1] = _rects[i].y;
        rect_data[4 * i + 2] = _rects[i].w;
        rect_data[4 * i + 3] = _rects[i].h;
    }
    // Set up an attribute for angles if needed
    glUseProgram(_pId);
    //allocation séparée pour les angles
    float *angles = malloc(_nb_rects * sizeof(float));
    assert(angles);
    for (int i = 0; i < _nb_rects; ++i) {
        angles[i] = _rects[i].angle_x;
    }
    glUniform1fv(glGetUniformLocation(_pId, "rect_angles"), _nb_rects, angles);
    glUseProgram(_pId);
    glUniform4fv(glGetUniformLocation(_pId, "rectangles"), _nb_rects, rect_data);
    glUniform1i(glGetUniformLocation(_pId, "nb_rects"), _nb_rects);

    //set la couleur en blanc
    glUniform4f(glGetUniformLocation(_pId, "rect_color"), 0.6f, 0.6f, 0.6f, 1.0f);
    for (int i = 0; i < _nb_mobiles; ++i) {
        for (int j = 0; j < _nb_rects; ++j) {
            collide_with_rotated_rect(&_mobiles[i], &_rects[j], e);
        }
    }
    gl4dgDraw(_quad);
    glUseProgram(0);

    free(rect_data);
    free(angles);
}


// Appeler cette fonction dans mobile_simu après mise à jour des particules
void rect_collide_all(mobile_t* mobiles, int nb, float e) {
    for (int i = 0; i < nb; i++) {
        for (int j = 0; j < _nb_rects; j++) {
            collide_with_rotated_rect(&mobiles[i], &_rects[j], e);
        }
    }
}

void oval_init_list(int capacity) {
    if (_ovals) free(_ovals);
    _max_poisson = capacity;
    _ovals = (oval3d_t*)malloc(_max_poisson * sizeof(oval3d_t));
    _nb_poisson = 0;
}

// Ajoute un poisson à la liste
// x, y, z : position du poisson, w, h : largeur et hauteur de l'oval 
void oval_add(float x, float y, float z, float w, float h, float d, float angle_x) {
    if (_nb_poisson >= _max_poisson) return; // ou agrandir dynamiquement
    _ovals[_nb_poisson].x = x;
    _ovals[_nb_poisson].y = y;
    _ovals[_nb_poisson].z = z;
    _ovals[_nb_poisson].w = w;
    _ovals[_nb_poisson].h = h;
    _ovals[_nb_poisson].d = d;
    _ovals[_nb_poisson].angle_x = angle_x; // Ajout de l'angle de rotation sur l'axe x
    _nb_poisson++;
}

static void collide_with_rotated_oval(mobile_t* m, const oval3d_t* o, float e) {
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
        rotatedY >= 0.0f && rotatedY <= o->h) {
        
        // Correct the particle's position to push it outside the oval
        if (rotatedY < o->h / 2.0f) {
            rotatedY = -m->r; // Push below
        } else {
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

//fonction pour mettre à jour la position des poissons
void update_fish_positions(void) {
    static float time = 0.0f;
    time += 0.1f;
    // Mouvement en diagonale avec variation en Y
    float base_x = -0.8f + time * 0.05f;  // Avance en X de gauche à droite
    float base_y = -0.8f + time * 0.3f;  // Avance en Y en diagonale
    poission_position_y += 0.00085f + (0.001f * sinf(time)); // Oscillation en Y
    
    // Ajoute une légère oscillation en Y avec cosinus
    poission_position[0] = base_x;
    poission_position[1] = -0.42f + poission_position_y;//base_y * (cosf(time * 3.0f) * 0.05f);
    
    // Si le poisson sort de l'écran, le faire revenir de l'autre côté
//    if (poission_position[0] > 1.0f) {
//        time = 0.0f;  // Réinitialiser le temps
//    }
    
    // Maj de la position du poisson dans le tableau _ovals
    if (_nb_poisson > 0) {
        _ovals[0].x = poission_position[0];
        _ovals[0].y = poission_position[1];
        
        // Faire pointer le poisson dans la direction du mouvement
        // L'angle est calculé en fonction de la direction du mouvement + oscillation
        _ovals[0].angle_x = atan2f(0.03f + 0.1f * -sinf(time * 3.0f), 0.05f);
    }
}

void oval_draw_all(void) {
    GLfloat *oval_data = malloc(4 * _nb_poisson * sizeof *oval_data); //4 pour x,y,w,h
    assert(oval_data);
    
    for (int i = 0; i < _nb_poisson; ++i) {
        oval_data[4 * i + 0] = _ovals[i].x;
        oval_data[4 * i + 1] = _ovals[i].y;
        oval_data[4 * i + 2] = _ovals[i].w;
        oval_data[4 * i + 3] = _ovals[i].h;
    }
    // Set up an attribute for angles if needed
    glUseProgram(_pId);
    //allocation séparée pour les angles
    float *angles = malloc(_nb_poisson * sizeof(float));
    assert(angles);
    for (int i = 0; i < _nb_poisson; ++i) {
        angles[i] = _ovals[i].angle_x;
    }
    glUniform1fv(glGetUniformLocation(_pId, "poisson_angles"), _nb_poisson, angles);
    glUseProgram(_pId);
    glUniform4fv(glGetUniformLocation(_pId, "poisson"), _nb_poisson, oval_data);
    glUniform1i(glGetUniformLocation(_pId, "nb_poisson"), _nb_poisson);

    //set la couleur en blanc
    glUniform4f(glGetUniformLocation(_pId, "poisson_color"), 1.0f, 1.0f, 1.0f, 1.0f);
    
    for (int i = 0; i < _nb_mobiles; ++i) {
        for (int j = 0; j < _nb_poisson; ++j) {
            collide_with_rotated_oval(&_mobiles[i], &_ovals[j], e);
        }
    }
    
    gl4dgDraw(_quad);
    glUseProgram(0);

    free(oval_data);
    free(angles);
}

void oval_collide_all(mobile_t* mobiles, int nb, float e) {
    for (int i = 0; i < nb; i++) {
        for (int j = 0; j < _nb_poisson; j++) {
            collide_with_rotated_oval(&mobiles[i], &_ovals[j], e);
        }
    }
}

// Fonction pour construire la grille spatiale
void build_spatial_grid() {
    // Initialiser la grille
    if (!_grid) {
        _grid = (spatial_cell_t*)malloc(HASH_SIZE * sizeof(spatial_cell_t));
        _cell_indices = (int*)malloc(_nb_mobiles * sizeof(int));
        _particle_indices = (int*)malloc(_nb_mobiles * sizeof(int));
    }
    
    // Réinitialiser les cellules
    for (int i = 0; i < HASH_SIZE; i++) {
        _grid[i].start_index = -1;
        _grid[i].count = 0;
    }
    
    // Attribuer les particules aux cellules
    for (int i = 0; i < _nb_mobiles; i++) {
        int cellX = (int)(((_mobiles[i].p.x + 1.0f) / 2.0f) / CELL_SIZE);
        int cellY = (int)(((_mobiles[i].p.y + 1.0f) / 2.0f) / CELL_SIZE);
        
        int cell_id = (cellY * 32 + cellX) % HASH_SIZE; // Hachage simple
        _mobiles[i].cell_id = cell_id;
        
        // Compter combien de particules par cellule
        _grid[cell_id].count++;
    }
    
    // Calculer les indices de départ
    int start = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        _grid[i].start_index = start;
        start += _grid[i].count;
        _grid[i].count = 0; // Réinitialiser pour la prochaine étape
    }
    
    // Remplir les indices
    for (int i = 0; i < _nb_mobiles; i++) {
        int cell_id = _mobiles[i].cell_id;
        int index = _grid[cell_id].start_index + _grid[cell_id].count;
        _cell_indices[index] = i;
        _grid[cell_id].count++;
    }
}

// Fonction pour calculer les forces gravitationnelles entre particules
void compute_gravity_forces() {
    //reset les forces
    for (int i = 0; i < _nb_mobiles; i++) {
        _mobiles[i].force.x = 0.0f;
        _mobiles[i].force.y = 0.0f;
        _mobiles[i].force.z = 0.0f;
    }
    
    //calcule des forces gravitationnelles entre chaque paire de particules
    for (int i = 0; i < _nb_mobiles; i++) {
        for (int j = i + 1; j < _nb_mobiles; j++) {
            float dx = _mobiles[j].p.x - _mobiles[i].p.x;
            float dy = _mobiles[j].p.y - _mobiles[i].p.y;
            //float dz = _mobiles[j].p.z - _mobiles[i].p.z;
            
            float r2 = dx*dx + dy*dy;// + dz*dz;
            float r = sqrtf(r2);
            
            // Éviter division par zéro et forces trop grandes entre particules proches
            if (r < MIN_DISTANCE) r = MIN_DISTANCE;
            
            //force gravitationnelle: F = G * m1 * m2 / r^2
            //toutes les particules ont la même masse (MASS)
            float force = G_CONSTANT * MASS * MASS / r2;
            
            //direction de la force (vecteur unitaire)
            float fx = force * dx / r;
            float fy = force * dy / r;
            //float fz = force * dz / r;
            
            //la force aux deux particules (action-réaction)
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
void compute_sph_forces() {
    // Construire la grille spatiale
    build_spatial_grid();
    // Calculer les densités et pressions
    for (int i = 0; i < _nb_mobiles; i++) {
        _mobiles[i].density = 0.0f;
        
        // Auto-contribution à la densité
        _mobiles[i].density += MASS * kernel_poly6(0.0f);
        
        // Contribution des voisins
        //int cell_id = _mobiles[i].cell_id;
        
        // Parcourir les 9 cellules voisines (en 2D)
        for (int offsetY = -1; offsetY <= 1; offsetY++) {
            for (int offsetX = -1; offsetX <= 1; offsetX++) {
                int cellX = (int)(((_mobiles[i].p.x + 1.0f) / 2.0f) / CELL_SIZE) + offsetX;
                int cellY = (int)(((_mobiles[i].p.y + 1.0f) / 2.0f) / CELL_SIZE) + offsetY;
                
                if (cellX < 0 || cellY < 0 || cellX >= 32 || cellY >= 32) continue;
                
                int neighbor_cell_id = (cellY * 32 + cellX) % HASH_SIZE;
                
                if (_grid[neighbor_cell_id].start_index == -1) continue;
                
                // Parcourir les particules de cette cellule
                for (int k = 0; k < _grid[neighbor_cell_id].count; k++) {
                    int j = _cell_indices[_grid[neighbor_cell_id].start_index + k];
                    if (i == j) continue;
                    
                    float dx = _mobiles[j].p.x - _mobiles[i].p.x;
                    float dy = _mobiles[j].p.y - _mobiles[i].p.y;
                    float r2 = dx*dx + dy*dy;
                    
                    if (r2 < H2) {
                        _mobiles[i].density += MASS * kernel_poly6(r2);
                    }
                }
            }
        }
        
        // Calculer la pression avec l'équation d'état
        //_mobiles[i].pressure = GAS_CONSTANT * (_mobiles[i].density - REST_DENSITY);
		float density_ratio = _mobiles[i].density / REST_DENSITY;
		if (density_ratio > 1.0f) {
		    // Pression positive (répulsive) si densité > REST_DENSITY
		    _mobiles[i].pressure = GAS_CONSTANT * (powf(density_ratio, 4) - 1.0f);
		} else {
		    // Pression négative (attractive) si densité < REST_DENSITY, mais plus faible
            _mobiles[i].pressure = GAS_CONSTANT * 2.0f * (density_ratio - 1.0f);
		}
    }
    
    // Calculer les forces
    for (int i = 0; i < _nb_mobiles; i++) {
        
        //int cell_id = _mobiles[i].cell_id;
        
        // Parcourir les 9 cellules voisines
        for (int offsetY = -1; offsetY <= 1; offsetY++) {
            for (int offsetX = -1; offsetX <= 1; offsetX++) {
                int cellX = (int)(((_mobiles[i].p.x + 1.0f) / 2.0f) / CELL_SIZE) + offsetX;
                int cellY = (int)(((_mobiles[i].p.y + 1.0f) / 2.0f) / CELL_SIZE) + offsetY;
                
                if (cellX < 0 || cellY < 0 || cellX >= 32 || cellY >= 32) continue;
                
                int neighbor_cell_id = (cellY * 32 + cellX) % HASH_SIZE;
                
                if (_grid[neighbor_cell_id].start_index == -1) continue;
                
                // Parcourir les particules de cette cellule
                for (int k = 0; k < _grid[neighbor_cell_id].count; k++) {
                    int j = _cell_indices[_grid[neighbor_cell_id].start_index + k];
                    if (i == j) continue;
                    
                    float dx = _mobiles[j].p.x - _mobiles[i].p.x;
                    float dy = _mobiles[j].p.y - _mobiles[i].p.y;
                    float r2 = dx*dx + dy*dy;
                    float r = sqrtf(r2);
                    
                    if (r < H && r > 0.0001f) {
                        // Force de pression
                        float pressure_factor = -MASS * (_mobiles[i].pressure + _mobiles[j].pressure) / 
                                               (2.0f * _mobiles[j].density) * kernel_spiky_gradient(r);
                        //printf("Pressure factor: %f\n", pressure_factor);
                        _mobiles[i].force.x += pressure_factor * dx / r;
                        _mobiles[i].force.y += pressure_factor * dy / r;
                        
                        // Force de viscosité
                        float visc_factor = VISCOSITY * MASS * 
                                         (_mobiles[j].v.x - _mobiles[i].v.x) * 
                                         //kernel_viscosity_laplacian(r) / _mobiles[j].density;
										 kernel_viscosity_improved(r, H);
                        
                        _mobiles[i].force.x += visc_factor*0.1f;
                        
                        visc_factor = VISCOSITY * MASS * 
                                   (_mobiles[j].v.y - _mobiles[i].v.y) * 
                                   //kernel_viscosity_laplacian(r) / _mobiles[j].density;
								   kernel_viscosity_improved(r, H);
                        
                        _mobiles[i].force.y += visc_factor;
						float min_distance = _mobiles[i].r + _mobiles[j].r;
						// Force de répulsion à courte distance (deux-couches)
						if (r < min_distance * 1.5f) {
						    // Première couche - très forte répulsion si presque en contact
						    if (r < min_distance * 1.1f) {
						        float repulsion_strength = 50.0f * (min_distance * 1.1f - r) / min_distance;
						        _mobiles[i].force.x += repulsion_strength * dx / r;
						        _mobiles[i].force.y += repulsion_strength * dy / r;
						    } 
						    // Deuxième couche - répulsion plus douce
						    else {
						        float repulsion_strength = 5.0f * (min_distance * 1.5f - r) / min_distance;
						        _mobiles[i].force.x += repulsion_strength * dx / r;
						        _mobiles[i].force.y += repulsion_strength * dy / r;
						    }
						}
						min_distance = _mobiles[i].r + _mobiles[j].r;
						if (r < min_distance) {
							float repulsion_strength = 10.0f * (min_distance - r) / min_distance;
							_mobiles[i].force.x += repulsion_strength * dx / r;
							_mobiles[i].force.y += repulsion_strength * dy / r;
						}
						
						// Limiter la densité
						if (_mobiles[i].density > REST_DENSITY * 2.0f) {
							_mobiles[i].v.x *= 0.5f;
							_mobiles[i].v.y *= 0.5f;
						}
						if (r < min_distance * 1.5f) {
						    float repulsion_strength = 2.0f * (min_distance * 1.5f - r) / min_distance;
						    _mobiles[i].force.x -= repulsion_strength * dx / r;
						    _mobiles[i].force.y -= repulsion_strength * dy / r;
						}
                    }
                }
            }
        }
		//limiter leur magnitude
        float force_magnitude = sqrtf(_mobiles[i].force.x * _mobiles[i].force.x + 
                                     _mobiles[i].force.y * _mobiles[i].force.y);
        const float max_force = 1000.0f;
        
        if (force_magnitude > max_force) {
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
        //printf("Force: (%f, %f)\n", _mobiles[i].force.x, _mobiles[i].force.y);
    }
}
/*fin fonction de ouf*/

/* initialise des paramètres GL et GL4D */
void init(void){        
        _quad = gl4dgGenQuadf();
        /* activer la synchronisation verticale */
        SDL_GL_SetSwapInterval(1);
        /* set la couleur d'effacement OpenGL */
        TIME_SCALE = 10.0f;
        vitesse = 10.0f;
        _g.y = -9.81f;
	_quad = gl4dgGenQuadf();
	/* activer la synchronisation verticale */
	SDL_GL_SetSwapInterval(1);
	/* set la couleur d'effacement OpenGL */
    TIME_SCALE = 10.0f;
    vitesse = 10.0f;
    _g.y = -9.81f;
    
    /* créer un programme GPU pour OpenGL (en GL4D) */
    _pId = gl4duCreateProgram("<vs>shaders/identity.vs", "<fs>shaders/calculs.fs", NULL);
    /* créer un programme GPU pour OpenGL (en GL4D) */
    _pId_particules = gl4duCreateProgram("<vs>shaders/parti.vs", "<fs>shaders/parti.fs", NULL);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    mobile_init(2000);
    //les rectangles
    rect_init_list(11); //la liste de rectangles
    // 0.99f droite -0.99f gauche
    rect_add(0.7f, -0.50f, 0.0f, 0.4f, 0.3f, 0.0f, 0.2f);
    rect_add(0.4f, -0.55f, 0.0f, 0.4f, 0.3f, 0.0f, 0.2f);
    rect_add(0.1f, -0.6f, 0.0f, 0.4f, 0.3f, 0.0f, 0.2f);
    rect_add(-0.2f, -0.65f, 0.0f, 0.4f, 0.3f, 0.0f, 0.2f);
    rect_add(-0.5f, -0.7f, 0.0f, 0.4f, 0.3f, 0.0f, 0.2f);
    rect_add(-0.8f, -0.75f, 0.0f, 0.4f, 0.3f, 0.0f, 0.2f);
    rect_add(-1.1f, -0.8f, 0.0f, 0.4f, 0.3f, 0.0f, 0.2f);
    rect_add(-1.4f, -0.85f, 0.0f, 0.4f, 0.3f, 0.0f, 0.2f);
    //rect_add(0.0f, -0.90f, 0.0f, 0.9f, 0.5f, 0.0f, 0.3f);
    oval_init_list(1); //la liste de poissons
    oval_add(poission_position[0], poission_position[1], 0.0f, 0.10f, 0.10f, 0.0f, -0.2f); // Poisson 1

    glGenVertexArrays(1, &particle_vao);
    glBindVertexArray(particle_vao);

    glGenBuffers(1, &particle_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, particle_vbo);

    //initiation avec une taille fixe
    glBufferData(GL_ARRAY_BUFFER, 2000 * sizeof(float) * 7, NULL, GL_DYNAMIC_DRAW);

    //configuration des attributs (position, rayon, couleur)
    glEnableVertexAttribArray(0); // position (vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); // rayon (float)
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2); // couleur (vec4)
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnable(GL_PROGRAM_POINT_SIZE);

    glBindVertexArray(0); // clean
}


void draw(void){
    static double t0 = 0.0;
    double t = gl4dGetElapsedTime() / 1000.0, dt = (t - t0);
    t0 = t;
	/* effacer le buffer de couleur (image) et le buffer de profondeur d'OpenGL */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	/* utiliser le programme GPU "_pId" */
	glUseProgram(_pId);
    mobile_simu();
	mobile_draw();
    rect_draw_all();
    //pour faire deux scene une avec et une sans poisson
    if (!(t0 < 50.0f)){
        update_fish_positions();
        oval_draw_all();
    }
    /* n'utiliser aucun programme GPU (pas nécessaire) */
    glUseProgram(0);
}

void mobile_init(int n){
	assert(_mobiles == NULL);
    _nb_mobiles = n;
    _mobiles = malloc(_nb_mobiles * sizeof *_mobiles);
    assert(_mobiles);
    // Créer une forme en ligne (une seule rangée) en haut à droite
    float min_x = 0.5f;       // Limite gauche de la zone de spawn
    float max_x = 0.9f;       // Limite droite de la zone de spawn
    float min_y = 0.5f;       // Limite basse de la zone de spawn
    float max_y = 0.9f;       // Limite haute de la zone de spawn
    
    for (int i = 0; i < n; i++) {
        // Position aléatoire dans la zone définie
        _mobiles[i].p.x = min_x + (max_x - min_x) * gl4dmSURand();
        _mobiles[i].p.y = min_y + (max_y - min_y) * gl4dmSURand();
        _mobiles[i].p.z = 0.0f;
        
        // Vitesse initiale nulle
        _mobiles[i].v.x = 0.0f;
        _mobiles[i].v.y = 0.0f;
        _mobiles[i].v.z = 0.0f;
        
        // Rayon légèrement variable pour plus de naturel
        _mobiles[i].r = 0.02f;// + 0.01f * gl4dmSURand(); //TODOOO
        
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



void mobile_simu(void) {
    static double t0 = 0;
    double t = gl4dGetElapsedTime() / 1000.0;
    double dt = (t - t0) * 30.0;
    t0 = t;

    if (dt > 0.03) dt = 0.03; // Limiter le pas de temps à 30 ms

    // Calculer les forces SPH
    compute_sph_forces();

    // Précalculer les valeurs constantes
    float dt_time_scale = dt * TIME_SCALE;
    float dt_vitesse = dt * vitesse;
    float max_speed_squared = 0.50f * 0.50f;
    float min_distance_squared = MIN_DISTANCE * MIN_DISTANCE;

    for (int i = 0; i < _nb_mobiles; ++i) {
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
        if (_mobiles[i].p.x - _mobiles[i].r <= -1.0f) {
            _mobiles[i].v.x = -_mobiles[i].v.x * e;
            _mobiles[i].p.x = -1.0f + _mobiles[i].r;
        } else if (_mobiles[i].p.x + _mobiles[i].r >= 1.0f) {
            _mobiles[i].v.x = -_mobiles[i].v.x * e;
            _mobiles[i].p.x = 1.0f - _mobiles[i].r;
        }

        if (_mobiles[i].p.y - _mobiles[i].r <= -1.0f) {
            _mobiles[i].v.y = -_mobiles[i].v.y * e;
            _mobiles[i].p.y = -1.0f + _mobiles[i].r;
        } else if (_mobiles[i].p.y + _mobiles[i].r >= 1.0f) {
            _mobiles[i].v.y = -_mobiles[i].v.y * e;
            _mobiles[i].p.y = 1.0f - _mobiles[i].r;
        }

        // Mettre à jour la couleur en fonction de la pression (visualisation) faut decommenter
        //float pressure_ratio = (_mobiles[i].pressure / (GAS_CONSTANT * REST_DENSITY)) * 0.01f;
        //pressure_ratio = fmaxf(0.0f, fminf(1.0f, pressure_ratio * 0.1f));
        //_mobiles[i].color[0] = 0.0f;
        //_mobiles[i].color[1] = 0.4f;
        //_mobiles[i].color[2] = 0.8f;

        // Calculer la vitesse actuelle
        float speed_squared = _mobiles[i].v.x * _mobiles[i].v.x + _mobiles[i].v.y * _mobiles[i].v.y;

        // Si la vitesse dépasse le maximum, la réduire
        if (speed_squared > max_speed_squared) {
            float scale = 0.50f / sqrtf(speed_squared);
            _mobiles[i].v.x *= scale;
            _mobiles[i].v.y *= scale;
        }
    }

    // Détection et résolution des collisions entre mobiles
    for (int i = 0; i < _nb_mobiles; ++i) {
        for (int j = i + 1; j < _nb_mobiles; ++j) {
            float dx = _mobiles[j].p.x - _mobiles[i].p.x;
            float dy = _mobiles[j].p.y - _mobiles[i].p.y;
            float dist2 = dx * dx + dy * dy;

            // Somme des rayons
            float sumRadii = _mobiles[i].r + _mobiles[j].r;
            float sumRadiiSquared = sumRadii * sumRadii;

            // Test de collision
            if (dist2 < sumRadiiSquared) {
                float dist = sqrtf(dist2);
                if (dist < 0.0001f) dist = 0.0001f; // éviter la division par zéro

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
                if (dot > 0.0f) continue;

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
    if (!(t0 < 50.0f)){
        oval_collide_all(_mobiles, _nb_mobiles, e);
    }
}


//void mobile_draw(void){
//	GLfloat *tmp = malloc(4 * _nb_mobiles * sizeof *tmp);
//	assert(tmp);
//	for (int i = 0; i < _nb_mobiles; ++i){
//		tmp[4 * i + 0] = _mobiles[i].p.x;
//		tmp[4 * i + 1] = _mobiles[i].p.y;
//		tmp[4 * i + 2] = _mobiles[i].r;
//	}
//	glUniform4fv(glGetUniformLocation(_pId, "positions"), _nb_mobiles, tmp);
//	for (int i = 0; i < _nb_mobiles; ++i)
//		for (int j = 0; j < 4; ++j)
//			tmp[4 * i + j] = _mobiles[i].color[j];
//	glUniform4fv(glGetUniformLocation(_pId, "couleurs"), _nb_mobiles, tmp);
//	glUniform1i(glGetUniformLocation(_pId, "nbe"), _nb_mobiles);
//	//TODO met le scatering et blur
//	gl4dgDraw(_quad);
//	free(tmp);
//}
void mobile_draw(void){
    GLfloat *tmp = malloc(sizeof(float) * 7 * _nb_mobiles);
    assert(tmp);

    for (int i = 0; i < _nb_mobiles; ++i) {
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

    glUseProgram(_pId_particules); // ton shader pour les particules
    glBindVertexArray(particle_vao);
    glDrawArrays(GL_POINTS, 0, _nb_mobiles);
    glBindVertexArray(0);
}