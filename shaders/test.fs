#version 330 core

// Entrées uniformes (remplaçant les variables globales de ShaderToy)
uniform float time;      // Remplace iTime
uniform vec2 resolution; // Remplace iResolution

// Entrées venant du vertex shader
in vec2 vsoTexCoord;    // Coordonnées de texture (remplace fragCoord)
in vec3 fragPos;        // Position du fragment

// Sortie
out vec4 FragColor;     // Remplace fragColor

// Constantes
const float EPS = 1e-4;
const float OFFSET = EPS * 10.0;
const float PI = 3.14159;
const float INF = 1e+10;

const vec3 lightDir = vec3(-0.48666426339228763, 0.8111071056538127, -0.3244428422615251);
const vec3 backgroundColor = vec3(0.388, 0.686, 0.91);
const vec3 gateColor = vec3(0.255, 0.835, 0.58);

const float totalTime = 75.0;

// Globals
vec3 cPos, cDir;
float normalizedGlobalTime = 0.0;

struct Intersect {
    bool isHit;
    vec3 position;
    float distance;
    vec3 normal;
    int material;
    vec3 color;
};
    
const int BASIC_MATERIAL = 0;
const int MIRROR_MATERIAL = 1;
const int EMISSIVE_MATERIAL = 2; // pour la perle
const int ROOF_MATERIAL = 3; // nouveau matériau noir

// Distance functions
vec3 opRep(vec3 p, float interval) {
    return mod(p, interval) - 0.5 * interval;
}

vec2 opRep(vec2 p, float interval) {
    return mod(p, interval) - 0.5 * interval;
}

float opRep(float x, float interval) {
    return mod(x, interval) - 0.5 * interval;
}

float sphereDist(vec3 p, vec3 c, float r) {
    return length(p - c) - r;
}

