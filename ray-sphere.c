#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include "SDL.h"

#include "geo.h"
#define ITERATEMAX 3
#define REFLECT 3

#define PI 3.14159265358979323846


typedef struct {
    sphere spheres[10];
    int sphereCount;
    light * l;
    SDL_Surface * screen ;
    SDL_Texture * texture ;
    SDL_Window * window ;
    SDL_Renderer * renderer ;

    vec3D camera_pos;
    vec3D camera_dir;
    vec3D camera_up;
    float x_rot;
    float y_rot;
} renderer_t;


unsigned int SCREEN_BPP = 32;
int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 800;
int RENDERRES  = 300; //Internal resolution
int WINDOWSIZE  = 300; //Internal resolution

int done = 0;

Uint32 phong(renderer_t * rend, vec3D * p, vec3D * q, int sp, vec3D * n, int status)
{
    int R,G,B;
    int Rd,Gd,Bd;
    int Ra,Ga,Ba;
    float Rs,Gs,Bs;
    vec3D temp;
    vec3D v;
    vec3D lt;
    vec3D r;
    float d;
    float s;

    v.x = 0;
    v.y = 0;
    v.z = 0;

    veccopy(p, &v);

    //calculate n and diffuse light
    substract(q,&(rend->spheres[sp].center), n);
    toUnit(n,n);

    substract(&rend->l->pos,q, &temp);
    toUnit(&temp,&temp);
    d = dot(&temp,n);
    if(d < 0) d = 0;

    Rd = (rend->l->color >> 16) * (rend->spheres[sp].color >> 16) / 255;
    Gd = ((rend->l->color >> 8) % 256) * ((rend->spheres[sp].color >> 8) % 256) / 255;
    Bd = (rend->l->color % 256) * (rend->spheres[sp].color % 256) / 255;
    Rd = Rd * d;
    Gd = Gd * d;
    Bd = Bd * d;

    //ambient light
    Ra = (rend->l->color >> 16) * (rend->spheres[sp].color >> 16) / 255;
    Ga = ((rend->l->color >> 8) % 256) * ((rend->spheres[sp].color >> 8) % 256) / 255;
    Ba = (rend->l->color % 256) * (rend->spheres[sp].color % 256) / 255;

    //fixed ambient property:
    Ra /= 10;
    Ga /= 10;
    Ba /= 10;

    //fixed specular color
    Rs = (rend->l->color >> 16) % 256;
    Gs = (rend->l->color >> 8) % 256;
    Bs = rend->l->color % 256;


    substract(&v,q,&v);
    toUnit(&v, &v);

    substract(&rend->l->pos,q,&lt);
    toUnit(&lt, &lt);

    mult(n, 2 * dot(n,&lt), &r);
    substract(&r,&lt,&r);
    toUnit(&r, &r);

    s = powf(dot(&r, &v), 15);
    if(s < 0) s = 0;

    Rs *= s;
    Gs *= s;
    Bs *= s;

    if(status == 1) //cast shadow
    {
        Rd /= 10;
        Gd /= 10;
        Bd /= 10;

        Rs /= 10;
        Gs /= 10;
        Bs /= 10;
    }

    R =  Ra + Rd + round(Rs);
    G =  Ga + Gd + round(Gs);
    B =  Ba + Bd + round(Bs);
    if (R > 255) R = 255;
    if (G > 255) G = 255;
    if (B > 255) B = 255;

    return SDL_MapRGB(rend->screen->format,R,G,B);


}

void closestQ(renderer_t * rend, vec3D *p, vec3D *d, int * sp, vec3D * qMin, int * stat, int ignore, float dl)
{
    int i;
    vec3D q;
    vec3D unitd;
    int temp;
    int min = -1;
    int status;
    *stat = 0;
    toUnit(d,&unitd);

    for (i = 0; i < rend->sphereCount; i++) 
    {
        if(i == ignore)
            continue;
        temp = intersectSphere(&rend->spheres[i],p,&unitd, &q, &status);
        if(status == 1 && (temp < min || min == -1))
        {
            if(dl)
            {
                if(temp > dl) continue;
            }
            *stat = 1;
            min = temp;
            *qMin = q;
            *sp = i;
            if(dl) break;
        }
    }
}

