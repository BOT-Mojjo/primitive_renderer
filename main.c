#include <stdio.h>
#include <stdlib.h> //delay
#include <math.h>   //sin, cos
#include <unistd.h>
#include <time.h> //clock

#define ifnt(condition) if (!condition)
#define otherwise else if

#define H_RESOLUTION 180
#define V_RESOLUTION 45
// expanded grayscale
#define BRIGHTNESS(depth) ".'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%%B@$"[depth >= 0 ? (depth < 69 ? depth : 68) : 0]
// reversed grayscale
//
// "$@B%%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'."

// simple grayscale
//  #define BRIGHTNESS(depth) ".,-~:;=!*#$@"[depth > 0 ? depth : 0]

#define EPSILON 0.000001

short FOV = 2;

typedef struct vec3
{ // I have no fucking clue what axis these are
    double x, y, z;
    // double x; // horisontal axis
    // double y; // vertical axis
    // double z; // depth axis
} vec3;

vec3 sub(vec3 v1, vec3 v2)
{
    vec3 product;
    product.x = v1.x - v2.x;
    product.y = v1.y - v2.y;
    product.z = v1.z - v2.z;
    return product;
}

vec3 cross(vec3 v1, vec3 v2)
{
    vec3 product;
    product.x = v1.y * v2.z - v1.z * v2.y;
    product.y = v1.z * v2.x - v1.x * v2.z;
    product.z = v1.x * v2.y - v1.y * v2.x;
    return product;
}

double dot(vec3 v1, vec3 v2)
{
    double product = 0;
    product += v1.x * v2.x;
    product += v1.y * v2.y;
    product += v1.z * v2.z;
    return product;
}

// general vec3 rotation matrix
// Tait-Bryan angles
vec3 rotate(vec3 point, double x_rad, double y_rad, double z_rad)
{

    // precalculate the cosine and sine of the radians
    double cos_x, cos_y, cos_z, sin_x, sin_y, sin_z;
    cos_x = cos(x_rad);
    cos_y = cos(y_rad);
    cos_z = cos(z_rad);

    sin_x = sin(x_rad);
    sin_y = sin(y_rad);
    sin_z = sin(z_rad);

    // matrix math
    double x1, x2, x3, y1, y2, y3, z1, z2, z3;
    x1 = cos_y * cos_z;
    x2 = sin_x * sin_y * cos_z - cos_x * sin_z;
    x3 = cos_x * sin_y * cos_z + sin_x * sin_z;

    y1 = cos_y * sin_z;
    y2 = sin_x * sin_y * sin_z + cos_x * cos_z;
    y3 = cos_x * sin_z * sin_z - sin_x * cos_z;

    z1 = -sin_y;
    z2 = sin_x * cos_y;
    z3 = cos_x * cos_y;

    vec3 result;
    // Multplication
    result.x = x1 * point.x + x2 * point.y + x3 * point.z;
    result.y = y1 * point.x + y2 * point.y + y3 * point.z;
    result.z = z1 * point.x + z2 * point.y + z3 * point.z;
    return result;
}

// triangle normal is a side product of ray randering step
// if you need the normal for anything do it after collision check or realise it's a frame old.
typedef struct triangle
{
    vec3 verts[3];
    vec3 normal;
} triangle;

int test(triangle *tri)
{
    printf("%p\n", tri);
    return 1;
}

// implementation of Tomas Moller & Ben Trumbore algorithm
int ray_collision(triangle *tri, vec3 ray_orig, vec3 ray_vec, vec3 *uv_out)
{
    vec3 e1 = sub(tri->verts[1], tri->verts[0]);
    vec3 e2 = sub(tri->verts[2], tri->verts[0]);
    // computes normal
    // maybe put in an if statement if it already has been before this render
    tri->normal = cross(e1, e2);
    vec3 ce2 = cross(ray_vec, e2);
    double det = dot(e1, ce2);

    // for two-sided tris
    // if (det > -EPSILON && det < EPSILON)
    //     return 0;

    // for one-sided tris
    if (det > -EPSILON)
        return 0;

    double inv_det = 1.0 / det;
    vec3 s = sub(ray_orig, tri->verts[0]);
    double u = inv_det * dot(s, ce2);
    if (u < 0 || u > 1)
        return 0;

    vec3 sce1 = cross(s, e1);
    double v = inv_det * dot(ray_vec, sce1);
    if (v < 0 || u + v > 1)
        return 0;

    double t = inv_det * dot(e2, sce1);
    uv_out->x = u;
    uv_out->y = v;
    uv_out->z = t;
    return 1;
}

