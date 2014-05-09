#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL/SDL.h>
#include "geo.h"

#define PI 3.14159265358979323846

void cross(vec3D *a, vec3D *b, vec3D * n)
{
    n->x = a->y * b->z - a->z * b->y;
    n->y = a->z * b->x - a->x * b->z;
    n->z = a->x * b->y - a->y * b->x;
}

//multiply with a float
void mult(vec3D *a, float b, vec3D * n)
{
    n->x  = a->x * b;
    n->y  = a->y * b;
    n->z  = a->z * b;
}

void add(vec3D *a, vec3D *b, vec3D * n)
{
    n->x  = a->x + b->x;
    n->y  = a->y + b->y;
    n->z  = a->z + b->z;
}

void substract(vec3D *a, vec3D *b, vec3D * n)
{
    n->x  = a->x - b->x;
    n->y  = a->y - b->y;
    n->z  = a->z - b->z;
}

void veccopy(vec3D *a, vec3D *n)
{
    n->x  = a->x;
    n->y  = a->y;
    n->z  = a->z;
}

void rotatex(vec3D *a, vec3D *n, float angle)
{
    n->x = a->x ;
    n->y = (a->y * cos(angle)) + (a->z * sin(-angle)) ;
    n->z = (a->y * sin(angle)) + (a->z * cos(angle)) ;
}

void rotatey(vec3D *a, vec3D *n, float angle)
{
    n->x = (a->x * cos(angle)) + (a->z * sin(angle)) ;
    n->y = a->y;
    n->z = (a->x * sin(-angle)) + (a->z * cos(angle)) ;
}

void rotatez(vec3D *a, vec3D *n, float angle)
{
    n->x = (a->x * cos(angle)) + (a->y * sin(-angle)) ;
    n->y = (a->x * sin(angle)) + (a->y * cos(angle)) ;
    n->z = a->z;
}

void toUnit(vec3D *a, vec3D * n)
{
    float mag = sqrt(a->x * a->x + a->y * a->y + a->z * a->z);
    n->x = a->x / mag;
    n->y = a->y / mag;
    n->z = a->z / mag;
}

float mag(vec3D *a)
{
    return sqrt(a->x * a->x + a->y * a->y + a->z * a->z);
}

//Dot product
float dot(vec3D *a, vec3D *b)
{
    float xx = a->x * b->x;
    float yy = a->y * b->y;
    float zz = a->z * b->z;
    return xx + yy + zz;
}

light * createLight(float x, float y, float z, Uint32 c)
{
    light * n	= (light*) malloc ( sizeof(light) );

    n->color = c;

    n->pos.x = x;
    n->pos.y = y;
    n->pos.z = z;

    return n;
}

void updateSphere(sphere * s, float x, float y, float z, float r, Uint32 c, int ref, float t)
{
    s->r = r;
    s->center.x = x;
    s->center.y = y;
    s->center.z = z;
    s->color = c;
    s->ref = ref;
    s->transparency = t;
}

sphere * createSphere(float x, float y, float z, float r)
{
    sphere * n	= (sphere*) malloc ( sizeof(sphere) );

    n->r = r;
    n->center.x = x;
    n->center.y = y;
    n->center.z = z;

    return n;
}

int intersectSphere(sphere * s, vec3D * p0, vec3D * d, vec3D * q, int * status)
{
    float a;
    float b;
    float c;

    float r0;
    float r1;

    float delta;

    vec3D p0MinC;

    substract(p0,&(s->center),&p0MinC);

    a = dot(d,d);
    b = 2 * dot(d,&p0MinC);
    c = dot(&p0MinC,&p0MinC) - (s->r * s->r);

    delta = (b*b) - (4*a*c);

    if(delta < 0)
    {
        *status = 0;
    }
    else
    {
        delta = sqrt(delta);
        r0 = (-b - delta) / (2 * a);
        r1 = (-b + delta) / (2 * a);
        *status = 1;
        if(r0 > 0.5 && r0 < r1)
        {
            mult(d,r0,q);
            add(p0,q,q);
            return r0;
        }
        else if(r1 > 0.5)
        {
            mult(d,r1,q);
            add(p0,q,q);
            return r1;
        }
        else
        {
            *status = 0;
            return -1;
        }
    }
    return -1;
}

