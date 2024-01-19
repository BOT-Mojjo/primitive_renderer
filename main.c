#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define ifnt(condition) if (!condition)

#define H_RESOLUTION 120
#define V_RESOLUTION 25
#define BRIGHTNESS(depth) ".,-~:;=!*#$@"[depth > 0 ? depth : 0]
//"inspired" by famous donut script
#define EPSILON 0.000001

short FOV = 90;

typedef struct vector
{               // I have no fucking clue what axis these are
    double x;   // horisontal axis
    double y;   // vertical axis
    double z;   // depth axis
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
int ray_collision(triangle tri, vector ray_dir, vector ray_orig, double *t, double *u, double *v)
{
    // calculate edges of tri
    vector e1 = v_sub(tri.vertices[0], tri.vertices[1]);
    vector e2 = v_sub(tri.vertices[0], tri.vertices[2]);

    // don't know if dir vector is supposed to be relative to ray orig or true orig. assume ray orig.
    vector pvec = v_cross(e2, ray_dir);

    // if det is near 0 ray lies in plane of triangle?
    double det = v_dot(e1, pvec);

    if (det < EPSILON && det > -EPSILON)
        return 0;

    vector tvec = v_sub(ray_orig, tri.vertices[0]);

    *u = v_dot(tvec, pvec);
    if (*u < 0 || *u > det)
        return 0;

    vector qvec = v_cross(tvec, ray_orig);

    *v = v_dot(ray_dir, qvec);
    if (*v < 0 || *v + *u > det)
        return 0;

    *t = v_dot(e2, qvec);
    double inv_det = 1 / det;

    *t *= inv_det;
    *u *= inv_det;
    *v *= inv_det;
    return 1;
}

int main()
{
    triangle tri;
    tri.vertices[0] = (vector){-100, 50, 0};
    tri.vertices[1] = (vector){-150, 200, 0};
    tri.vertices[2] = (vector){60, 45, 0};
    double FOV_step = (double)FOV / H_RESOLUTION;
    double start_step_h = -FOV_step * (H_RESOLUTION / 2);
    double start_step_v = -FOV_step * (V_RESOLUTION / 2);
    char buffer[H_RESOLUTION + 1];
    double trash, trash1, trash2;
    for (int ii = 0; ii < V_RESOLUTION; ii++)
    {
        for (int i = 0; i < H_RESOLUTION; i++)
        {
            buffer[i] = ray_collision(tri, (vector){start_step_h + (i * FOV_step), ii - 10, -2}, (vector){0, -20, 25}, &trash, &trash1, &trash2);
            if (buffer[i] == 1)
                buffer[i] = '1';
            if (buffer[i] == 0)
                buffer[i] = '-';
        }
        buffer[H_RESOLUTION] = 0;
        printf("%s\n", buffer);
    }
    return 0;
}