void t_rotate(triangle *tri, double x_rad, double y_rad, double z_rad)
{
    for (char i = 0; i < 3; i++)
    {
        tri->verts[i] = rotate(tri->verts[i], x_rad, y_rad, z_rad);
    }
}

int main()
{
    double tan_30 = 2.5 / (tan(0.5235));
    triangle tri[12];

    tri[4].verts[0] = (vec3){5, 0, 0};
    tri[4].verts[2] = (vec3){0, 0, 0};
    tri[4].verts[1] = (vec3){2.5, tan_30, 0};

    tri[1].verts[0] = (vec3){0, 0, 0};
    tri[1].verts[2] = (vec3){5, 0, 0};
    tri[1].verts[1] = (vec3){2.5, 2.5, 5};

    tri[2].verts[0] = (vec3){5, 0, 0};
    tri[2].verts[2] = (vec3){2.5, tan_30, 0};
    tri[2].verts[1] = (vec3){2.5, 2.5, 5};

    tri[3].verts[0] = (vec3){2.5, tan_30, 0};
    tri[3].verts[2] = (vec3){0, 0, 0};
    tri[3].verts[1] = (vec3){2.5, 2.5, 5};

    triangle tri_1;
    tri_1.verts[0] = (vec3){5, 0, 0};
    tri_1.verts[1] = (vec3){0, 0, 0};
    tri_1.verts[2] = (vec3){2.5, tan_30, 0};

    double h_step = FOV / (double)H_RESOLUTION;
    double h_start = -h_step * (H_RESOLUTION / 2);
    double v_step = FOV / (double)V_RESOLUTION;
    double v_start = -v_step * (V_RESOLUTION / 2);
    char buffer[H_RESOLUTION + 1];

    for (int iii = 0; iii < 600; iii++)
    {
        clock_t start = clock();
        vec3 temp_coords, temp_coords2;
        char hit;
        for (int ii = 0; ii < V_RESOLUTION; ii++)
        {
            for (int i = 0; i < H_RESOLUTION; i++)
            {
                hit = 0;
                temp_coords2.z = 10000;
                for (char iV = 0; iV < 4; iV++)
                {
                    if (!ray_collision(&tri[iV + 1], (vec3){0, 0, 15}, (vec3){v_start + (ii * v_step), h_start + (i * h_step), -1}, &temp_coords))
                        continue;
                    buffer[i] = BRIGHTNESS((int)floor(temp_coords.z + 1));
                    hit = 1;
                }
                if (!hit)
                    buffer[i] = ' ';
            }
            buffer[H_RESOLUTION] = 0;
            printf("%s\n", buffer);
        }
        for (char iV = 0; iV < 4; iV++)
        {
            t_rotate(&tri[iV + 1], 0.03, 0.03, 0);
        }

        // printf("%f, %f, %f\n", tri.verts[0].x, tri.verts[0].y, tri.verts[0].z);
        // printf("%f, %f, %f\n", tri.verts[1].x, tri.verts[1].y, tri.verts[1].z);
        // printf("%f, %f, %f\n", tri.verts[2].x, tri.verts[2].y, tri.verts[2].z);
        // printf("%f ---------------------------------\n", 0.003 * iii);
        clock_t end = clock() - start;
        usleep(16666 - ((int)end * 10));
        // test(&tri[0]);
        // printf("%f, %f, %f\n", tri[1].normal.x, tri[1].normal.y, tri[1].normal.z);
        // printf("%f, %f, %f\n", tri[2].normal.x, tri[2].normal.y, tri[2].normal.z);
        // printf("%f, %f, %f\n", tri[3].normal.x, tri[3].normal.y, tri[3].normal.z);
        // printf("%f, %f, %f\n", tri[4].normal.x, tri[4].normal.y, tri[4].normal.z);
        // printf("%p, %p, %f, %f\n", &tri_1, &tri[0], (&tri[0])->normal.y, tri[0].normal.x);
        // printf("%d, %d, \n", sizeof(triangle), sizeof(vec3));
    }
    return 0;
}