void paintPix(renderer_t * rend, int x, int y, Uint32 c)
{
    Uint32 *pixPtr;
    pixPtr = (Uint32*) rend->screen->pixels  + rend->screen->pitch/4 * y;
    pixPtr[x] = c;
}

Uint32 trace(renderer_t * rend, vec3D *p, vec3D *d, int step, int ignore)
{
    Uint16 R,G,B;
    Uint32 local = 0;
    Uint32 reflected = 0;
    Uint32 transmitted = 0;
    int status;
    vec3D q;
    vec3D feelerI;
    vec3D feelerD;
    vec3D n;
    vec3D r;
    vec3D tmp;
    vec3D unitd;
    int sp;
    int dummy;
    float dl;

    toUnit(d,&unitd);

    //max number of iterations
    if(step > ITERATEMAX)
        return 0;

    //get closest intersection point
    closestQ(rend, p,d, &sp, &q, &status, ignore, 0);

    //if no intersection
    if(status == 0)
        return 0;

    substract(&rend->l->pos, &q, &feelerD);
    dl = mag(&feelerD);
    toUnit(&feelerD,&feelerD);

    //get closest intersection point between light and q
    closestQ(rend, &q,&feelerD,&dummy, &feelerI, &status, ignore, dl);

    //calculate local color
    local = phong(rend, p, &q,sp, &n, status);

    //calculate reflection vector
    substract(&q, p, &tmp);
    toUnit(&tmp, &tmp);

    mult(&n, 2 * dot(&tmp,&n), &r);
    substract(&tmp,&r,&r);
    /*mult(&r, -1, &r);*/
    toUnit(&r, &r);

    reflected = trace(rend, &q, &r, step + 1, sp);
    reflected = SDL_MapRGB(rend->screen->format, (reflected >> 16)/rend->spheres[sp].ref, ((reflected >> 8) % 256)/rend->spheres[sp].ref, (reflected % 256)/rend->spheres[sp].ref ) ;

    if(rend->spheres[sp].transparency > 0)
    {
        transmitted = trace(rend, &q, d, step + 1, -1);
        transmitted = SDL_MapRGB(rend->screen->format, (transmitted >> 16)*rend->spheres[sp].transparency, ((transmitted >> 8) % 256)*rend->spheres[sp].transparency, (transmitted % 256)*rend->spheres[sp].transparency ) ;
    }

    R = ((reflected >> 16) + (local >> 16)) * (1.0 - rend->spheres[sp].transparency) + (transmitted >> 16);
    G = (((reflected >> 8) % 256) + ((local >> 8) % 256)) * (1.0 - rend->spheres[sp].transparency) + ((transmitted >> 8) % 256) ;
    B = (((reflected) % 256) + ((local  % 256))) * (1.0 - rend->spheres[sp].transparency) + ((transmitted  % 256)) ;

    if (R > 255) R = 255;
    if (G > 255) G = 255;
    if (B > 255) B = 255;

    return SDL_MapRGB(rend->screen->format,R,G,B);
}


void traceRays(renderer_t * rend)
{
    int x,y;
    /*float xStep;*/
    /*float yStep;*/
    vec3D p1;
    Uint32 c;

    vec3D left;
    vec3D up;

    vec3D right_step;
    vec3D down_step;

    vec3D corner;

    /*cross(&rend->camera_up, &rend->camera_dir, &left);*/
    /*cross(&rend->camera_dir, &left, &up);*/

    cross(&rend->camera_dir, &rend->camera_up, &left);
    cross(&left, &rend->camera_dir,  &up);

    toUnit(&left, &left);
    toUnit(&up, &up);

    add(&left, &up, &corner);
    add(&corner, &rend->camera_dir, &corner);

    mult(&left, -2.0 / RENDERRES, &right_step);
    mult(&up,   -2.0 / RENDERRES, &down_step);

    veccopy(&corner, &p1);
    for (y = 0; y < rend->screen->h; y++) 
    {

        add(&p1, &down_step, &p1);

        for (x = 0; x < rend->screen->w; x++) 
        {
            add(&p1, &right_step, &p1);
            c = trace(rend, &rend->camera_pos, &p1, 0, -1);

            paintPix(rend, x,y,c);
        }
        add(&p1, &left, &p1);
        add(&p1, &left, &p1);
    }
        /*printf("##########\n");*/
        /*printf("x:%f\n", p1.x);*/
        /*printf("y:%f\n", p1.y);*/
        /*printf("z:%f\n", p1.z);*/
        /*printf("##########\n");*/
}


