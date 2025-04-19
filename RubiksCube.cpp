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
    static const int layers[6] = {1,-1,1,-1,1,-1};
    for (int i=0;i<moves;++i){
        int f = rand()%6;
        rotation_queue.emplace_back(f,layers[f]);
    }
    if(!animation_active && !rotation_queue.empty()){
        auto n = rotation_queue.front();
        rotation_queue.erase(rotation_queue.begin());
        startRotation(n.first,n.second,true);
    }
}

// --------------- animation --------------------------------------------------
void RubiksCube::startRotation(int face, int layer, bool clockwise) {
    if (animation_active || layer == 0) return; // disallow middle slices for simplicity

    rotating_face  = face;
    rotating_layer = layer;
    rotation_angle = 0.0f;
    animation_active = true;

    rotation_axis = FACE_DIR[face];
    if (!clockwise) rotation_axis = -rotation_axis;

    logRotation(face, layer, clockwise);
}

void RubiksCube::updateAnimation() {
    if (!animation_active) return;

    rotation_angle += ROTATION_SPEED;
    if (rotation_angle >= 90.0f) {
        bool clockwise = (rotation_axis == FACE_DIR[rotating_face]);
        updateCubiesAfterRotation(rotating_face, rotating_layer, clockwise);
        rotation_angle   = 0.0f;
        animation_active = false;
    }
}

// --------------- core logic -------------------------------------------------
void RubiksCube::updateCubiesAfterRotation(int face, int layer, bool clockwise) {
    // Determine principal axis index and cw direction according to our helper
    int axis = (face == RIGHT || face == LEFT) ? 0 : (face == TOP || face == BOTTOM ? 1 : 2);
    // For LEFT, BOTTOM, BACK faces the perceived clockwise is opposite
    if (face == LEFT || face == BOTTOM || face == BACK) clockwise = !clockwise;

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
        vec3 relR = rotateVec90(rel, axis, clockwise);
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
            vec3 dirNew = rotateVec90(dirOld, axis, clockwise);
            int toFace = -1;
            for (int k = 0; k < 6; ++k) {
                if (dot(dirNew, FACE_DIR[k]) > 0.9f) { toFace = k; break; }
            }
            newColors[toFace] = src.colors[f];
        }
        for (int k = 0; k < 6; ++k) dst.colors[k] = newColors[k];

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
