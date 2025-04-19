#include "RubiksCube.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <iostream>

// Generic helpers ------------------------------------------------------------
static vec3 rotateVec90(const vec3 &v, int axis, bool cw) {
    // axis: 0->X,1->Y,2->Z. cw means +90° when looking toward +axis.
    if (axis == 0) {            // X
        return cw ? vec3(v.x,  v.z, -v.y) : vec3(v.x, -v.z,  v.y);
    } else if (axis == 1) {     // Y
        return cw ? vec3(-v.z, v.y,  v.x) : vec3( v.z, v.y, -v.x);
    }
    /* axis == 2 */             // Z
    return cw ? vec3( v.y, -v.x, v.z) : vec3(-v.y,  v.x, v.z);
}

// ---------------------------------------------------------------------------
RubiksCube::RubiksCube()
    : rotating_face(-1), rotating_layer(0), rotation_angle(0.0f), animation_active(false) {
    initialize();
}

RubiksCube::~RubiksCube() = default;

void RubiksCube::initialize() {
    cubies.clear();
    cubie_transforms.clear();

    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            for (int z = -1; z <= 1; ++z)
                cubies.emplace_back(x, y, z);

    regenerateTransforms();
}

// --------------- animation --------------------------------------------------
void RubiksCube::randomize(int moves) {
    rotation_queue.clear();
    static const int faces[6] = {RIGHT, LEFT, TOP, BOTTOM, FRONT, BACK};
    static const int layers[6] = {1, -1, 1, -1, 1, -1};
    
    printf("Queueing %d random moves\n", moves);
    
    // Generate random moves and add to queue - one move at a time
    for (int i = 0; i < moves; i++) {
        int face_idx = rand() % 6;
        int face = faces[face_idx];
        int layer = layers[face_idx];
        bool clockwise = (rand() % 2 == 0);
        
        // Store face, layer, and whether it's clockwise
        rotation_queue.emplace_back(std::make_pair(face, layer));
        rotation_queue_clockwise.push_back(clockwise);
        
        printf("Queued move %d: Face %d, Layer %d, %s\n", 
               i+1, face, layer, clockwise ? "CW" : "CCW");
    }
    
    // Start the first rotation if not already animating
    if (!animation_active && !rotation_queue.empty()) {
        auto move = rotation_queue.front();
        bool clockwise = rotation_queue_clockwise.front();
        rotation_queue.erase(rotation_queue.begin());
        rotation_queue_clockwise.erase(rotation_queue_clockwise.begin());
        
        printf("Starting first move: Face %d, Layer %d, %s\n", 
               move.first, move.second, clockwise ? "CW" : "CCW");
        startRotation(move.first, move.second, clockwise);
    }
}

// --------------- animation --------------------------------------------------
void RubiksCube::startRotation(int face, int layer, bool clockwise) {
    if (animation_active) return; // Don't start new rotation if one is already in progress

    rotating_face  = face;
    rotating_layer = layer;
    rotation_angle = 0.0f;
    rotating_clockwise = !clockwise;
    animation_active = true;

    rotation_axis = FACE_DIR[face];
    if (!clockwise) rotation_axis = -rotation_axis;

    logRotation(face, layer, clockwise);
}

void RubiksCube::updateAnimation() {
    if (!animation_active) {
        // If we have more moves in the queue, start the next one
        if (!rotation_queue.empty()) {
            auto move = rotation_queue.front();
            bool clockwise = rotation_queue_clockwise.front();
            rotation_queue.erase(rotation_queue.begin());
            rotation_queue_clockwise.erase(rotation_queue_clockwise.begin());
            
            printf("Starting next queued move: Face %d, Layer %d, %s\n", 
                   move.first, move.second, clockwise ? "CW" : "CCW");
            startRotation(move.first, move.second, clockwise);
        }
        return;
    }

    rotation_angle += ROTATION_SPEED;
    if (rotation_angle >= 90.0f) {
        updateCubiesAfterRotation(rotating_face, rotating_layer, rotating_clockwise);
        rotation_angle = 0.0f;
        animation_active = false;
        
        // If we have more moves in the queue, start the next one
        if (!rotation_queue.empty()) {
            auto move = rotation_queue.front();
            bool clockwise = rotation_queue_clockwise.front();
            rotation_queue.erase(rotation_queue.begin());
            rotation_queue_clockwise.erase(rotation_queue_clockwise.begin());
            
            printf("Starting next move in sequence: Face %d, Layer %d, %s\n", 
                   move.first, move.second, clockwise ? "CW" : "CCW");
            startRotation(move.first, move.second, clockwise);
        }
    }
}

