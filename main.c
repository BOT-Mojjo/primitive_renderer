#include <stdio.h> // printf
#include <stdlib.h>
#include <math.h>    // sin, cos, tan, acos
#include <unistd.h>  // usleep
#include <time.h>    // clock
#include <termios.h> // terminal fuckery

#define ifnt(condition) if (!condition)
#define otherwise else if

#define H_RESOLUTION 150
#define V_RESOLUTION 50
// expanded grayscale
// #define BRIGHTNESS(depth) "@&%%QWNM0gB$#DR8mHXKAUbGOpV4d9h6PkqwSE2]ayjxY5Zoen[ult13If}C{iF|(7J)vTLs?z/*cr!+<>;=^,_:'-.`"[depth >= 0 ? (depth < 92 ? depth : 91) : 0]
// #define BRIGHTNESS(depth) "$@B%%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'."[depth >= 0 ? (depth < 69 ? depth : 68) : 0]
#define BRIGHTNESS(depth) "@$#*!=;:~-,."[depth >= 0 ? (depth < 12 ? depth : 11) : 0]
// #define BRIGHTNESS(depth) "#O-."[depth >= 0 ? (depth < 4 ? depth : 3) : 0]
// reversed grayscale
// ".'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%%B@$"

// meny string
char menu[] = "+-------------------------------+\n| h=toggle menu |r=reset mesh   |\n| q=quit        |a=animate      |\n| 8=rotate up   |2=rotate down  |\n| 4=rotate left |6=rotate right |\n| 7=roll left   |9=roll left    |\n| 1=inc. step s.|3=dec. step s. |\n| +=inc. mesh s.|-=dec. step s. |\n+-------------------------------+\n";
char str_buffer[128];
struct termios orig_termios;

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
    // Precalculate the cosine and sine of the radians
    double cos_x, cos_y, cos_z, sin_x, sin_y, sin_z;
    cos_x = cos(x_rad);
    cos_y = cos(y_rad);
    cos_z = cos(z_rad);

    sin_x = sin(x_rad);
    sin_y = sin(y_rad);
    sin_z = sin(z_rad);

    // Matrix math
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

// will break if the string isn't terminated
// with a NL or 0
void str_clean(char *str, int limit)
{
    for (int i = 0; i < limit; i++)
    {
        if (str[i] != 10 && str[i] != 0)
        {
            continue;
        }
        str[i] = 0;
        break;
    }
}

