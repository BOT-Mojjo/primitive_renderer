#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>




#define ifnt(condition) if (!condition)

#define H_RESOLUTION 80
#define V_RESOLUTION 80
#define BRIGHTNESS(depth) ".,-~:;=!*#$@"[depth > 0 ? depth : 0]
//"inspired" by famous donut script
#define EPSILON 0.000001

typedef struct vector
{
    double x; // horisontal axis
    double y; // vertical axis
    double z; // depth axis
} vector;

vector v_sub(vector v1, vector v2)
{
    vector product;
    product.x = v1.x - v2.x;
    product.y = v1.y - v2.y;
    product.z = v1.z - v2.z;
    return product;
}

vector v_cross(vector v1, vector v2)
{
    vector product;
    product.x = v1.y * v2.z - v1.z * v2.y;
    product.y = v1.z * v2.x - v1.x * v2.z;
    product.z = v1.x * v2.y - v1.y * v2.x;
    return product;
}

double v_dot(vector v1, vector v2)
{
    double product = 0;
    product += v1.x * v2.x;
    product += v1.y * v2.y;
    product += v1.z * v2.z;
    return product;
}

typedef struct triangle
{
    vector vertices[3];
    vector normal;
} triangle;

// implementation of Tomas Moller & Ben Trumbore algorithm
int ray_collision(triangle tri, vector ray_dir, vector ray_orig, double* t, double* u, double* v)
{
    // calculate edges of tri
    vector e1 = v_sub(tri.vertices[1], tri.vertices[0]); 
    vector e2 = v_sub(tri.vertices[2], tri.vertices[0]);

    //don't know if dir vector is supposed to be relative to ray orig or true orig. assume ray orig.
    vector pvec = v_cross(ray_dir, e2);

    //if det is near 0 ray lies in plane of triangle?
    double det = v_dot(e1, pvec);

    if (det < EPSILON) return 0;

    vector tvec = v_sub(ray_orig, tri.vertices[0]);

    *u = v_dot(tvec, pvec);
    if(*u < 0 || *u > det) return 0;

    vector qvec = v_cross(ray_orig, tvec);

    *v = v_dot(ray_dir, qvec);
    if(*v < 0 || *v + *u > det) return 0;

    *t = v_dot(e2, qvec);
    double inv_det = 1 / det;

    *t *= inv_det;
    *u *= inv_det;
    *v *= inv_det;
    return 1;
}

int main()
{
    ifnt(0) printf("%s\n", "hej");

    return 0;
}