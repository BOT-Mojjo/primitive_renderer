void tetrahedron(triangle *tri) {
    double tan_30 = 2.5 / (tan(0.5235));
    tri[0].verts[0] = (vec3){5, 0, 0};
    tri[0].verts[2] = (vec3){0, 0, 0};
    tri[0].verts[1] = (vec3){2.5, tan_30, 0};

    tri[1].verts[0] = (vec3){0, 0, 0};
    tri[1].verts[2] = (vec3){5, 0, 0};
    tri[1].verts[1] = (vec3){2.5, 2.5, 5};

    tri[2].verts[0] = (vec3){5, 0, 0};
    tri[2].verts[2] = (vec3){2.5, tan_30, 0};
    tri[2].verts[1] = (vec3){2.5, 2.5, 5};

    tri[3].verts[0] = (vec3){2.5, tan_30, 0};
    tri[3].verts[2] = (vec3){0, 0, 0};
    tri[3].verts[1] = (vec3){2.5, 2.5, 5};
}

void cube(triangle *mesh) {
    {
        mesh[0].verts[0] = (vec3){1, 1, 1};
        mesh[0].verts[1] = (vec3){1, -1, 1};
        mesh[0].verts[2] = (vec3){-1, -1, 1};

        mesh[1].verts[0] = (vec3){1, 1, 1};
        mesh[1].verts[1] = (vec3){-1, -1, 1};
        mesh[1].verts[2] = (vec3){-1, 1, 1};

        mesh[2].verts[0] = (vec3){-1, 1, 1};
        mesh[2].verts[1] = (vec3){-1, -1, 1};
        mesh[2].verts[2] = (vec3){-1, -1, -1};

        mesh[3].verts[0] = (vec3){-1, 1, 1};
        mesh[3].verts[1] = (vec3){-1, -1, -1};
        mesh[3].verts[2] = (vec3){-1, 1, -1};

        mesh[4].verts[0] = (vec3){-1, 1, -1};
        mesh[4].verts[1] = (vec3){-1, -1, -1};
        mesh[4].verts[2] = (vec3){1, 1, -1};

        mesh[5].verts[0] = (vec3){-1, -1, -1};
        mesh[5].verts[1] = (vec3){1, -1, -1};
        mesh[5].verts[2] = (vec3){1, 1, -1};

        mesh[6].verts[0] = (vec3){1, 1, -1};
        mesh[6].verts[1] = (vec3){1, -1, -1};
        mesh[6].verts[2] = (vec3){1, -1, 1};

        mesh[7].verts[0] = (vec3){1, 1, -1};
        mesh[7].verts[1] = (vec3){1, -1, 1};
        mesh[7].verts[2] = (vec3){1, 1, 1};

        mesh[8].verts[0] = (vec3){1, 1, 1};
        mesh[8].verts[1] = (vec3){-1, 1, 1};
        mesh[8].verts[2] = (vec3){-1, 1, -1};

        mesh[9].verts[0] = (vec3){1, 1, 1};
        mesh[9].verts[1] = (vec3){-1, 1, -1};
        mesh[9].verts[2] = (vec3){1, 1, -1};

        mesh[10].verts[0] = (vec3){1, -1, 1};
        mesh[10].verts[1] = (vec3){-1, -1, -1};
        mesh[10].verts[2] = (vec3){-1, -1, 1};

        mesh[11].verts[0] = (vec3){1, -1, 1};
        mesh[11].verts[1] = (vec3){1, -1, -1};
        mesh[11].verts[2] = (vec3){-1, -1, -1};
    }
}