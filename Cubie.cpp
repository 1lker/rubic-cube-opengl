#include "Cubie.h"

Cubie::Cubie(int x, int y, int z) : x(x), y(y), z(z) {
    // Initialize all faces to black (not visible)
    for (int i = 0; i < 6; i++) {
        colors[i] = BLACK;
        visible[i] = false;
    }
    
    // Set visibility based on position
    updateVisibility();
    
    // Set initial colors based on position
    resetToInitialState();
}

void Cubie::updateVisibility() {
    // A face is visible if it's on the outer layer of the cube
    visible[RIGHT] = (x == 1);
    visible[LEFT] = (x == -1);
    visible[TOP] = (y == 1);
    visible[BOTTOM] = (y == -1);
    visible[FRONT] = (z == 1);
    visible[BACK] = (z == -1);
}

void Cubie::resetToInitialState() {
    // Set colors for visible faces based on position
    for (int face = 0; face < 6; face++) {
        if (visible[face]) {
            colors[face] = getInitialColor(x, y, z, face);
        } else {
            colors[face] = BLACK;
        }
    }
}

color4 Cubie::getInitialColor(int x, int y, int z, int face) {
    // Each face of the Rubik's cube has a standard color
    switch (face) {
        case RIGHT:  // +X face
            return RED;
        case LEFT:   // -X face
            return ORANGE;
        case TOP:    // +Y face
            return WHITE;
        case BOTTOM: // -Y face
            return YELLOW;
        case FRONT:  // +Z face
            return GREEN;
        case BACK:   // -Z face
            return BLUE;
        default:
            return BLACK;
    }
}