void events(renderer_t * rend)
{
    vec3D dir;
    int x, y;
    int xdist;
    int ydist;

    dir.x = 0;
    dir.y = 0;
    dir.z = 1;

    SDL_GetMouseState(&x, &y);
    xdist = (SCREEN_WIDTH/2) - x;
    ydist = (SCREEN_HEIGHT/2) - y;

    static SDL_Event event;
    while( SDL_PollEvent( &event ) ) {
        switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        done = 1;
                        break;
                    case SDLK_s:
                        substract(&rend->camera_pos, &rend->camera_dir , &rend->camera_pos);
                        break;
                    case SDLK_w:
                        add(&rend->camera_pos, &rend->camera_dir , &rend->camera_pos);
                        break;
                    default:
                        break;
                }
                break;
            case SDL_QUIT:
                done = 1;
                break;
        }
    }
    /*rotatey(&rend->camera_dir, &rend->camera_dir, xdist * (-0.00017));*/
    /*rotatex(&rend->camera_dir, &rend->camera_dir, ydist * (-0.00017));*/
    /*toUnit(&rend->camera_dir, &rend->camera_dir);*/

    rend->x_rot += xdist * (-0.00017);
    rend->y_rot += ydist * (-0.00017);

    if(rend->y_rot > (PI / 4))
        rend->y_rot = PI / 4;

    if(rend->y_rot < (-PI / 4))
        rend->y_rot = -PI / 4;


    veccopy(&dir, &rend->camera_dir);

    rotatex(&rend->camera_dir, &rend->camera_dir, rend->y_rot);
    rotatey(&rend->camera_dir, &rend->camera_dir, rend->x_rot);

    toUnit(&rend->camera_dir, &rend->camera_dir);
}

void init(renderer_t * rend, int fullscreen)
{
    SDL_Init(SDL_INIT_VIDEO);
    if(fullscreen)
    {
        SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP, &rend->window, &rend->renderer);
    }
    else
    {
        SDL_CreateWindowAndRenderer(WINDOWSIZE, WINDOWSIZE, 0, &rend->window, &rend->renderer);
        SCREEN_WIDTH = WINDOWSIZE;
        SCREEN_HEIGHT = WINDOWSIZE;
    }
    assert(rend->window != NULL);
    assert(rend->renderer != NULL);
    rend->screen = SDL_CreateRGBSurface(0, RENDERRES, RENDERRES, SCREEN_BPP, 0,0,0,0);
    assert(rend->screen != NULL);

    if(fullscreen)
    {
        SDL_GetWindowSize(rend->window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
    }

    rend->texture = SDL_CreateTexture(rend->renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            RENDERRES, RENDERRES);

    rend->camera_pos.x = 0;
    rend->camera_pos.y = 0;
    rend->camera_pos.z = 0;

    rend->camera_dir.x = 0;
    rend->camera_dir.y = 0;
    rend->camera_dir.z = 1;

    rend->camera_up.x = 0;
    rend->camera_up.y = 1;
    rend->camera_up.z = 0;

    rend->y_rot = 0;
    rend->x_rot = 0;

    /*rend->l = createLight(+0,10,50,SDL_MapRGB(screen->format, 255, 255, 255));*/
    //create spheres
    rend->l = createLight(+5,5,0,SDL_MapRGB(rend->screen->format, 150, 150, 150));
    updateSphere(&rend->spheres[0],   -20,    -15,    45,     5,   SDL_MapRGB(rend->screen->format,   255,   128,   128),   REFLECT,     0);
    updateSphere(&rend->spheres[1],    15,    -10,    35,     5,   SDL_MapRGB(rend->screen->format,   100,   100,   100),   6,   0.6);
    updateSphere(&rend->spheres[2],     0,    -10,    25,     5,   SDL_MapRGB(rend->screen->format,   128,   128,   255),   REFLECT,     0);
    updateSphere(&rend->spheres[3],     0,   -120,    55,   100,   SDL_MapRGB(rend->screen->format,   255,   255,   255),   REFLECT,     0);
    updateSphere(&rend->spheres[4],     0,    120,    55,   100,   SDL_MapRGB(rend->screen->format,   255,   255,     0),   REFLECT,     0);
    updateSphere(&rend->spheres[5],     0,      0,   200,   100,   SDL_MapRGB(rend->screen->format,   255,   128,   255),   REFLECT,     0);
    updateSphere(&rend->spheres[6],    15,      0,    40,     5,   SDL_MapRGB(rend->screen->format,     0,     0,     0),         1,     0);
    updateSphere(&rend->spheres[7],   -45,      0,    45,    20,   SDL_MapRGB(rend->screen->format,     30,     30,     30),         1,     0);
    rend->sphereCount = 8;
}

