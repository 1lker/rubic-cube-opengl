#ifndef CUBIE_H
#define CUBIE_H

#include "Angel.h"
#include <vector>

typedef vec4 color4;
typedef vec4 point4;

// Standard face colors for Rubik's cube
const color4 RED     = color4(1.0, 0.0, 0.0, 1.0);    // right face (+X)
const color4 ORANGE  = color4(1.0, 0.5, 0.0, 1.0);    // left face (-X)
const color4 WHITE   = color4(1.0, 1.0, 1.0, 1.0);    // top face (+Y)
const color4 YELLOW  = color4(1.0, 1.0, 0.0, 1.0);    // bottom face (-Y)
const color4 GREEN   = color4(0.0, 1.0, 0.0, 1.0);    // front face (+Z)
const color4 BLUE    = color4(0.0, 0.0, 1.0, 1.0);    // back face (-Z)
const color4 BLACK   = color4(0.0, 0.0, 0.0, 1.0);    // for inner faces (not visible)

// Face directions
enum Face {
    RIGHT = 0,   // +X
    LEFT = 1,    // -X
    TOP = 2,     // +Y
    BOTTOM = 3,  // -Y
    FRONT = 4,   // +Z
    BACK = 5     // -Z
};

// Face directions (unit vectors)
const vec3 FACE_DIR[6] = {
    vec3( 1.0,  0.0,  0.0),  // RIGHT
    vec3(-1.0,  0.0,  0.0),  // LEFT
    vec3( 0.0,  1.0,  0.0),  // TOP
    vec3( 0.0, -1.0,  0.0),  // BOTTOM
    vec3( 0.0,  0.0,  1.0),  // FRONT
    vec3( 0.0,  0.0, -1.0)   // BACK
};

// Structure for a cubie (a small cube in the Rubik's cube)
class Cubie {
public:
    int x, y, z;           // Position in 3D grid (-1, 0, 1)
    color4 colors[6];      // Colors for each face
    bool visible[6];       // Whether each face is visible

    Cubie(int x = 0, int y = 0, int z = 0);
    void updateVisibility();
    void resetToInitialState();
    
    // Get the initial color for a given position and face
    static color4 getInitialColor(int x, int y, int z, int face);
};

#endif // CUBIE_H