float sdCappedCylinder(vec3 p, vec2 h) {
    vec2 d = abs(vec2(length(p.xz), p.y)) - h;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float udBox(vec3 p, vec3 b) {
    return length(max(abs(p) - b, 0.0));
}

float udFloor(vec3 p) {
    float t1 = 1.0;
    float t2 = 3.0;
    float d = -0.5;
    for(float i = 0.0; i < 3.0; i++) {
        float f = pow(2.0, i);
        d += 0.1 / f * (sin(f * t1 * p.x + t2 * time) + sin(f * t1 * p.z + t2 * time));
    }
    return dot(p, vec3(0.0, 1.0, 0.0)) - d;
}
float dot2(in vec3 v) { return dot(v,v); }

float udTriangle(vec3 p, vec3 a, vec3 b, vec3 c)
{
    vec3 ba = b - a; vec3 pa = p - a;
    vec3 cb = c - b; vec3 pb = p - b;
    vec3 ac = a - c; vec3 pc = p - c;
    vec3 nor = cross(ba, ac);

    return sqrt(
    (sign(dot(cross(ba,nor),pa)) +
     sign(dot(cross(cb,nor),pb)) +
     sign(dot(cross(ac,nor),pc))<2.0)
     ?
     min( min(
     dot2(ba*clamp(dot(ba,pa)/dot2(ba),0.0,1.0)-pa),
     dot2(cb*clamp(dot(cb,pb)/dot2(cb),0.0,1.0)-pb) ),
     dot2(ac*clamp(dot(ac,pc)/dot2(ac),0.0,1.0)-pc) )
     :
     dot(nor,pa)*dot(nor,pa)/dot2(nor) );
}

//float dGate(vec3 p) {
//    p.y -= 1.3 * 0.5;
//    
//    float r = 0.05;
//    float left = sdCappedCylinder(p - vec3(-1.0, 0.0, 0.0), vec2(r, 1.3));
//    float right = sdCappedCylinder(p - vec3(1.0, 0.0, 0.0), vec2(r, 1.3));
//
//    float ty = 0.02 * p.x * p.x;
//    float tx = 0.5 * (p.y - 1.3);
//    float katsura = udBox(p - vec3(0.0, 1.3 + ty, 0.0), vec3(1.7 + tx, r * 2.0 + ty, r));
//
//    float kan = udBox(p - vec3(0.0, 0.7, 0.0), vec3(1.3, r, r));
//    float gakuduka = udBox(p - vec3(0.0, 1.0, 0.0), vec3(r, 0.3, r));
//
//    return min(min(left, right), min(gakuduka, min(katsura, kan)));
//}

//pour la perle en haut de la porte
//float pearl = sphereDist(p, vec3(0.0, 2., 0.0), 0.1);
//float dGate(vec3 p) {
//    p.y -= 1.3 * 0.5;
//    
//    float r = 0.05;
//
//    // Colonnes latérales
//    float left = sdCappedCylinder(p - vec3(-1.0, 0.0, 0.0), vec2(r, 1.3));
//    float right = sdCappedCylinder(p - vec3(1.0, 0.0, 0.0), vec2(r, 1.3));
//
//    // Nouveau toit style chinois (courbé)
//    float roofCurve = 0.3 * sin(p.x * 3.0); // vague douce
//    vec3 roofCenter = vec3(0.0, 1.5 + roofCurve, 0.0);
//    float roof = udBox(p - roofCenter, vec3(1.7, 0.1, r));
//
//    // Traverse centrale décorative
//    float mid = udBox(p - vec3(0.0, 1.0, 0.0), vec3(1.2, 0.1, r));
//    
//
//    return min(min(min(left, right), min(roof, mid)), pearl);
//
//}

float sdBentRod(vec3 p, vec3 center, float radius, float angleStart, float angleEnd, float thickness) {
    // Projette le point dans le plan XY relatif au centre
    vec2 rel = p.xy - center.xy;
    float angle = atan(rel.y, rel.x);
    float len = length(rel);
    
    // Clamp l’angle dans l’arc désiré
    float t = clamp((angle - angleStart) / (angleEnd - angleStart), 0.0, 1.0);
    float targetAngle = mix(angleStart, angleEnd, t);
    
    // Position de l'arc cible
    vec2 arcPos = center.xy + radius * vec2(cos(targetAngle), sin(targetAngle));
    
    // Distance euclidienne au tube courbé
    float d2D = length(rel - (arcPos - center.xy)) - thickness;

    // Combine avec la distance en Z
    return length(vec2(d2D, p.z - center.z)) - thickness;
}

float dGate(vec3 p) {
    p.y -= 1.3 * 0.5;
    
    float r = 0.05;

    // Colonnes latérales (piliers verts)
    float left = sdCappedCylinder(p - vec3(-1.0, 0.0, 0.0), vec2(r, 1.3));
    float right = sdCappedCylinder(p - vec3(1.0, 0.0, 0.0), vec2(r, 1.3));

    // Traverse centrale décorative (verte aussi)
    float mid = udBox(p - vec3(0.0, 1.0, 0.0), vec3(1.2, 0.1, r));
/*
    // Toit triangulaire noir
    // Calcul de la pente triangulaire
    float roofY = roofHeight - abs(p.x) * 0.5;
    vec3 roofPos = p - vec3(0.0, roofY, 0.0);
    float roof = udTriangle(roofPos, vec3(roofWidth, 0.08, r));
    // Base du toit, une bande horizontale noire
    float roofBaseY = roofHeight - 0.8; // Ajuste la hauteur pour bien remplir le vide
    vec3 basePos = p - vec3(0.0, roofBaseY, 0.0);
    float roofBase = udBox(basePos, vec3(roofWidth * 0.95, 0.3, r));
*/

// Toit triangulaire noir
float roofHeight = 2.1;
float roofWidth = 1.8;
float roofY = roofHeight - abs(p.x) * 0.5;
// Définition des 3 sommets du triangle (vu de face)
vec3 a = vec3(-roofWidth, roofHeight - 0.8, 0.0); // coin gauche
vec3 b = vec3( roofWidth, roofHeight - 0.8, 0.0); // coin droit
vec3 c = vec3(0.0, roofHeight, 0.0);              // sommet

float roof = udTriangle(p, a, b, c);

// Courbes sinusoïdales sur le toit
float roofBaseHeight = roofHeight - 0.6;
float r_curve = 0.05; // épaisseur des tiges

// Tige courbée principale
float waveMagnitude = 0.3;
float waveFrequency = 4.0;
float roofCurve = waveMagnitude * sin(p.x * waveFrequency);
vec3 roofTopCenter = vec3(0.0, roofBaseHeight + roofCurve, 0.0);
float mainCurve = udBox(p - roofTopCenter, vec3(1.7, 0.04, r_curve));

// Tige courbée secondaire meme taille mais dans l'autre sens
float secondCurve = -waveMagnitude * 0.6 * sin(p.x * waveFrequency * 1.2);
vec3 secondCurvePos = vec3(0.0, roofBaseHeight + 0.15 + secondCurve, 0.0);
float secondRod = udBox(p - secondCurvePos, vec3(1.7, 0.03, r_curve));

float arcs = min(mainCurve, secondRod);

    // Tiges décoratives aux coins (vertes)
    float tipHeight = 0.1;
    float leftTip = sdCappedCylinder(p - vec3(-roofWidth, (roofY + 0.3) + tipHeight * 0.4, 0.0), vec2(r*0.4, tipHeight));
    float rightTip = sdCappedCylinder(p - vec3(roofWidth, (roofY + 0.3) + tipHeight * 0.4, 0.0), vec2(r*0.4, tipHeight));
    
    // Ornements aux extrémités des tiges
    float leftOrn = sphereDist(p, vec3(-roofWidth, roofY + tipHeight * 4.5, 0.0), r * 1.2);
    float rightOrn = sphereDist(p, vec3(roofWidth, roofY + tipHeight * 4.5, 0.0), r * 1.2);
    
    // Fusion des tiges et ornements
    float tips = min(min(leftTip, rightTip), min(leftOrn, rightOrn));
    
    // Fusion de tous les éléments
    //return min(min(min(left, right), mid), min(roof, tips));
    return min(min(min(left, right), mid), min(min(roof, tips), arcs));

}


float sceneDistance(vec3 p) {
    float floorDist = udFloor(p);
    float gateDist = dGate(p);
    return min(floorDist, gateDist);
}

// Color functions
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

Intersect minIntersect(Intersect a, Intersect b) {
    if(a.distance < b.distance) {
        return a;
    } else {
        return b;
    }
}

Intersect sceneIntersect(vec3 p) {
//    Intersect a;
//    float floorDist = udFloor(p);
//    float gateDist = dGate(p);
//    
//    // On isole la perle pour savoir si c'est elle
//    float pearlDist = sphereDist(p, vec3(0.0, 1.8, 0.0), 0.1);
//
//    // Si la perle est la plus proche
//    if (pearlDist < gateDist && pearlDist < floorDist) {
//        a.distance = pearlDist;
//        a.material = EMISSIVE_MATERIAL;
//    } else if (floorDist < gateDist) {
//        // C'est le sol
//        a.distance = floorDist;
//        a.material = MIRROR_MATERIAL; // Le sol continue à refléter
//    } else {
//        // C'est la porte
//        a.distance = gateDist;
//        a.material = BASIC_MATERIAL; // La porte ne reflète plus
//    }
//    return a;
    Intersect a;
    float floorDist = udFloor(p);
    
    // Porte et ses composants
    float pillarL = sdCappedCylinder(p - vec3(-1.0, 0.65, 0.0), vec2(0.05, 1.3));
    float pillarR = sdCappedCylinder(p - vec3(1.0, 0.65, 0.0), vec2(0.05, 1.3));
    float traverse = udBox(p - vec3(0.0, 1.0, 0.0), vec3(1.2, 0.1, 0.05));
    
    // Toit triangulaire
    float roofHeight = 2.1;
    float roofWidth = 1.8;
    float roofY = roofHeight - abs(p.x) * 0.5;
    vec3 roofPos = p - vec3(0.0, roofY, 0.0);
    float roof = udBox(roofPos, vec3(roofWidth, 0.08, 0.05));
    
    // Tiges décoratives
    float tipHeight = 0.3;
    float tipL = sdCappedCylinder(p - vec3(-roofWidth, roofY + tipHeight * 0.5, 0.0), vec2(0.03, tipHeight));
    float tipR = sdCappedCylinder(p - vec3(roofWidth, roofY + tipHeight * 0.5, 0.0), vec2(0.03, tipHeight));
    
    // Ornements aux extrémités
    float ornL = sphereDist(p, vec3(-roofWidth, roofY + tipHeight, 0.0), 0.06);
    float ornR = sphereDist(p, vec3(roofWidth, roofY + tipHeight, 0.0), 0.06);
    
    // Perle décorative au centre du toit (optionnelle)
    float pearl = sphereDist(p, vec3(0.0, roofHeight + 0.1, 0.0), 0.1);

    // Recherche de l'élément le plus proche
    float d = floorDist;
    int mat = MIRROR_MATERIAL;
    
    // Pour chaque élément, vérifie s'il est plus proche que ce qu'on a déjà trouvé
    // et assigne le matériau approprié
    
    // Piliers verts
    if (pillarL < d) { d = pillarL; mat = BASIC_MATERIAL; }  // Matériau vert pour piliers
    if (pillarR < d) { d = pillarR; mat = BASIC_MATERIAL; }
    if (traverse < d) { d = traverse; mat = BASIC_MATERIAL; } // Traverse verte aussi
    
    // Toit noir
    if (roof < d) { d = roof; mat = ROOF_MATERIAL; }         // Matériau noir pour toit
    
    // Tiges et ornements verts
    if (tipL < d) { d = tipL; mat = BASIC_MATERIAL; }        // Tiges vertes
    if (tipR < d) { d = tipR; mat = BASIC_MATERIAL; }
    if (ornL < d) { d = ornL; mat = BASIC_MATERIAL; }        // Ornements verts
    if (ornR < d) { d = ornR; mat = BASIC_MATERIAL; }
    
    // Perle lumineuse
    if (pearl < d) { d = pearl; mat = EMISSIVE_MATERIAL; }   // Matériau lumineux pour perle
    
    a.distance = d;
    a.material = mat;
    return a;
}

vec3 getNormal(vec3 p) {
    vec2 e = vec2(1.0, -1.0) * 0.001;
    return normalize(
        e.xyy * sceneDistance(p + e.xyy) + e.yyx * sceneDistance(p + e.yyx) + 
        e.yxy * sceneDistance(p + e.yxy) + e.xxx * sceneDistance(p + e.xxx));
}

float getShadow(vec3 ro, vec3 rd) {
    float h = 0.0;
    float c = 0.0;
    float r = 1.0;
    float shadowCoef = 0.5;

    for(float t = 0.0; t < 50.0; t++) {
        h = sceneDistance(ro + rd * c);
        if(h < EPS) return shadowCoef;
        r = min(r, h * 16.0 / c);
        c += h;
    }

    return 1.0 - shadowCoef + r * shadowCoef;
}

Intersect getRayColor(vec3 origin, vec3 ray) {
    // marching loop
    float dist, minDist, trueDepth;
    float depth = 0.0;
    vec3 p = origin;
    int count = 0;
    Intersect nearest;

    // first pass
    for(int i = 0; i < 50; i++) {
        dist = sceneDistance(p);
        depth += dist;
        p = origin + depth * ray;

        count = i;
        if(abs(dist) < EPS) break;
    }

    if(abs(dist) < EPS) {
        nearest = sceneIntersect(p);
        nearest.position = p;
        nearest.normal = getNormal(p);
        nearest.distance = depth;

        float ambientIntensity = 0.4; //pour la lumière ambiante
        float diffuse = clamp(dot(lightDir, nearest.normal), ambientIntensity, 1.0);
        float specular = pow(clamp(dot(reflect(lightDir, nearest.normal), ray), 0.0, 1.0), 2.0);

        if(nearest.material == BASIC_MATERIAL) {
            // Couleur rouge pour la porte Torii
            nearest.color = gateColor * diffuse * 1.2;
        }else if(nearest.material == MIRROR_MATERIAL){
            nearest.color = vec3(0.5, 0.7, 0.8) * diffuse + vec3(0.0) * specular;
        }else if(nearest.material == EMISSIVE_MATERIAL){
            // Couleur verte brillante pour la perle
            nearest.color = vec3(0.2, 1.0, 0.3) * 2.5; // très lumineux
        } else if(nearest.material == ROOF_MATERIAL) {
            // Toit noir mat
            nearest.color = vec3(0.05, 0.05, 0.05) * diffuse;
        }
        nearest.color += vec3(0.1, 0.15, 0.15);
        nearest.isHit = true;
    } else {
        nearest.color = backgroundColor;
        nearest.isHit = false;
    }
    nearest.color = clamp(nearest.color - 0.05 * nearest.distance, 0.0, 1.0);
    
    return nearest;
}

void main() {
    normalizedGlobalTime = mod(time / totalTime, 1.0);

    // Normalisation des coordonnées d'écran (venant de vsoTexCoord au lieu de fragCoord)
    // Mapping [0,1] → [-1,1]
    vec2 p = vsoTexCoord.xy * 2.0 - 1.0;
    // Correction du ratio pour éviter la déformation
    p.x *= resolution.x / resolution.y;
    
    // Caméra fixe qui regarde vers la porte
    cPos = vec3(0.0, 1.0, -4.0);                // Position de la caméra
    cDir = normalize(vec3(0.0, -0.1, 1.0));     // Direction vers la porte

    // Calcul du rayon pour le raymarching
    vec3 cSide = normalize(cross(cDir, vec3(0.0, 1.0, 0.0)));
    vec3 cUp = normalize(cross(cSide, cDir));
    float targetDepth = 1.3;
    vec3 ray = normalize(cSide * p.x + cUp * p.y + cDir * targetDepth);

    // Accumulation de la couleur avec reflets
    vec3 color = vec3(0.0);
    float alpha = 1.0;
    Intersect nearest;

    for(int i = 0; i < 3 ; i++) {
        nearest = getRayColor(cPos, ray);
        
        color += alpha * nearest.color;
        alpha *= 0.3; // Réduire encore plus l'opacité du reflet
        
        if(!nearest.isHit || nearest.material != MIRROR_MATERIAL) break;
        
        ray = normalize(reflect(ray, nearest.normal));
        cPos = nearest.position + nearest.normal * OFFSET;
    }
    
    // Ajouter un dégradé de ciel si aucun objet n'est touché
    if (!nearest.isHit) {
        // Créer un dégradé basé sur la direction du rayon (y)
        float skyGradient = 0.5 * (ray.y + 1.0); // Transforme [-1,1] en [0,1]
        vec3 skyColor = mix(
            vec3(1.0, 1.0, 1.0),              // Blanc en bas
            vec3(0.5, 0.7, 1.0),              // Bleu ciel en haut
            skyGradient
        );
        
        // Remplacer la couleur de fond par le dégradé du ciel
        color = skyColor;
    }
    
    // Ajouter une touche finale de luminosité à toute la scène
    color *= 1.1; // Augmenter globalement la luminosité de 10%
    
    FragColor = vec4(color, 1.0);
}
