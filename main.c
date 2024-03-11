#include <stdio.h> // printf, fopen, fclose, fgets, feof, fgetc
#include <stdlib.h>
#include <math.h>    // sin, cos, tan, acos
#include <unistd.h>  // usleep
#include <time.h>    // clock
#include <termios.h> // terminal configuration, raw terminal

#define ifnt(condition) if (!condition)
#define otherwise else if
#define otherwisent(condition) otherwise(!condition)

#define H_RESOLUTION 150
#define V_RESOLUTION 50

// expanded grayscale
// #define BRIGHTNESS(depth) "@&%%QWNM0gB$#DR8mHXKAUbGOpV4d9h6PkqwSE2]ayjxY5Zoen[ult13If}C{iF|(7J)vTLs?z/*cr!+<>;=^,_:'-.`"[depth >= 0 ? (depth < 92 ? depth : 91) : 0]
// #define BRIGHTNESS(depth) "$@B%%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'."[depth >= 0 ? (depth < 69 ? depth : 68) : 0]
#define BRIGHTNESS(depth) (depth >= 0 ? (depth < 12 ? depth : 11) : 0)["@$#*!=;:~-,."]
// #define BRIGHTNESS(depth) "#O-."[depth >= 0 ? (depth < 4 ? depth : 3) : 0]
// reversed grayscale
// ".'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%%B@$"

char grayscale[256] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 7, 8, 9, 9, 11, 12, 13, 14, 15, 16, 18, 19, 21, 22, 24, 25, 27, 28, 30, 31, 32, 34, 35, 37, 38, 39, 41, 42, 43, 45, 46, 47, 49, 50, 51, 53, 54, 55, 57, 58, 59, 60, 62, 63, 64, 65, 67, 68, 69, 70, 71, 73, 74, 75, 76, 77, 79, 80, 81, 82, 83, 84, 86, 87, 88, 89, 90, 91, 93, 94, 95, 96, 97, 98, 99, 100, 101, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 218, 219, 220, 221, 222, 223, 224, 225, 226, 226, 227, 228, 229, 230, 231, 232, 233, 233, 234, 235, 236, 237, 238, 239, 240, 240, 241, 242, 243, 244, 245, 246, 246, 247, 248, 249, 250, 251, 252, 252, 253, 254, 255};
#define ESC_SQ_LENGTH 20
#define ROW_ARR_LENGTH (H_RESOLUTION * ESC_SQ_LENGTH + 1)
void print_grayscale()
{
    for (int i = 0; i < 256; i++)
    {
        double c, Y, g;
        c = pow((double)i / 255.0, 2.2);
        Y = c * 0.2126 + c * 0.7152 + 0.0722 * c;

        if (Y <= (216.0 / 24389.0))   // The CIE standard states 0.008856 but 216/24389 is the intent for 0.008856451679036
            g = Y * (24389.0 / 27.0); // The CIE standard states 903.3, but 24389/27 is the intent, making 903.296296296296296
        else
            g = pow(Y, (1.0 / 3.0)) * 116 - 16;
        g *= 2.55;
        printf("\x1b[48;2;%i;%i;%im%s", (int)g, (int)g, (int)g, " ");
    }
    printf("\x1b[0m\n");
}

// meny string
struct termios orig_termios;

#define EPSILON 0.000001

// program variables

char FOV = 1;
char menu[] = "+-------------------------------+\n| h=toggle menu |r=reset mesh   |\n| s=switch ctrl.|a=animate      |\n| 8=rotate up   |2=rotate down  |\n| 4=rotate left |6=rotate right |\n| 7=roll left   |9=roll right   |\n| 1=inc. step s.|3=dec. step s. |\n| +=inc. mesh s.|-=dec. step s. |\n+-q=quit-d=debug----------------+\n+-------------------------------+\n| h=toggle menu |r=reset mesh   |\n| s=switch ctrl.|a=animate      |\n| 8=move mesh +y|2=move mesh -y |\n| 4=move mesh -x|6=move mesh +x |\n| 7=move mesh -z|9=move mesh +z |\n| 1=inc. step s.|3=dec. step s. |\n| +=inc. mesh s.|-=dec. step s. |\n+-q=quit-d=debug----------------+\n";
char str_buffer[128];

typedef struct vec3
{
    float x, y, z;
} vec3;

#define VEC3_ZERO \
    (vec3) { 0, 0, 0 }

vec3 sub(vec3 v0, vec3 v1)
{
    vec3 product;
    product.x = v0.x - v1.x;
    product.y = v0.y - v1.y;
    product.z = v0.z - v1.z;
    return product;
}

