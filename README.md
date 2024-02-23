# Primitive Renderer

## A simple tool to view .OBJ files

### Description

This was an exam project made over the course of a few weeks. It was chosen due to it's small size yet diverse challenge during it's creation. It uses an implementation of the Möller-Trumbore algorithm for rendering triangles.

### Usage

Clone the project locally and compile it using your preferred C compiler.
Upon running the program it will ask for a .obj file, specify its path relative to the directory of the program.

Press 'H' to open a small UI listing all of the controls

### Restrictions

The project saves the parsed values from the .obj file in the C stack, so beware stack overflow in case of larger meshes.
The project also utilises functions from the termios.h and unistd.h headers so a UNIX-flavoured system is required.
