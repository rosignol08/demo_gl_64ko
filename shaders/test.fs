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
const vec3 backgroundColor = vec3(1, 0.808, 0.0);//0.388, 0.686, 0.91);
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
const int ROOF_MATERIAL2 = 4; // pour la sphere de soleil

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
float sdPyramidRoof(vec3 p, vec3 c, float width, float height) {
    // Position relative to center
    p = p - c;
    
    // Elongate the roof along the Z axis
    float lengthZ = width * 2.0;  // Make it longer in Z direction
    float lengthX = width;        // Keep original width in X direction
    
    // Top ridge line instead of a point
    float halfRidgeLength = lengthZ * 0.5;
    vec3 ridgeStart = vec3(0.0, height, -halfRidgeLength);
    vec3 ridgeEnd = vec3(0.0, height, halfRidgeLength);
    
    // Create elongated roof with triangular sides
    float t1 = udTriangle(p, vec3(-lengthX, 0.0, -halfRidgeLength), vec3(-lengthX, 0.0, halfRidgeLength), ridgeStart);
    float t2 = udTriangle(p, vec3(-lengthX, 0.0, halfRidgeLength), ridgeEnd, ridgeStart);
    float t3 = udTriangle(p, vec3(lengthX, 0.0, -halfRidgeLength), vec3(lengthX, 0.0, halfRidgeLength), ridgeEnd);
    float t4 = udTriangle(p, vec3(lengthX, 0.0, -halfRidgeLength), ridgeStart, ridgeEnd);
    float t5 = udTriangle(p, vec3(-lengthX, 0.0, -halfRidgeLength), vec3(lengthX, 0.0, -halfRidgeLength), ridgeStart);
    float t6 = udTriangle(p, vec3(-lengthX, 0.0, halfRidgeLength), vec3(lengthX, 0.0, halfRidgeLength), ridgeEnd);
    
    // Combine all triangular sides (removed the base plane max operation)
    return min(min(min(t1, t2), min(t3, t4)), min(t5, t6));
}


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
    float backOffset = 0.2;
    // Colonnes latérales (piliers verts)
    float left = sdCappedCylinder(p - vec3(-1.0, 0.0, 0.0), vec2(r, 1.3));
    float right = sdCappedCylinder(p - vec3(1.0, 0.0, 0.0), vec2(r, 1.3));

    // Traverse centrale décorative (verte aussi)
    float mid = udBox(p - vec3(0.0, 1.0, 0.0), vec3(1.2, 0.1, r));
    // Toit triangulaire noir
    float roofHeight = 2;
    float roofWidth = 1.8;
    float roofY = roofHeight - abs(p.x) * 0.5;
    // Définition des 3 sommets du triangle (vu de face)