int load_obj(char *path, triangle **mesh_out, short *mesh_size)
{
    short vec_amount = 0, tri_amount = 0;
    FILE *file = fopen(path, "r");
    if (file == NULL)
        return 0;

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

    // file parser
    while (fgets(str_buffer, 128, file))
    {
        if (str_buffer[0] == 'v' && str_buffer[1] == ' ')
        {
            vec3 output;
            char offset = 2;
            for (char i = 0; i < 3; i++)
            { // God bless pointermath. writing to the x, y, & z components without having to specify which one
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

triangle *mesh = 0;

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

void mesh_scale(triangle *mesh_in, triangle *mesh_out, int mesh_size, double scalar)
{
    for (int i = 0; i < mesh_size * 3; i++)
    {
        mesh_out[i / 3].verts[i % 3] = scale(mesh_in[i / 3].verts[i % 3], scalar);
    }
}

#include "shapes.c" // cube, tetrahedron
int main()
{
    short poly_count;
    int input = 0;
    int anim_timer = 0;
    puts("Please input path for .obj file:");
    char path[128];
    fgets(path, 128, stdin);
    str_clean(path, 128);
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

    char render_menu = 0;

    triangle render_mesh[poly_count];
    mesh_scale(mesh, mesh, poly_count, 4);

    vec3 light_vec3 = (vec3){0.5, 0, -0.5};
    vec3 rotation = (vec3){0, 0, 0};
    vec3 anim_rotation = (vec3){0, 0, 0};
    double step_size = 0.01;

    int iii = 0;
    double depth;

    while (1)
    {
        clock_t start = clock();
        vec3 temp_coords;
        char hit;
        double light_level;

        // rotate the mesh and apply it to a duplicate to prevente
        // accumilative mutation of the original from building up
        mesh_rotate(&mesh, render_mesh, rotation.x, rotation.y, rotation.z, poly_count);

        for (int ii = 0; ii < V_RESOLUTION; ii++)
        {
            for (int i = 0; i < H_RESOLUTION; i++)
            {
                hit = 0;
                depth = 10000;
                for (int iV = 0; iV < poly_count; iV++)
                {
                    if (!ray_collision(render_mesh[iV], (vec3){0, 0, 12}, (vec3){(h_start + (i * h_step)) * 1.4, v_start + (ii * v_step), -1}, &temp_coords))
                        continue;
                    if (temp_coords.z > depth)
                        continue;
                    depth = temp_coords.z;
                    light_level = acos(dot(light_vec3, render_mesh[iV].normal) / (mag(light_vec3) * mag(render_mesh[iV].normal)));
                    buffer[ii][i] = BRIGHTNESS((int)floor((light_level * 9) / 1.4));
                    hit = 1;
                }
                if (!hit)
                    buffer[ii][i] = ' ';
            }
            buffer[ii][H_RESOLUTION] = 0;
        }

        // menu logic
        if (render_menu)
        {
            for (int i = 0; i < 9; i++)
            {
                for (int ii = 0; ii < 33; ii++)
                {
                    buffer[i][ii] = menu[ii + ((8 - i) * 34)];
                }
            }
        }

        clock_t end = clock() - start;
        if ((int)end < 16666)
            usleep(16666 - end);
        for (int i = V_RESOLUTION - 1; i >= 0; i--)
        {
            printf("%s\n", buffer[i]);
        }

        iii++;
        if (anim_timer > 0)
        {
            anim_timer--;
            rotation.x += anim_rotation.x;
            rotation.y += anim_rotation.y;
            rotation.z += anim_rotation.z;
        }
        else
            input = fgetc(stdin);
        switch (input)
        {
        // X axis:
        case 56: // up/8
            rotation.x -= step_size;
            break;
        case 50: // down/2
            rotation.x += step_size;
            break;
        // Y axis:
        case 52: // left/4
            rotation.y -= step_size;
            break;
        case 54: // right/6
            rotation.y += step_size;
            break;
        // Z axis:
        case 55: // rotate left/7
            rotation.z += step_size;
            break;
        case 57: // rotate right/9
            rotation.z -= step_size;
            break;
        // Step size
        case 49: // step down/1
            step_size -= 0.01;
            step_size = step_size < 0 ? 0 : step_size;
            break;
        case 51: // step up/3
            step_size += 0.01;
            break;
        // zoom? mesh size
        case 45: // scale up
            mesh_scale(mesh, mesh, poly_count, 1 / 1.1);
            break;
        case 43: // scale down
            mesh_scale(mesh, mesh, poly_count, 1.1);
            break;
        case 'r':
            rotation = (vec3){0, 0, 0};
            free(mesh);
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
            mesh_scale(mesh, mesh, poly_count, 4);
            break;
        case 'h':
            render_menu = !render_menu;
            break;
        case 'a':
            restore_term();
            printf("%s\n", "Rotation is measured in radians.");
            printf("%s", "X rotation per frame: ");
            str_clean(fgets(str_buffer, 128, stdin), 128);
            anim_rotation.x = strtod(str_buffer, (void *)0);
            printf("%s", "Y rotation per frame: ");
            str_clean(fgets(str_buffer, 128, stdin), 128);
            anim_rotation.y = strtod(str_buffer, (void *)0);
            printf("%s", "Z rotation per frame: ");
            str_clean(fgets(str_buffer, 128, stdin), 128);
            anim_rotation.z = strtod(str_buffer, (void *)0);
            printf("%s", "Amount of frames: ");
            str_clean(fgets(str_buffer, 128, stdin), 128);
            anim_timer = atoi(str_buffer);
            set_raw_term();
            input = 0;
            break;
        default:
            break;
        }
        if (input == 'q')
            break;
    }
    free(mesh);
    return 0;
}
