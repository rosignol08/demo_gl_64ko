/*!\file porte.c
 * \brief GL4Dummies, simple shader demo
 * \author Modified from existing code
 * \date April 20, 2025
 */
#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dm.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4dh.h>

static void init(void);
static void draw(void);

/* Variables pour le rendu OpenGL */
static GLuint _pid = 0;
static GLuint _quad = 0;

void shader_test(int state)
{
    switch (state)
    {
    case GL4DH_INIT:
        init();
        return;
    case GL4DH_FREE:
        if (_quad)
        {
            _quad = 0;
        }
        if (_pid)
        {
            glDeleteProgram(_pid);
            _pid = 0;
        }
        return;
    case GL4DH_UPDATE_WITH_AUDIO:
        return;
    default: /* GL4DH_DRAW */
        draw();
        return;
    }
}

static void init(void)
{
    // Nouvelle manière include direct avec imfs (merci julien)
    const char *imfs = "<imfs>test.fs</imfs>\n"
#ifdef __GLES4D__
                       "#version 300 es\n"
                       "precision mediump float;\n"
#else
                       "#version 330 core\n"
#endif
                       "uniform float time;\n"
                       "uniform vec2 resolution;\n"
                       "in vec2 vsoTexCoord;\n"
                       "in vec3 fragPos;\n"
                       "out vec4 FragColor;\n"
                       "const vec3 lightDir=vec3(-.486664263392288,.811107105653813,-.324442842261525),backgroundColor=vec3(1,.808,0),gateColor=vec3(.255,.835,.58);\n"
                       "vec3 cPos,cDir;\n"
                       "float normalizedGlobalTime=0.;\n"
                       "struct Intersect{bool isHit;vec3 position;float distance;vec3 normal;int material;vec3 color;};\n"
                       "float sphereDist(vec3 p,vec3 c,float r)\n"
                       "{\n"
                       "return length(p-c)-r;\n"
                       "}\n"
                       "float sdCappedCylinder(vec3 p,vec2 h)\n"
                       "{\n"
                       "h=abs(vec2(length(p.xz),p.y))-h;\n"
                       "return min(max(h.x,h.y),0.)+length(max(h,0.));\n"
                       "}\n"
                       "float udBox(vec3 p,vec3 b)\n"
                       "{\n"
                       "return length(max(abs(p)-b,0.));\n"
                       "}\n"
                       "float udFloor(vec3 p)\n"
                       "{\n"
                       "float d=-.5;\n"
                       "for(float i=0.;i<3.;i++)\n"
                       "{\n"
                       "float f=pow(2.,i);\n"
                       "d+=.05/f*(sin(f*p.x+1.2*time)+sin(f*p.z+1.2*time));\n"
                       "}\n"
                       "return dot(p,vec3(0,1,0))-d;\n"
                       "}\n"
                       "float dGate(vec3 p)\n"
                       "{\n"
                       "p.y-=.65;\n"
                       "float left=sdCappedCylinder(p-vec3(-1,0,0),vec2(.05,1.3)),right=sdCappedCylinder(p-vec3(1,0,0),vec2(.05,1.3)),mid=udBox(p-vec3(0,1,0),vec3(1.2,.1,.05)),roofY=2-abs(p.x)*.5,curveFade=pow(clamp(abs(p.x/1.8),0.,1.),6.5),mainCurve=udBox(p-vec3(0,2-.6+.2*curveFade-.1,0),vec3(1.9,.04,.05)),leftTip=sdCappedCylinder(p-vec3(-1.85,roofY+.3+.04,0),vec2(.02,.1)),rightTip=sdCappedCylinder(p-vec3(1.85,roofY+.3+.04,0),vec2(.02,.1)),leftOrn=sphereDist(p,vec3(-1.85,roofY+.2,0),.06);\n"
                       "curveFade=sphereDist(p,vec3(1.85,roofY+.2,0),.06);\n"
                       "roofY=sphereDist(p,vec3(0,-1,10),3);\n"
                       "return min(min(min(min(left,right),mid),min(min(min(leftTip,rightTip),min(leftOrn,curveFade)),mainCurve)),roofY);\n"
                       "}\n"
                       "float sceneDistance(vec3 p)\n"
                       "{\n"
                       "return min(udFloor(p),dGate(p));\n"
                       "}\n"
                       "Intersect gateIntersect(vec3 p)\n"
                       "{\n"
                       "Intersect g;\n"
                       "g.distance=1e5;\n"
                       "g.material=0;\n"
                       "p.y-=.65;\n"
                       "float d=sdCappedCylinder(p-vec3(-1,0,0),vec2(.05,1.3));\n"
                       "if(d<g.distance)\n"
                       "g.distance=d,g.material=0;\n"
                       "d=sdCappedCylinder(p-vec3(1,0,0),vec2(.05,1.3));\n"
                       "if(d<g.distance)\n"
                       "g.distance=d,g.material=0;\n"
                       "d=udBox(p-vec3(0,1,0),vec3(1.2,.1,.05));\n"
                       "if(d<g.distance)\n"
                       "g.distance=d,g.material=0;\n"
                       "float roofY=2.-abs(p.x)*.5,curveFade=pow(clamp(abs(p.x/1.8),0.,1.),6.5);\n"
                       "d=udBox(p-vec3(0,1.4+.2*curveFade-.1,0),vec3(1.9,.04,.05));\n"
                       "if(d<g.distance)\n"
                       "g.distance=d,g.material=0;\n"
                       "d=sdCappedCylinder(p-vec3(-1.8,roofY+.3+.04,0),vec2(.02,.1));\n"
                       "if(d<g.distance)\n"
                       "g.distance=d,g.material=0;\n"
                       "d=sdCappedCylinder(p-vec3(1.8,roofY+.3+.04,0),vec2(.02,.1));\n"
                       "if(d<g.distance)\n"
                       "g.distance=d,g.material=0;\n"
                       "d=sphereDist(p,vec3(-1.8,roofY+.45,0),.06);\n"
                       "if(d<g.distance)\n"
                       "g.distance=d,g.material=0;\n"
                       "d=sphereDist(p,vec3(1.8,roofY+.45,0),.06);\n"
                       "if(d<g.distance)\n"
                       "g.distance=d,g.material=0;\n"
                       "d=sphereDist(p,vec3(0,-1,10),3);\n"
                       "if(d<g.distance)\n"
                       "g.distance=d,g.material=3;\n"
                       "return g;\n"
                       "}\n"
                       "Intersect sceneIntersect(vec3 p)\n"
                       "{\n"
                       "Intersect a;\n"
                       "a.distance=udFloor(p);\n"
                       "a.material=1;\n"
                       "Intersect g=gateIntersect(p);\n"
                       "if(g.distance<a.distance)\n"
                       "a=g;\n"
                       "return a;\n"
                       "}\n"
                       "vec3 getNormal(vec3 p)\n"
                       "{\n"
                       "vec2 e=vec2(1,-1)*.001;\n"
                       "return normalize(e.xyy*sceneDistance(p+e.xyy)+e.yyx*sceneDistance(p+e.yyx)+e.yxy*sceneDistance(p+e.yxy)+e.xxx*sceneDistance(p+e.xxx));\n"
                       "}\n"
                       "Intersect getRayColor(vec3 origin,vec3 ray)\n"
                       "{\n"
                       "float dist,depth=0.;\n"
                       "vec3 p=origin;\n"
                       "int count=0;\n"
                       "Intersect nearest;\n"
                       "for(int i=0;i<50;i++)\n"
                       "{\n"
                       "dist=sceneDistance(p);\n"
                       "depth+=dist;\n"
                       "p=origin+depth*ray;\n"
                       "count=i;\n"
                       "if(abs(dist)<1e-4)\n"
                       "break;\n"
                       "}\n"
                       "if(abs(dist)<1e-4)\n"
                       "{\n"
                       "nearest=sceneIntersect(p);\n"
                       "nearest.position=p;\n"
                       "nearest.normal=getNormal(p);\n"
                       "nearest.distance=depth;\n"
                       "float diffuse=clamp(dot(lightDir,nearest.normal),.4,1.);\n"
                       "if(nearest.material==0)\n"
                       "nearest.color=gateColor*diffuse*1.2;\n"
                       "else if(nearest.material==1)\n"
                       "nearest.color=vec3(.2);\n"
                       "else if(nearest.material==3)\n"
                       "nearest.color=vec3(1,.11,.11)*4.;\n"
                       "nearest.color+=vec3(.1,.15,.15);\n"
                       "nearest.isHit=true;\n"
                       "}\n"
                       "else\n"
                       "nearest.color=backgroundColor,nearest.isHit=false;\n"
                       "nearest.color=clamp(nearest.color-.05*nearest.distance,0.,1.);\n"
                       "return nearest;\n"
                       "}\n"
                       "void main()\n"
                       "{\n"
                       "normalizedGlobalTime=mod(time/75.,1.);\n"
                       "vec2 p=vsoTexCoord.xy*2.-1.;\n"
                       "p.x*=resolution.x/resolution.y;\n"
                       "cPos=vec3(0,1,-4);\n"
                       "cDir=normalize(vec3(0,-.1,1));\n"
                       "vec3 cSide=normalize(cross(cDir,vec3(0,1,0))),cUp=normalize(cross(cSide,cDir));\n"
                       "cUp=normalize(cSide*p.x+cUp*p.y+cDir*1.3);\n"
                       "cSide=vec3(0);\n"
                       "float alpha=1.;\n"
                       "Intersect nearest;\n"
                       "for(int i=0;i<3;i++)\n"
                       "{\n"
                       "nearest=getRayColor(cPos,cUp);\n"
                       "cSide+=alpha*nearest.color;\n"
                       "alpha*=.9;\n"
                       "if(!nearest.isHit||nearest.material!=1)\n"
                       "break;\n"
                       "cUp=normalize(reflect(cUp,nearest.normal));\n"
                       "cPos=nearest.position+nearest.normal*.001;\n"
                       "}\n"
                       "if(!nearest.isHit)\n"
                       "{\n"
                       "float skyGradient=clamp(pow(.5*(cUp.y+.6),.8),0.,1.);\n"
                       "cSide=mix(vec3(.92,.53,.32),vec3(.98,.82,.14),skyGradient);\n"
                       "}\n"
                       "if(!nearest.isHit)\n"
                       "{\n"
                       "vec3 cloudPos=cUp*30.;\n"
                       "cloudPos.x*=.7;\n"
                       "float noise=0.;\n"
                       "for(int i=0;i<6;i++)\n"
                       "{\n"
                       "float scale=pow(2.,float(i));\n"
                       "noise+=sin(cloudPos.x*.3*scale+time*.2)*sin(cloudPos.z*.3*scale+time*.15)*(.35/scale);\n"
                       "}\n"
                       "float cloudMask=smoothstep(0.,.3,cUp.y)*(smoothstep(.15,.25,noise)*.6);\n"
                       "noise=sin(cloudPos.x*.5)*sin(cloudPos.z*.2)*sin(cloudPos.x*.08+cloudPos.z*.15);\n"
                       "cloudMask*=smoothstep(0.,.2,noise+.3);\n"
                       "cSide=mix(cSide,vec3(1,.98,.9),cloudMask);\n"
                       "}\n"
                       "FragColor=vec4(cSide,1);\n"
                       "}";
    const char *imvs = "<imvs>test.vs</imvs>\n"
#ifdef __GLES4D__
                       "#version 300 es\n"
#else
                       "#version 330 core\n"
#endif
                       "layout (location = 0) in vec3 vsiPosition;\n"
                       "layout (location = 1) in vec3 vsiNormal;\n"
                       "layout (location = 2) in vec2 vsiTexCoord;\n"
                       "uniform int inv; \n"
                       "out vec2 vsoTexCoord;\n"
                       "void main(void) {\n"
                       "    gl_Position = vec4(vsiPosition, 1.0);\n"
                       "    if(inv != 0)\n"
                       "      vsoTexCoord = vec2(vsiTexCoord.s, 1.0 - vsiTexCoord.t);\n"
                       "    else\n"
                       "      vsoTexCoord = vec2(vsiTexCoord.s, vsiTexCoord.t);\n"
                       "  }\n";

    _pid = gl4duCreateProgram(imvs, imfs, NULL);

    _quad = gl4dgGenQuadf();
}

static void draw(void)
{
    static double t0 = 0.0;
    double t = gl4dGetElapsedTime() / 1000.0;
    double dt = t - t0;
    t0 = t;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(_pid);
    glUniform1f(glGetUniformLocation(_pid, "time"), (float)t);
    glUniform2f(glGetUniformLocation(_pid, "resolution"), (float)1280, (float)720);

    gl4duBindMatrix("modelViewMatrix");
    gl4duLoadIdentityf();
    gl4duSendMatrices();

    gl4dgDraw(_quad);
    glUseProgram(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}
/*
triangle de https://github.com/marklundin/glsl-sdf-primitives/blob/master/udTriangle.glsl
shader inspiré de https://www.shadertoy.com/view/XttSWN
*/