//    vec3 a = vec3(-roofWidth, roofHeight - 0.8, backOffset); // coin gauche
//    vec3 b = vec3( roofWidth, roofHeight - 0.8, backOffset); // coin droit
//    vec3 c = vec3(0.0, roofHeight, 0.0);              // sommet
//    // Pyramid roof parameters
//    vec3 roofCenter = vec3(0.0, 1.7, backOffset * 0.5); // Center point of the pyramid
//    float pyramidWidth = 1.2;  // Width of pyramid base
//    float pyramidHeight = 0.8; // Height of pyramid
//    //float roof = sdPyramidRoof(p, roofCenter, pyramidWidth, pyramidHeight);
//    //float roof = udTriangle(p, a, b, c);
    // Courbes sinusoïdales sur le toit
    float roofBaseHeight = roofHeight - 0.6;
    float r_curve = 0.05; // épaisseur des tiges
    
    // Tige courbée principale
    float waveMagnitude = 0.2;
    float curveFade = pow(clamp(abs(p.x / roofWidth), 0.0, 1.0), 6.50); // 0 au centre, 1 aux bords
    float roofCurve = waveMagnitude * curveFade;
    
    
    vec3 roofTopCenter = vec3(0.0, roofBaseHeight + roofCurve -0.1, 0.0);
    float mainCurve = udBox(p - roofTopCenter, vec3(1.9, 0.04, r_curve));
    
    float arcs = mainCurve;
    
    // Décalage vers l'arrière pour les tiges et ornements (z positif = arrière)
    //float backOffset = 0.5;
    
    // Tiges décoratives aux coins (vertes)
    float tipHeight = 0.1;
    float leftTip = sdCappedCylinder(p - vec3(-roofWidth-0.05, (roofY + 0.3) + tipHeight * 0.4, 0.0), vec2(r*0.4, tipHeight));
    float rightTip = sdCappedCylinder(p - vec3(roofWidth+0.05, (roofY + 0.3) + tipHeight * 0.4, 0.0), vec2(r*0.4, tipHeight));
    
    // Ornements aux extrémités des tiges
    float leftOrn = sphereDist(p, vec3(-roofWidth-0.05, roofY + tipHeight * 2.0, 0.0), r * 1.2);
    float rightOrn = sphereDist(p, vec3(roofWidth+0.05, roofY + tipHeight * 2.0, 0.0), r * 1.2);
    
    // Fusion des tiges et ornements
    float tips = min(min(leftTip, rightTip), min(leftOrn, rightOrn));
    
    //pour la perle en haut de la porte
    float pearl = sphereDist(p, vec3(0.0, 0-1.0, 10.0), 3);

    // Fusion de tous les éléments
    return min(min(min(min(left, right), mid), min( tips, arcs)),pearl);
    //return min(min(min(min(left, right), mid), min(tips, arcs)),pearl);
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

Intersect gateIntersect(vec3 p) {
    Intersect g;
    g.distance = 1e5;
    g.material = BASIC_MATERIAL;

    p.y -= 1.3 * 0.5;

    float r = 0.05;
    float backOffset = 0.2;

    float d;

    // Piliers verts
    d = sdCappedCylinder(p - vec3(-1.0, 0.0, 0.0), vec2(r, 1.3));
    if (d < g.distance) { g.distance = d; g.material = BASIC_MATERIAL; }

    d = sdCappedCylinder(p - vec3(1.0, 0.0, 0.0), vec2(r, 1.3));
    if (d < g.distance) { g.distance = d; g.material = BASIC_MATERIAL; }

    // Traverse verte
    d = udBox(p - vec3(0.0, 1.0, 0.0), vec3(1.2, 0.1, r));
    if (d < g.distance) { g.distance = d; g.material = BASIC_MATERIAL; }

    // Toit triangulaire noir
    float roofHeight = 2.0;
    float roofWidth = 1.8;
    float roofY = roofHeight - abs(p.x) * 0.5;
    //vec3 a = vec3(-roofWidth, roofHeight - 0.8, backOffset);
    //vec3 b = vec3( roofWidth, roofHeight - 0.8, backOffset);
    //vec3 c = vec3(0.0, roofHeight, 0.0);
    //d = udTriangle(p, a, b, c);
    //if (d < g.distance) { g.distance = d; g.material = ROOF_MATERIAL; }

    // Tige courbe centrale (verte)
    float roofBaseHeight = roofHeight - 0.6;
    float r_curve = 0.05;
    float waveMagnitude = 0.2;
    float curveFade = pow(clamp(abs(p.x / roofWidth), 0.0, 1.0), 6.5);
    float roofCurve = waveMagnitude * curveFade;
    vec3 roofTopCenter = vec3(0.0, roofBaseHeight + roofCurve-0.1, 0.0);
    d = udBox(p - roofTopCenter, vec3(1.9, 0.04, r_curve));
    if (d < g.distance) { g.distance = d; g.material = BASIC_MATERIAL; }

    // Tiges et ornements verts
    float tipHeight = 0.1;
    d = sdCappedCylinder(p - vec3(-roofWidth, (roofY + 0.3) + tipHeight * 0.4, 0.0), vec2(r * 0.4, tipHeight));
    if (d < g.distance) { g.distance = d; g.material = BASIC_MATERIAL; }

    d = sdCappedCylinder(p - vec3(roofWidth, (roofY + 0.3) + tipHeight * 0.4, 0.0), vec2(r * 0.4, tipHeight));
    if (d < g.distance) { g.distance = d; g.material = BASIC_MATERIAL; }

    d = sphereDist(p, vec3(-roofWidth, roofY + tipHeight * 4.5, 0.0), r * 1.2);
    if (d < g.distance) { g.distance = d; g.material = BASIC_MATERIAL; }

    d = sphereDist(p, vec3(roofWidth, roofY + tipHeight * 4.5, 0.0), r * 1.2);
    if (d < g.distance) { g.distance = d; g.material = BASIC_MATERIAL; }

    // Perle lumineuse
    d = sphereDist(p, vec3(0.0, -1.0, 10.0), 3);
    
    if (d < g.distance) { g.distance = d; g.material = ROOF_MATERIAL; }

    return g;
}


