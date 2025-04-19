#ifndef RUBIKS_CUBE_H
#define RUBIKS_CUBE_H

#include "Cubie.h"
#include <vector>

// Constants
const float CUBE_SIZE = 0.25f;
const float CUBE_GAP = 0.03f;
const float ROTATION_SPEED = 3.0f;

class RubiksCube {
public:
    RubiksCube();
    ~RubiksCube();
    
    void initialize();
    void resetCube();
    void randomize(int moves = 20);
    
    // Rotation methods
    void startRotation(int face, int layer, bool clockwise);
    void updateAnimation();
    bool isAnimating() const { return animation_active; }
    
    // Get methods
    const std::vector<Cubie>& getCubies() const { return cubies; }
    const std::vector<mat4>& getTransforms() const { return cubie_transforms; }
    
    // Get current rotation state for rendering
    int getRotatingFace() const { return rotating_face; }
    int getRotatingLayer() const { return rotating_layer; }
    float getRotationAngle() const { return rotation_angle; }
    const vec3& getRotationAxis() const { return rotation_axis; }
    
    // Get cubies on a specific face and layer
    std::vector<int> getFaceCubies(int face, int layer) const;
    
private:
    static const int NUM_CUBIES = 27;  // 3x3x3 grid
    
    std::vector<Cubie> cubies;
    std::vector<mat4> cubie_transforms;
    
    // Animation state
    int rotating_face;
    int rotating_layer;
    float rotation_angle;
    vec3 rotation_axis;
    bool animation_active;
    std::vector<std::pair<int, int>> rotation_queue; // <face, layer>
    
    // Helper methods
    void regenerateTransforms();
    void updateCubiesAfterRotation(int face, int layer, bool clockwise);
    vec3 rotatePosition(const vec3& pos, int face, bool clockwise) const;
    mat4 calculateCubieTransform(int x, int y, int z) const;
    
    // For debugging
    void printCubieColors(int index) const;
    void logRotation(int face, int layer, bool clockwise) const;
};

#endif // RUBIKS_CUBE_H