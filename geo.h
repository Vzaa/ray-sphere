#ifndef GEO_H
#define GEO_H
typedef struct
{
    float x;
    float y;
    float z;
} vec3D;

typedef struct
{
    vec3D center;
    float r;
    Uint32 color;
    int ref;
    float transparency;
} sphere;

typedef struct
{
    vec3D pos;
    Uint32 color;
} light;


light * createLight(float x, float y, float z, Uint32 c);
sphere * createSphere(float x, float y, float z, float r);
int intersectSphere(sphere * s, vec3D * p0, vec3D * d, vec3D * q, int * status);
void toUnit(vec3D *a, vec3D * n);
void add(vec3D *a, vec3D *b, vec3D * n);
void substract(vec3D *a, vec3D *b, vec3D * n);
void mult(vec3D *a, float b, vec3D * n);
float dot(vec3D *a, vec3D *b);
float mag(vec3D *a);
void cross(vec3D *a, vec3D *b, vec3D * n);
void veccopy(vec3D *a, vec3D *n);
void updateSphere(sphere * s, float x, float y, float z, float r, Uint32 c, int ref, float t);
void rotatex(vec3D *a, vec3D *n, float angle);
void rotatey(vec3D *a, vec3D *n, float angle);
void rotatez(vec3D *a, vec3D *n, float angle);

#endif