Intersect sceneIntersect(vec3 p) {
    Intersect a;
    float floorDist = udFloor(p);
    a.distance = floorDist;
    a.material = MIRROR_MATERIAL;

    Intersect g = gateIntersect(p);
    if (g.distance < a.distance) {
        a = g;
    }

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
            //nearest.color = vec3(0.2, 0.51, 0.376) * diffuse + vec3(0.0) * specular;
            nearest.color = vec3(0.2, 0.2, 0.2);
        }else if(nearest.material == EMISSIVE_MATERIAL){
            // Couleur verte brillante pour la perle
            nearest.color = vec3(1, 0.282, 0.204) * 1.0; // très lumineux
        } else if(nearest.material == ROOF_MATERIAL) {
            nearest.color = vec3(1, 0.11, 0.11) * 4.0; // très lumineux
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
        alpha *= 0.9; // Réduire encore plus l'opacité du reflet
        
        if(!nearest.isHit || nearest.material != MIRROR_MATERIAL) break;
        
        ray = normalize(reflect(ray, nearest.normal));
        cPos = nearest.position + nearest.normal * OFFSET;
    }
    
    // Ajouter un dégradé de ciel si aucun objet n'est touché
    if (!nearest.isHit) {
        // Créer un dégradé basé sur la direction du rayon (y)
        float skyGradient = pow(0.5 * (ray.y + 1.0), 0.8); // Transforme [-1,1] en [0,1] avec une courbe plus prononcée
        
        // Couleurs plus contrastées pour le dégradé
        vec3 skyColorBottom = vec3(0.92, 0.53, 0.32);    // Jaune/orange plus vif en bas
        vec3 skyColorTop = vec3(0.98, 0.82, 0.14);        // Orange/rouge plus intense en haut
        
        // Remplacer la couleur de fond par le dégradé du ciel
        color = mix(skyColorBottom, skyColorTop, skyGradient);
    }
    // Add clouds when looking at sky
    if (!nearest.isHit) {
        // Cloud noise function
        vec3 cloudPos = ray * 30.0; // Scale increased to shrink clouds
        // Stretch clouds on X axis
        cloudPos.x *= 0.7; // Reduce X frequency to stretch clouds horizontally
        float noise = 0.0;
        
        // More iterations for finer detail
        for (int i = 0; i < 6; i++) {
            float scale = pow(2.0, float(i));
            // Higher frequency values and faster time factor for quicker variation
            noise += (sin(cloudPos.x * 0.3 * scale + time * 0.2) * 
                     sin(cloudPos.z * 0.3 * scale + time * 0.15)) * (0.35 / scale);
        }
        
        // Only show clouds in the upper hemisphere
        float cloudMask = smoothstep(0.0, 0.3, ray.y);
        
        // Sharper thresholds for more defined small clouds
        float cloudDensity = smoothstep(0.15, 0.25, noise) * 0.6 * cloudMask;
        
        // Add cloud distribution pattern for scattered appearance
        float distribution = sin(cloudPos.x * 0.5) * sin(cloudPos.z * 0.2) * sin(cloudPos.x * 0.08 + cloudPos.z * 0.15);
        cloudDensity *= smoothstep(0.0, 0.2, distribution + 0.3);
        
        vec3 cloudColor = vec3(1.0, 0.98, 0.9);
        
        color = mix(color, cloudColor, cloudDensity);
    }
    FragColor = vec4(color, 1.0);
}
