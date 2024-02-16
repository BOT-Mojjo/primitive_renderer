#include <stdio.h> // printf
#include <stdlib.h>
#include <math.h>    // sin, cos, tan, acos
#include <unistd.h>  // usleep
#include <time.h>    // clock
#include <termios.h> // terminal fuckery

#define ifnt(condition) if (!condition)
#define otherwise else if

#define H_RESOLUTION 180
#define V_RESOLUTION 60
// expanded grayscale
#define BRIGHTNESS(depth) "$@B%%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'."[depth >= 0 ? (depth < 69 ? depth : 68) : 0]
// reversed grayscale
// ".'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%%B@$"
//

// simple grayscale
//  #define BRIGHTNESS(depth) ".,-~:;=!*#$@"[depth > 0 ? depth : 0]

struct termios orig_termios;

void restore_term()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void set_raw_term()
{ // saves local variables before changing them, and changeing them back at program termination
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(restore_term);
    // changing them to not exho text input
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

#define EPSILON 0.000001

// program variables

char FOV = 1;

typedef struct vec3
{ // I have no fucking clue what axis these are. might aswell be called a b c
    double x, y, z;
} vec3;

vec3 sub(vec3 v0, vec3 v1)
{
    vec3 product;
    product.x = v0.x - v1.x;
    product.y = v0.y - v1.y;
    product.z = v0.z - v1.z;
    return product;
}

vec3 cross(vec3 v0, vec3 v1)
{
    vec3 product;
    product.x = v0.y * v1.z - v0.z * v1.y;
    product.y = v0.z * v1.x - v0.x * v1.z;
    product.z = v0.x * v1.y - v0.y * v1.x;
    return product;
}

vec3 scale(vec3 v0, double scalar)
{
    return (vec3){v0.x * scalar, v0.y * scalar, v0.z * scalar};
}

double dot(vec3 v0, vec3 v1)
{
    double product = 0;
    product += v0.x * v1.x;
    product += v0.y * v1.y;
    product += v0.z * v1.z;
    return product;
}

double mag(vec3 v0)
{
    return sqrt(v0.x * v0.x + v0.y * v0.y + v0.z * v0.z);
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
    y3 = cos_x * sin_y * sin_z - sin_x * cos_z;

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
// if you need the normal for anything do it after the rendering math or realise it's a frame old.
typedef struct triangle
{
    vec3 verts[3];
    vec3 normal;
} triangle;

// implementation of Tomas Moller & Ben Trumbore algorithm
int ray_collision(triangle tri, vec3 ray_orig, vec3 ray_vec, vec3 *uv_out)
{
    vec3 e1 = sub(tri.verts[1], tri.verts[0]);
    vec3 e2 = sub(tri.verts[2], tri.verts[0]);
    vec3 ce2 = cross(ray_vec, e2);
    double det = dot(e1, ce2);

    // for two-sided tris
    // if (det > -EPSILON && det < EPSILON)
    //     return 0;

    // for one-sided tris
    if (det < EPSILON)
        return 0;

    double inv_det = 1.0 / det;
    vec3 s = sub(ray_orig, tri.verts[0]);
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

// returns new triangle to not mutate orignal, does mutate originals normal vector
void mesh_rotate(triangle **mesh_in, triangle *mesh_out, double x_rad, double y_rad, double z_rad, int mesh_size)
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
    y3 = cos_x * sin_y * sin_z - sin_x * cos_z;

    z1 = -sin_y;
    z2 = sin_x * cos_y;
    z3 = cos_x * cos_y;

    // Multplication
    for (int ii = 0; ii < mesh_size; ii++)
    {
        for (char i = 0; i < 3; i++)
        {
            mesh_out[ii].verts[i].x = x1 * (*mesh_in + ii)->verts[i].x + x2 * (*mesh_in + ii)->verts[i].y + x3 * (*mesh_in + ii)->verts[i].z;
            mesh_out[ii].verts[i].y = y1 * (*mesh_in + ii)->verts[i].x + y2 * (*mesh_in + ii)->verts[i].y + y3 * (*mesh_in + ii)->verts[i].z;
            mesh_out[ii].verts[i].z = z1 * (*mesh_in + ii)->verts[i].x + z2 * (*mesh_in + ii)->verts[i].y + z3 * (*mesh_in + ii)->verts[i].z;
        }

        vec3 e0 = sub(mesh_out[ii].verts[1], mesh_out[ii].verts[0]);
        vec3 e1 = sub(mesh_out[ii].verts[2], mesh_out[ii].verts[0]);
        mesh_out[ii].normal = cross(e1, e0);
    }
}

int load_obj(char *path, triangle **mesh_out, short *mesh_size)
{
    short vec_amount = 0, tri_amount = 0;
    FILE *file = fopen(path, "r");
    if (file == NULL)
        return 0;

    char str_buffer[128];

    // cant be bothered making a dynamic array
    // so count the triangles and verticies beforehand
    while (!feof(file))
    {
        fgets(str_buffer, 128, file);
        if (str_buffer[0] == 'v' && str_buffer[1] == ' ')
            vec_amount++;
        otherwise(str_buffer[0] == 'f' && str_buffer[1] == ' ')
        {
            tri_amount++;
        }
    }
    rewind(file);

    if (vec_amount < 1 || tri_amount < 1)
        return 2;

    vec3 vec_buffer[vec_amount];
    *mesh_out = (triangle *)malloc(sizeof(triangle) * tri_amount);
    if (*mesh_out == NULL)
        return 3;
    *mesh_size = tri_amount;
    vec_amount = 0, tri_amount = 0;
    while (fgets(str_buffer, 128, file))
    {
        if (str_buffer[0] == 'v' && str_buffer[1] == ' ')
        {
            vec3 output;
            char offset = 2;
            for (char i = 0; i < 3; i++)
            {
                *((double *)&output + i) = strtod(&str_buffer[offset + 9 * i], (void *)0);
                if (str_buffer[offset + (9 * i)] == '-')
                    offset++;
            }
            vec_buffer[vec_amount] = output;
            vec_amount++;
        }
        otherwise(str_buffer[0] == 'f' && str_buffer[1] == ' ')
        {
            int cur_char = 0;
            triangle tri;
            for (char i = 0; i < 3; i++)
            {
                while (str_buffer[cur_char] != ' ')
                {
                    cur_char++;
                }
                tri.verts[i] = vec_buffer[atoi(&str_buffer[cur_char + 1]) - 1];
                cur_char++;
            }
            *(*mesh_out + tri_amount) = tri;
            tri_amount++;
        }
    }
    fclose(file);
    return 1;
}

#include "shapes.c" // cube, tetrahedron
int main()
{
    short poly_count;
    triangle *mesh = 0;
    puts("Please input path for .obj file:");
    char path[128];
    fgets(path, 128, stdin);
    for (unsigned char i = 0; i < 128; i++)
    {
        if (path[i] != 10)
            continue;
        path[i] = 0;
        break;
    }

    set_raw_term();

    printf("\nLoading object %s\n", path);
    char load_failure = 0;
    switch (load_obj(path, &mesh, &poly_count))
    {
    case 0:
        puts("File not Found.");
        load_failure = 1;
        break;
    case 2:
        puts("File not supported.");
        load_failure = 1;
        break;
    case 3:
        puts("Memory Allocation failed.");
        load_failure = 1;
        break;
    default:
        printf("Object %s loaded succesfully\n", path);
    }
    if (load_failure)
        return 0;

    double h_step = FOV / (double)H_RESOLUTION;
    double h_start = -h_step * (H_RESOLUTION / 2);
    double v_step = FOV / (double)V_RESOLUTION;
    double v_start = -v_step * (V_RESOLUTION / 2);
    char buffer[V_RESOLUTION][H_RESOLUTION + 1];

    for (int i = 0; i < poly_count * 3; i++)
    {
        mesh[i / 3].verts[i % 3] = scale(mesh[i / 3].verts[i % 3], 5);
    }
    triangle render_mesh[poly_count];

    vec3 light_vec3 = (vec3){0.5, 0, -0.5};
    vec3 rotation;

    int iii = 0;
    double depth;
    // for (int iii = 0; iii < 600; iii++)
    while (1)
    {
        clock_t start = clock();
        vec3 temp_coords;
        char hit;
        double light_level;

        mesh_rotate(&mesh, render_mesh, 0 * iii, 0.05 * iii, 0 * iii, poly_count);

        for (int ii = 0; ii < V_RESOLUTION; ii++)
        {
            for (int i = 0; i < H_RESOLUTION; i++)
            {
                hit = 0;
                depth = 10000;
                for (short iV = 0; iV < poly_count; iV++)
                { // rotate the input on render-step to reduce
                  // accumalitive deformation of mesh from rotation
                    if (!ray_collision(render_mesh[iV], (vec3){0, 0, 12}, (vec3){(h_start + (i * h_step)) * 1.4, v_start + (ii * v_step), -1}, &temp_coords))
                        continue;
                    if (temp_coords.z > depth)
                        continue;
                    depth = temp_coords.z;
                    light_level = acos(dot(light_vec3, render_mesh[iV].normal) / (mag(light_vec3) * mag(render_mesh[iV].normal)));
                    buffer[ii][i] = BRIGHTNESS((int)floor((light_level * 57.3) / 1.4));
                    hit = 1;
                }
                if (!hit)
                    buffer[ii][i] = ' ';
            }
            buffer[ii][H_RESOLUTION] = 0;
        }
        clock_t end = clock() - start;
        for (int i = V_RESOLUTION - 1; i >= 0; i--)
        {
            printf("%s\n", buffer[i]);
        }
        if ((int)end < 16666)
            usleep(16666 - end);
        iii++;
    }
    free(mesh);
    return 0;
}