int main(int argc, char *argv[])
{
    int t0, t1;
    int render_time;
    int opt;
    float angle = 0;
    int dir = 0;
    SDL_Rect texture_dst;
    SDL_Rect render_dst;
    renderer_t rend;
    int fullscreen = 1;

    while ((opt = getopt(argc, argv, "w:r:")) != -1) {
        switch (opt) {
            case 'w':
                fullscreen = 0;
                WINDOWSIZE = atoi(optarg);
                break;
            case 'r':
                RENDERRES = atoi(optarg);
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-r surface_resolution] [-w window_size]\n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    init(&rend, fullscreen);

    texture_dst.x = 0;
    texture_dst.y = 0;
    texture_dst.w = RENDERRES;
    texture_dst.h = RENDERRES;

    render_dst.x = (SCREEN_WIDTH - SCREEN_HEIGHT) / 2;
    render_dst.y = 0;
    render_dst.w = SCREEN_HEIGHT;
    render_dst.h = SCREEN_HEIGHT;

    while(1)
    {
        t0 = SDL_GetTicks();
        /*SDL_LockSurface(screen);*/

        SDL_SetRenderDrawColor(rend.renderer, 0, 0, 0, 255);
        SDL_RenderClear(rend.renderer);
        traceRays(&rend);


        SDL_UpdateTexture(rend.texture, &texture_dst, rend.screen->pixels, rend.screen->pitch);
        SDL_RenderCopyEx(rend.renderer, rend.texture, NULL, &render_dst, angle, NULL, 0);
        SDL_RenderPresent(rend.renderer);
        /*angle++;*/
        events(&rend);
        if(done)
            break;

        if(dir)
        {
            rend.l->pos.x -= 1;
            if (rend.l->pos.x < 1)
                dir = 0;
        }
        else
        {
            rend.l->pos.x += 1;
            if (rend.l->pos.x > 10)
                dir = 1;
        }
        /*veccopy(&rend.camera_pos, &rend.l->pos);*/
        /*rend.l->pos.y += 5;*/

        /*rotatex(&rend.camera_dir, &rend.camera_dir, 0.017);*/
        /*rotatey(&rend.camera_dir, &rend.camera_dir, 0.017);*/
        /*rotatez(&rend.camera_dir, &rend.camera_dir, 0.017);*/
        /*toUnit(&rend.camera_dir, &rend.camera_dir);*/
        t1 = SDL_GetTicks();
        render_time = t1 - t0;
        if(render_time < (1000 / 30)) //limit to 30fps
            SDL_Delay((1000 / 30) - render_time);
    }
    free(rend.l);
    SDL_FreeSurface(rend.screen);
    SDL_DestroyTexture(rend.texture);
    SDL_DestroyRenderer(rend.renderer);
    SDL_DestroyWindow(rend.window);
    SDL_Quit();

    return 0;
}