// --------------- core logic -------------------------------------------------
void RubiksCube::updateCubiesAfterRotation(int face, int layer, bool clockwise) {
    // Determine principal axis index and cw direction according to our helper
    int axis = (face == RIGHT || face == LEFT) ? 0 : (face == TOP || face == BOTTOM ? 1 : 2);
    // For LEFT, BOTTOM, BACK faces the perceived clockwise is opposite
    bool perceived_clockwise = clockwise;
    if (face == LEFT || face == BOTTOM || face == BACK) 
        perceived_clockwise = !perceived_clockwise;

    printf("Updating cubies after rotation: face %d, layer %d, clockwise %d, perceived_clockwise %d\n", 
           face, layer, clockwise, perceived_clockwise);

    // Gather indices of cubies in the affected slice
    std::vector<int> slice = getFaceCubies(face, layer);
    std::vector<Cubie> old = cubies; // snapshot

    // --- Position update ----------------------------------------------------
    for (int idx : slice) {
        Cubie &c = cubies[idx];
        vec3 centre(0);
        if (axis == 0) centre.x = layer;
        else if (axis == 1) centre.y = layer;
        else centre.z = layer;

        vec3 rel = vec3(c.x, c.y, c.z) - centre;
        vec3 relR = rotateVec90(rel, axis, perceived_clockwise);
        vec3 posN = relR + centre;
        c.x = int(round(posN.x));
        c.y = int(round(posN.y));
        c.z = int(round(posN.z));
    }

    // --- Sticker colour remap (generic) -------------------------------------
    for (int idx : slice) {
        Cubie &dst = cubies[idx];
        const Cubie &src = old[idx];
        color4 newColors[6];

        for (int f = 0; f < 6; ++f) {
            vec3 dirOld = FACE_DIR[f];
            vec3 dirNew = rotateVec90(dirOld, axis, perceived_clockwise);
            int toFace = -1;
            for (int k = 0; k < 6; ++k) {
                if (dot(dirNew, FACE_DIR[k]) > 0.9f) { 
                    toFace = k; 
                    break; 
                }
            }
            if (toFace != -1) {
                newColors[toFace] = src.colors[f];
                printf("Color map: face %d -> face %d\n", f, toFace);
            } else {
                printf("ERROR: Could not find destination face for direction (%.2f, %.2f, %.2f)\n",
                       dirNew.x, dirNew.y, dirNew.z);
            }
        }
        
        // Copy the new colors
        for (int k = 0; k < 6; ++k) 
            dst.colors[k] = newColors[k];

        dst.updateVisibility();
        cubie_transforms[idx] = calculateCubieTransform(dst.x, dst.y, dst.z);
    }
}

// --------------- utilities --------------------------------------------------
std::vector<int> RubiksCube::getFaceCubies(int face, int layer) const {
    std::vector<int> res;
    for (int i = 0; i < cubies.size(); ++i) {
        const Cubie &c = cubies[i];
        bool onLayer = false;
        switch (face) {
            case RIGHT:  case LEFT:   onLayer = (c.x == layer); break;
            case TOP:    case BOTTOM: onLayer = (c.y == layer); break;
            case FRONT:  case BACK:   onLayer = (c.z == layer); break;
        }
        if (onLayer) res.push_back(i);
    }
    return res;
}

mat4 RubiksCube::calculateCubieTransform(int x,int y,int z) const {
    float s = CUBE_SIZE + CUBE_GAP;
    return Translate(x*s, y*s, z*s) * Scale(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
}

void RubiksCube::regenerateTransforms() {
    cubie_transforms.clear();
    for (const Cubie &c : cubies) cubie_transforms.push_back(calculateCubieTransform(c.x,c.y,c.z));
}

void RubiksCube::logRotation(int face, int layer, bool cw) const {
    static const char *n[]={"RIGHT","LEFT","TOP","BOTTOM","FRONT","BACK"};
    printf("Rotate %s layer %d %s\n", n[face], layer, cw?"CW":"CCW");
}