vec3 add(vec3 v0, vec3 v1)
{
    vec3 product;
    product.x = v0.x + v1.x;
    product.y = v0.y + v1.y;
    product.z = v0.z + v1.z;
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

vec3 scale(vec3 v0, float scalar)
{
    return (vec3){v0.x * scalar, v0.y * scalar, v0.z * scalar};
}

float dot(vec3 v0, vec3 v1)
{
    float product = 0;
    product += v0.x * v1.x;
    product += v0.y * v1.y;
    product += v0.z * v1.z;
    return product;
}

float mag(vec3 v0)
{
    return sqrtf(v0.x * v0.x + v0.y * v0.y + v0.z * v0.z);
}

vec3 norm_vec3(vec3 v0)
{
    float magn = mag(v0);
    return (vec3){v0.x / magn, v0.y / magn, v0.z / magn};
}

// general vec3 rotation matrix
// Tait-Bryan angles
vec3 rotate(vec3 point, float x_rad, float y_rad, float z_rad)
{

    // precalculate the cosine and sine of the radians
    float cos_x, cos_y, cos_z, sin_x, sin_y, sin_z;
    cos_x = cosf(x_rad);
    cos_y = cosf(y_rad);
    cos_z = cosf(z_rad);

    sin_x = sinf(x_rad);
    sin_y = sinf(y_rad);
    sin_z = sinf(z_rad);

    // matrix math
    float x1, x2, x3, y1, y2, y3, z1, z2, z3;
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

typedef struct quat
{
    float w, x, y, z;
} quat;

// can be used to check if unit quats are still unit sized, saving a sqrt call
#define PRE_QUAT_MAG(q) (q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z)
#define QUAT_PRECISION 0.000001
#define QUAT_ZERO \
    (quat) { 1, 0, 0, 0 }
#define QUAT_TRUE_ZERO \
    (quat) { 0, 0, 0, 0 }
#define FORMATTED_QUATERNION(buffer, str, q) sprintf(buffer, "%sx: % f, y: % f, z: % f, w:% f", str, q.x, q.y, q.z, q.w)

quat quat_norm(quat q)
{
    float mag = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    q.w /= mag;
    q.x /= mag;
    q.y /= mag;
    q.z /= mag;
    return q;
}

// quat multiplication is non-commutative, x*y != y*x
quat quat_mult(quat q0, quat q1)
{
    quat product;
    product.w = (q0.w * q1.w - q0.x * q1.x - q0.y * q1.y - q0.z * q1.z);
    product.x = (q0.w * q1.x + q0.x * q1.w + q0.y * q1.z - q0.z * q1.y);
    product.y = (q0.w * q1.y - q0.x * q1.z + q0.y * q1.w + q0.z * q1.x);
    product.z = (q0.w * q1.z + q0.x * q1.y - q0.y * q1.x + q0.z * q1.w);
    return product;
}

// Squish the rotation data into a quat, scalar is angle and vector is axis
quat local_rotation(quat q)
{
    quat product;
    product.w = cosf(q.w / 2.0);
    float a_comp = sinf(q.w / 2.0);
    product.x = q.x * a_comp;
    product.y = q.y * a_comp;
    product.z = q.z * a_comp;
    return product;
}

void quat_rotate(quat *q, quat rotation)
{
    float pre_mag = PRE_QUAT_MAG((*q));
    if (pre_mag > QUAT_PRECISION)
        *q = quat_norm(*q);

    quat q0 = quat_mult(local_rotation(rotation), *q);
    *q = q0;
}

// tri normal is a side product of rotation
// if you need the normal for anything do it after the rotation math or realise it's a frame old.
typedef struct tri
{
    vec3 verts[3];
    vec3 normal;
} tri;

void mesh_rotate(tri *mesh_in, tri *mesh_out, int mesh_size, quat *q, quat rotation)
{
    quat_rotate(q, rotation);
    quat q0 = *q;
    float x1, x2, x3, y1, y2, y3, z1, z2, z3;
    x1 = 1 - 2 * q0.y * q0.y - 2 * q0.z * q0.z;
    x2 = 2 * q0.x * q0.y - 2 * q0.w * q0.z;
    x3 = 2 * q0.x * q0.z + 2 * q0.w * q0.y;

    y1 = 2 * q0.x * q0.y + 2 * q0.w * q0.z;
    y2 = 1 - 2 * q0.x * q0.x - 2 * q0.z * q0.z;
    y3 = 2 * q0.y * q0.z - 2 * q0.w * q0.x; // this one was supposed to be subtraction too
                                            // thanks to https://danceswithcode.net/engineeringnotes/quaternions/quaternions.html
    z1 = 2 * q0.x * q0.z - 2 * q0.w * q0.y; // for the correct rotation matrix.
    z2 = 2 * q0.y * q0.z + 2 * q0.w * q0.x; // this one was addition not subtraction.
    z3 = 1 - 2 * q0.x * q0.x - 2 * q0.y * q0.y;

    // Multiplication
    for (int ii = 0; ii < mesh_size; ii++)
    {
        for (char i = 0; i < 3; i++)
        {
            mesh_out[ii].verts[i].x = x1 * (mesh_in + ii)->verts[i].x + x2 * (mesh_in + ii)->verts[i].y + x3 * (mesh_in + ii)->verts[i].z;
            mesh_out[ii].verts[i].y = y1 * (mesh_in + ii)->verts[i].x + y2 * (mesh_in + ii)->verts[i].y + y3 * (mesh_in + ii)->verts[i].z;
            mesh_out[ii].verts[i].z = z1 * (mesh_in + ii)->verts[i].x + z2 * (mesh_in + ii)->verts[i].y + z3 * (mesh_in + ii)->verts[i].z;
        }

        vec3 e0 = sub(mesh_out[ii].verts[1], mesh_out[ii].verts[0]);
        vec3 e1 = sub(mesh_out[ii].verts[2], mesh_out[ii].verts[0]);
        mesh_out[ii].normal = cross(e1, e0);
    }
}

// implementation of Tomas Moller & Ben Trumbore algorithm
int ray_collision(tri tri, vec3 ray_orig, vec3 ray_vec, vec3 *uv_out)
{
    vec3 e1 = sub(tri.verts[1], tri.verts[0]);
    vec3 e2 = sub(tri.verts[2], tri.verts[0]);
    vec3 ce2 = cross(ray_vec, e2);
    float det = dot(e1, ce2);

    // for one-sided tris
    if (det < EPSILON)
        return 0;

    float inv_det = 1.0 / det;
    vec3 s = sub(ray_orig, tri.verts[0]);
    float u = inv_det * dot(s, ce2);
    if (u < 0 || u > 1)
        return 0;

    vec3 sce1 = cross(s, e1);
    float v = inv_det * dot(ray_vec, sce1);
    if (v < 0 || u + v > 1)
        return 0;

    float t = inv_det * dot(e2, sce1);
    uv_out->x = u;
    uv_out->y = v;
    uv_out->z = t;
    return 1;
}

void mesh_scale(tri *mesh_in, tri *mesh_out, int mesh_size, float scalar)
{
    for (int i = 0; i < mesh_size * 3; i++)
    {
        mesh_out[i / 3].verts[i % 3] = scale(mesh_in[i / 3].verts[i % 3], scalar);
    }
}

void mesh_translate(tri *mesh_in, tri *mesh_out, int mesh_size, vec3 movement)
{
    for (int i = 0; i < mesh_size * 3; i++)
    {
        mesh_out[i / 3].verts[i % 3] = add(mesh_out[i / 3].verts[i % 3], movement);
    }
}

// expects cstring
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

int load_obj(char *path, tri **mesh_out, short *mesh_size)
{
    short vec_amount = 0, tri_amount = 0;
    FILE *file = fopen(path, "r");
    if (file == NULL)
        return 0;

    // cant be bothered making a dynamic array
    // so count the tris and verticies beforehand
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
    *mesh_out = (tri *)malloc(sizeof(tri) * tri_amount);
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
                *((float *)&output + i) = strtod(&str_buffer[offset + 9 * i], (void *)0);
                if (str_buffer[offset + (9 * i)] == '-')
                    offset++;
            }
            vec_buffer[vec_amount] = output;
            vec_amount++;
        }
        otherwise(str_buffer[0] == 'f' && str_buffer[1] == ' ')
        {
            int cur_char = 0;
            tri tri;
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

tri *mesh = 0;

void restore_term()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\x1b[?25h");
}

void set_raw_term()
{ // saves local variables before changing them, and changeing them back at program termination
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(restore_term);
    // changing them to not exho text input
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    cfsetspeed(&raw, B230400);
    tcsetattr(STDIN_FILENO, TCSADRAIN, &raw);
    printf("\x1b[?25l");
}

clock_t ave_arr[122];
char ave_arr_filled = 0;
char ave_arr_offset = 0;
int main()
{

    // printf("\x1b[48;2;000;000;000m ");
    char test = '\x1b';
    short poly_count;
    int input = 0;
    int anim_timer = 0;
    char menu_select = 0;
    char debug = 0;
    ave_arr[120] = INT64_MAX;
    ave_arr[121] = 0;
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

    float h_step = FOV / (float)H_RESOLUTION;
    float h_start = -h_step * (H_RESOLUTION / 2);
    float v_step = FOV / (float)V_RESOLUTION;
    float v_start = -v_step * (V_RESOLUTION / 2);
    char buffer[V_RESOLUTION * ROW_ARR_LENGTH];
    char render_menu = 0;

    tri render_mesh[poly_count];
    mesh_scale(mesh, mesh, poly_count, 1);

    vec3 mesh_anchor = VEC3_ZERO;

    vec3 light_vec3 = (vec3){-0.5, 0, 0.5};
    quat mesh_rotation = QUAT_ZERO;
    vec3 rotation = VEC3_ZERO;
    quat frame_rotation = QUAT_TRUE_ZERO;
    vec3 mesh_movement = VEC3_ZERO;
    vec3 anim_rotation = VEC3_ZERO;
    float step_size = 0.01;

    int iii = 0;
    float depth;

    clock_t end = 0;
    clock_t print_t = 0;

    while (1)
    {
        fflush(stdin);
        clock_t start = clock();
        vec3 temp_coords;
        char hit;
        float light_level;

        // Apply Rotation
        clock_t rotation_t = clock();
        mesh_rotate(mesh, render_mesh, poly_count, &mesh_rotation, frame_rotation);
        rotation_t = clock() - rotation_t;
        // if I want to do it right scale should be here though I see little reason not to do it in the origin mesh
        // apply translation
        clock_t translation_t = clock();
        mesh_anchor = add(mesh_anchor, mesh_movement);
        mesh_movement = VEC3_ZERO;
        mesh_translate(render_mesh, render_mesh, poly_count, mesh_anchor);
        translation_t = clock() - translation_t;

        frame_rotation = QUAT_TRUE_ZERO;
        clock_t render_t = clock();

        int rgb;

        for (int ii = 0; ii < V_RESOLUTION; ii++)
        {
            for (int i = 0; i < H_RESOLUTION; i++)
            {
                hit = 0;
                depth = HUGE_VAL;
                for (int iV = 0; iV < poly_count; iV++)
                {
                    if (!ray_collision(render_mesh[iV], (vec3){0, 0, 12}, (vec3){(h_start + (i * h_step)) * 1.4, -v_start - (ii * v_step), -1}, &temp_coords))
                        continue;
                    if (temp_coords.z > depth)
                        continue;
                    depth = temp_coords.z;
                    light_level = acosf(dot(light_vec3, render_mesh[iV].normal) / (mag(light_vec3) * mag(render_mesh[iV].normal)));
                    // buffer[ii][i] = BRIGHTNESS((int)floor((light_level * 9) / 1.4));
                    // buffer[ii][i] = (int)round(light_level * 81.5286);
                    rgb = (int)round(light_level * 81.5286);
                    sprintf(buffer + (ROW_ARR_LENGTH * ii + i * ESC_SQ_LENGTH), "\x1b[48;2;%03i;%03i;%03im ", rgb, rgb, rgb);
                    hit = 1;
                }
                if (!hit)
                    sprintf(buffer + (ROW_ARR_LENGTH * ii + i * ESC_SQ_LENGTH), "\x1b[48;2;000;000;000m ");
            }
            buffer[(ROW_ARR_LENGTH * ii) - 1] = 10;
        }
        buffer[ROW_ARR_LENGTH * V_RESOLUTION - 1] = 0;
        buffer[0] = '\x1b';
        render_t = clock() - render_t;

        // menu logic
        // TODO: figure out new implementation of help & debug menu

        // if (0)
        // {
        //     for (int i = 0; i < 9; i++)
        //     {
        //         for (int ii = 0; ii < 33; ii++)
        //         {
        //             buffer[i][ii] = menu[ii + ((8 - i) * 34) + (menu_select * 306)];
        //         }
        //     }
        //     if (debug) // you knwo, this debug thing really is annoying, if only someone cleaned it up
        //     {
        //         sprintf(buffer[0] + 17, "step_s.=%#6.4f", step_size);

        //         *(buffer[0] + 31) = '-'; // fixes corner of menu
        //         *(buffer[0] + 32) = '+';

        //         FORMATTED_QUATERNION(buffer[0] + 34, "Mesh Quaternion Rotation: ", mesh_rotation);
        //         *(buffer[0] + 113) = ' ';
        //         sprintf(buffer[1] + 34, "Mesh Anchor: x: %f, y: %f, z: %f", mesh_anchor.x, mesh_anchor.y, mesh_anchor.z);
        //         *(buffer[1] + 84) = ' ';

        //         if (end < ave_arr[120])
        //             ave_arr[120] = end;
        //         if (end > ave_arr[121])
        //             ave_arr[121] = end;
        //         int average_cycles;
        //         ave_arr[ave_arr_offset] = end;
        //         ave_arr_offset++;

        //         if (ave_arr_offset == 120)
        //         {
        //             ave_arr_offset = 0;
        //             ave_arr_filled = 1;
        //         }
        //         if (ave_arr_filled)
        //         {
        //             long unsigned long total_cycles = 0;
        //             for (char i = 0; i < 120; i++)
        //             {
        //                 total_cycles += ave_arr[i];
        //             }
        //             average_cycles = total_cycles / 120;
        //         }
        //         else
        //         {
        //             long unsigned long total_cycles = 0;
        //             for (char i = 0; i < ave_arr_offset; i++)
        //             {
        //                 total_cycles += ave_arr[i];
        //             }
        //             average_cycles = total_cycles / ave_arr_offset;
        //         }

        // sprintf(buffer[7] + H_RESOLUTION - 26, "Render step length: %06lu", render_t);
        // sprintf(buffer[6] + H_RESOLUTION - 26, "Rotation s. length: %06lu", rotation_t);
        // sprintf(buffer[5] + H_RESOLUTION - 26, "Translation length: %06lu", translation_t);
        // sprintf(buffer[4] + H_RESOLUTION - 26, "Print frame length: %06lu", print_t);
        // sprintf(buffer[3] + H_RESOLUTION - 27, "Short. frame length: %06lu", ave_arr[120]);
        // sprintf(buffer[2] + H_RESOLUTION - 26, "Long. frame length: %06lu", ave_arr[121]);
        // sprintf(buffer[1] + H_RESOLUTION - 26, "Avrg. frame length: %06i", average_cycles);
        // sprintf(buffer[0] + H_RESOLUTION - 25, "Cycles last frame: %06lu", end);
        //     }
        // }
        // stops frames to look like they're falling
        clock_t render_time = clock() - start;
        print_t = clock();

        if ((int)render_time + print_t < 30000)
            usleep(30000 - (render_time + print_t));

        // output
        printf("%s\x1b[0m\n", buffer);
        print_t = clock() - print_t;
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
        char step_mod = 1;
        switch (input)
        {
        case 50: // 2 rotate +x/translate -y
            step_mod = -1;
        case 56: // 8 rotate -x/translate +y
            if (menu_select)
                mesh_movement.y += step_size * step_mod * 10;
            else
                frame_rotation = (quat){-step_size * step_mod, 1, 0, 0};
            break;
        case 52: // 4 rotate -y/translate -x
            step_mod = -1;
        case 54: // 6 rotate +y/translate +x
            if (menu_select)
                mesh_movement.x += step_size * step_mod * 10;
            else
                frame_rotation = (quat){step_size * step_mod, 0, 1, 0};
            break;
        // Z axis:
        case 55: // 7 rotate +z/translate -z
            step_mod = -1;
        case 57: // 9 rotate -z/translate +z
            if (menu_select)
                mesh_movement.z += step_size * step_mod * 10;
            else
                frame_rotation = (quat){-step_size * step_mod, 0, 0, 1};
            break;
        // Step size
        case 49: // step down/1
            step_mod = -1;
        case 51: // step up/3
            step_size += 0.01 * step_mod;
            step_size = step_size < 0 ? 0 : step_size;
            break;
        // zoom? mesh size
        case 45: // scale up
            mesh_scale(mesh, mesh, poly_count, 1 / 1.1);
            break;
        case 43: // scale down
            mesh_scale(mesh, mesh, poly_count, 1.1);
            break;

        case 'r':
            rotation = VEC3_ZERO;
            mesh_rotation = QUAT_ZERO;
            mesh_anchor = VEC3_ZERO;
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
            break;

        case 'h':
            render_menu = !render_menu;
            break;

        case 'd':
            debug = !debug;
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

        case 's':
            menu_select = !menu_select;
            break;

        default:
            break;
        }
        if (input == 'q')
            break;
        end = clock() - start;
    }
    free(mesh);
    return 0;
}
