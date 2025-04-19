#define GL_SILENCE_DEPRECATION
#include "Angel.h"
#include "RubiksCube.h"
#include "Cubie.h"
#include <vector>
#include <algorithm>
#include <ctime>
#include <cmath>

// Comment required by the project specification
// Ortho is the new Porto!
// Pineapple is important

// Vertices of a unit cube centered at origin
point4 vertices[8] = {
    point4(-0.5, -0.5,  0.5, 1.0),  // 0: left-bottom-front
    point4(-0.5,  0.5,  0.5, 1.0),  // 1: left-top-front
    point4( 0.5,  0.5,  0.5, 1.0),  // 2: right-top-front
    point4( 0.5, -0.5,  0.5, 1.0),  // 3: right-bottom-front
    point4(-0.5, -0.5, -0.5, 1.0),  // 4: left-bottom-back
    point4(-0.5,  0.5, -0.5, 1.0),  // 5: left-top-back
    point4( 0.5,  0.5, -0.5, 1.0),  // 6: right-top-back
    point4( 0.5, -0.5, -0.5, 1.0)   // 7: right-bottom-back
};

// Indices for each face (clockwise ordering)
const int face_indices[6][4] = {
    {3, 2, 6, 7},  // right face (+X)
    {0, 1, 5, 4},  // left face (-X)
    {1, 2, 6, 5},  // top face (+Y)
    {0, 4, 7, 3},  // bottom face (-Y)
    {0, 3, 2, 1},  // front face (+Z)
    {4, 5, 6, 7}   // back face (-Z)
};

// Global variables
RubiksCube rubiksCube;

// Vertex data for rendering
std::vector<point4> points;
std::vector<color4> colors;

// Camera control
float cam_distance = 4.0f;
float cam_theta = 0.5f;
float cam_phi = 0.5f;
double last_x = 0.0, last_y = 0.0;
bool mouse_dragging = false;
bool is_rotating_view = false;

// Shader uniforms
GLuint ModelView, Projection;

// Vertex buffer and array objects
GLuint vao;
GLuint buffer;

// Global variables for mouse control
bool shift_pressed = false;
bool drag_started = false;
int drag_face = -1;
int drag_layer = 0;
double drag_start_x = 0.0, drag_start_y = 0.0;
const double DRAG_THRESHOLD = 30.0;  // Pixels to trigger a drag rotation

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void printHelp();
bool is_point_on_cube(double x, double y);
std::pair<int, int> pick_face(double mouse_x, double mouse_y, bool force_selection = false);

// Function required by the project specification
void project_done() {
    printf("314254 Done!\n");
}

// Create vertices and colors for a single cubie
void generate_cubie_geometry(const Cubie& cubie, std::vector<point4>& out_points, std::vector<color4>& out_colors) {
    // For each face
    for (int face = 0; face < 6; face++) {
        // Skip invisible faces (inner faces)
        if (!cubie.visible[face]) continue;
        
        // Create two triangles for this face
        int a = face_indices[face][0];
        int b = face_indices[face][1];
        int c = face_indices[face][2];
        int d = face_indices[face][3];
        
        // Triangle 1
        out_points.push_back(vertices[a]);
        out_points.push_back(vertices[b]);
        out_points.push_back(vertices[c]);
        
        out_colors.push_back(cubie.colors[face]);
        out_colors.push_back(cubie.colors[face]);
        out_colors.push_back(cubie.colors[face]);
        
        // Triangle 2
        out_points.push_back(vertices[a]);
        out_points.push_back(vertices[c]);
        out_points.push_back(vertices[d]);
        
        out_colors.push_back(cubie.colors[face]);
        out_colors.push_back(cubie.colors[face]);
        out_colors.push_back(cubie.colors[face]);
    }
}

// Regenerate all geometry
void regenerate_geometry() {
    points.clear();
    colors.clear();
    
    // Generate geometry for each cubie
    for (const Cubie& cubie : rubiksCube.getCubies()) {
        generate_cubie_geometry(cubie, points, colors);
    }
    
    // Update the buffer
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, 
                 points.size() * sizeof(point4) + colors.size() * sizeof(color4), 
                 NULL, 
                 GL_STATIC_DRAW);
    
    glBufferSubData(GL_ARRAY_BUFFER, 0, 
                   points.size() * sizeof(point4), points.data());
    
    glBufferSubData(GL_ARRAY_BUFFER, points.size() * sizeof(point4), 
                   colors.size() * sizeof(color4), colors.data());
}

// Initialize OpenGL state
void init() {
    // Initialize the Rubik's cube
    rubiksCube.initialize();
    
    // Generate initial geometry
    regenerate_geometry();
    
    // Load shaders
    GLuint program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);
    
    // Create vertex array and buffer
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &buffer);
    
    // Bind vertex array
    glBindVertexArray(vao);
    
    // Bind buffer and upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, 
                 points.size() * sizeof(point4) + colors.size() * sizeof(color4), 
                 NULL, 
                 GL_STATIC_DRAW);
    
    // Upload points
    glBufferSubData(GL_ARRAY_BUFFER, 0, 
                   points.size() * sizeof(point4), points.data());
    
    // Upload colors
    glBufferSubData(GL_ARRAY_BUFFER, points.size() * sizeof(point4), 
                   colors.size() * sizeof(color4), colors.data());
    
    // Set up vertex attributes
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    GLuint vColor = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 
                         BUFFER_OFFSET(points.size() * sizeof(point4)));
    
    // Get uniform locations
    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");
    
    // Set projection matrix
    mat4 projection = Perspective(45.0, 1.0, 0.1, 100.0);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2, 0.2, 0.2, 1.0);
}

// Check if a point is within the cube's bounds
bool is_point_on_cube(double x, double y) {
    // Get current window size
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    
    // Normalize device coordinates (-1 to 1)
    float ndc_x = (2.0f * x) / width - 1.0f;
    float ndc_y = 1.0f - (2.0f * y) / height;
    
    printf("NDC coordinates: (%.2f, %.2f)\n", ndc_x, ndc_y);
    
    // Increase detection radius to make it easier to click on the cube
    const float radius = 0.9f; // Increased from 0.7f
    float dist_squared = ndc_x * ndc_x + ndc_y * ndc_y;
    
    printf("Distance squared from center: %.2f (threshold: %.2f)\n", 
           dist_squared, radius * radius);
    
    return dist_squared < (radius * radius);
}

// Ray-cube intersection for better face picking
bool ray_cube_intersection(const vec3& ray_origin, const vec3& ray_dir, 
                          vec3& intersection_point, int& face_hit, bool force_selection = false) {
    // Cube bounds (considering all cubies and spacing)
    float cube_size = 3.0f * CUBE_SIZE + 2.0f * CUBE_GAP;
    // Increase cube size for more forgiving intersection testing
    cube_size *= 1.5f;  // Make the cube 50% larger for intersection purposes
    
    float min_bound = -cube_size/2.0f;
    float max_bound = cube_size/2.0f;
    
    printf("Ray origin: (%.2f, %.2f, %.2f), direction: (%.2f, %.2f, %.2f)\n", 
           ray_origin.x, ray_origin.y, ray_origin.z, 
           ray_dir.x, ray_dir.y, ray_dir.z);
    printf("Cube bounds: min=%.2f, max=%.2f\n", min_bound, max_bound);
    
    // Ray-box intersection test
    float t_min = INFINITY;
    int hit_face = -1;
    
    // Test each face of the cube
    // Right face (+X)
    if (ray_dir.x != 0.0f) {
        float t = (max_bound - ray_origin.x) / ray_dir.x;
        if (t > 0.0f) {
            vec3 p = ray_origin + t * ray_dir;
            if (p.y >= min_bound && p.y <= max_bound && 
                p.z >= min_bound && p.z <= max_bound) {
                printf("Hit RIGHT face at (%.2f, %.2f, %.2f), t=%.2f\n", p.x, p.y, p.z, t);
                if (t < t_min) {
                    t_min = t;
                    hit_face = RIGHT;
                    intersection_point = p;
                }
            }
        }
    }
    
    // Left face (-X)
    if (ray_dir.x != 0.0f) {
        float t = (min_bound - ray_origin.x) / ray_dir.x;
        if (t > 0.0f) {
            vec3 p = ray_origin + t * ray_dir;
            if (p.y >= min_bound && p.y <= max_bound && 
                p.z >= min_bound && p.z <= max_bound) {
                printf("Hit LEFT face at (%.2f, %.2f, %.2f), t=%.2f\n", p.x, p.y, p.z, t);
                if (t < t_min) {
                    t_min = t;
                    hit_face = LEFT;
                    intersection_point = p;
                }
            }
        }
    }
    
    // Top face (+Y)
    if (ray_dir.y != 0.0f) {
        float t = (max_bound - ray_origin.y) / ray_dir.y;
        if (t > 0.0f) {
            vec3 p = ray_origin + t * ray_dir;
            if (p.x >= min_bound && p.x <= max_bound && 
                p.z >= min_bound && p.z <= max_bound) {
                printf("Hit TOP face at (%.2f, %.2f, %.2f), t=%.2f\n", p.x, p.y, p.z, t);
                if (t < t_min) {
                    t_min = t;
                    hit_face = TOP;
                    intersection_point = p;
                }
            }
        }
    }
    
    // Bottom face (-Y)
    if (ray_dir.y != 0.0f) {
        float t = (min_bound - ray_origin.y) / ray_dir.y;
        if (t > 0.0f) {
            vec3 p = ray_origin + t * ray_dir;
            if (p.x >= min_bound && p.x <= max_bound && 
                p.z >= min_bound && p.z <= max_bound) {
                printf("Hit BOTTOM face at (%.2f, %.2f, %.2f), t=%.2f\n", p.x, p.y, p.z, t);
                if (t < t_min) {
                    t_min = t;
                    hit_face = BOTTOM;
                    intersection_point = p;
                }
            }
        }
    }
    
    // Front face (+Z)
    if (ray_dir.z != 0.0f) {
        float t = (max_bound - ray_origin.z) / ray_dir.z;
        if (t > 0.0f) {
            vec3 p = ray_origin + t * ray_dir;
            if (p.x >= min_bound && p.x <= max_bound && 
                p.y >= min_bound && p.y <= max_bound) {
                printf("Hit FRONT face at (%.2f, %.2f, %.2f), t=%.2f\n", p.x, p.y, p.z, t);
                if (t < t_min) {
                    t_min = t;
                    hit_face = FRONT;
                    intersection_point = p;
                }
            }
        }
    }
    
    // Back face (-Z)
    if (ray_dir.z != 0.0f) {
        float t = (min_bound - ray_origin.z) / ray_dir.z;
        if (t > 0.0f) {
            vec3 p = ray_origin + t * ray_dir;
            if (p.x >= min_bound && p.x <= max_bound && 
                p.y >= min_bound && p.y <= max_bound) {
                printf("Hit BACK face at (%.2f, %.2f, %.2f), t=%.2f\n", p.x, p.y, p.z, t);
                if (t < t_min) {
                    t_min = t;
                    hit_face = BACK;
                    intersection_point = p;
                }
            }
        }
    }
    
    if (hit_face != -1) {
        face_hit = hit_face;
        printf("Final hit face: %d at point (%.2f, %.2f, %.2f)\n", 
               hit_face, intersection_point.x, intersection_point.y, intersection_point.z);
        return true;
    }
    
    // If force_selection is true and no face was hit, choose the most front-facing face
    if (force_selection) {
        // Determine which face is most front-facing (largest dot product with view direction)
        vec3 view_dir = -ray_dir;  // Invert ray direction to get view direction
        float max_dot = -1.0f;
        int best_face = -1;
        
        // Check each face normal
        float dots[6];
        dots[RIGHT] = dot(view_dir, vec3(1, 0, 0));
        dots[LEFT] = dot(view_dir, vec3(-1, 0, 0));
        dots[TOP] = dot(view_dir, vec3(0, 1, 0));
        dots[BOTTOM] = dot(view_dir, vec3(0, -1, 0));
        dots[FRONT] = dot(view_dir, vec3(0, 0, 1));
        dots[BACK] = dot(view_dir, vec3(0, 0, -1));
        
        for (int i = 0; i < 6; i++) {
            if (dots[i] > max_dot) {
                max_dot = dots[i];
                best_face = i;
            }
        }
        
        if (best_face != -1) {
            face_hit = best_face;
            
            // Create an intersection point on that face (approximate)
            float dist = cam_distance * 0.8f;  // Use a reasonable distance
            
            // Set a default position based on the face
            switch (best_face) {
                case RIGHT: intersection_point = vec3(max_bound, 0, 0); break;
                case LEFT: intersection_point = vec3(min_bound, 0, 0); break;
                case TOP: intersection_point = vec3(0, max_bound, 0); break;
                case BOTTOM: intersection_point = vec3(0, min_bound, 0); break;
                case FRONT: intersection_point = vec3(0, 0, max_bound); break;
                case BACK: intersection_point = vec3(0, 0, min_bound); break;
            }
            
            printf("Forced selection: face %d with dot product %.2f\n", best_face, max_dot);
            return true;
        }
    }
    
    printf("No face intersection found\n");
    return false;
}

// Convert mouse coordinates to a ray in world space
void mouse_to_ray(double mouse_x, double mouse_y, vec3& ray_origin, vec3& ray_dir) {
    // Get current window size
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    
    // Normalize device coordinates (-1 to 1)
    float ndc_x = (2.0f * mouse_x) / width - 1.0f;
    float ndc_y = 1.0f - (2.0f * mouse_y) / height;
    
    // Calculate camera position
    float cam_x = cam_distance * sin(cam_theta) * cos(cam_phi);
    float cam_y = cam_distance * sin(cam_phi);
    float cam_z = cam_distance * cos(cam_theta) * cos(cam_phi);
    
    ray_origin = vec3(cam_x, cam_y, cam_z);
    
    // Calculate ray direction (simplified)
    vec3 at(0, 0, 0);
    vec3 forward = normalize(at - ray_origin);
    vec3 right = normalize(cross(vec3(0, 1, 0), forward));
    vec3 up = cross(forward, right);
    
    // Field of view factor (adjust based on your perspective settings)
    float fov_factor = tan(DegreesToRadians * 45.0f / 2.0f);
    
    // Calculate world space direction
    ray_dir = normalize(forward + right * ndc_x * fov_factor + up * ndc_y * fov_factor);
}

// Determine which face was clicked
std::pair<int, int> pick_face(double mouse_x, double mouse_y, bool force_selection) {
    // If click is outside cube bounds, return invalid face
    if (!is_point_on_cube(mouse_x, mouse_y)) {
        return std::make_pair(-1, 0);
    }
    
    // Get ray from mouse position
    vec3 ray_origin, ray_dir;
    mouse_to_ray(mouse_x, mouse_y, ray_origin, ray_dir);
    
    // Find intersection with cube
    vec3 intersection_point;
    int face_hit;
    
    if (ray_cube_intersection(ray_origin, ray_dir, intersection_point, face_hit, force_selection)) {
        // Determine layer based on intersection point
        float spacing = CUBE_SIZE + CUBE_GAP;
        float cube_size = 3.0f * CUBE_SIZE + 2.0f * CUBE_GAP;
        float min_bound = -cube_size/2.0f;
        float max_bound = cube_size/2.0f;
        
        // Convert intersection point to cubie coordinates
        int layer;
        
        switch (face_hit) {
            case RIGHT:
                layer = 1;  // Outer layer
                break;
            case LEFT:
                layer = -1;  // Outer layer
                break;
            case TOP:
                layer = 1;  // Outer layer
                break;
            case BOTTOM:
                layer = -1;  // Outer layer
                break;
            case FRONT:
                layer = 1;  // Outer layer
                break;
            case BACK:
                layer = -1;  // Outer layer
                break;
            default:
                layer = 0;
        }
        
        return std::make_pair(face_hit, layer);
    }
    
    return std::make_pair(-1, 0);
}

// Display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Calculate camera position
    float cam_x = cam_distance * sin(cam_theta) * cos(cam_phi);
    float cam_y = cam_distance * sin(cam_phi);
    float cam_z = cam_distance * cos(cam_theta) * cos(cam_phi);
    
    // Set up view matrix
    point4 eye(cam_x, cam_y, cam_z, 1.0);
    point4 at(0.0, 0.0, 0.0, 1.0);
    vec4 up(0.0, 1.0, 0.0, 0.0);
    
    mat4 view = LookAt(eye, at, up);
    
    // Calculate vertices per cubie
    std::vector<int> vertices_per_cubie;
    std::vector<int> start_indices;
    
    int start_idx = 0;
    for (const Cubie& cubie : rubiksCube.getCubies()) {
        int count = 0;
        for (int face = 0; face < 6; face++) {
            if (cubie.visible[face]) {
                count += 6; // 2 triangles * 3 vertices
            }
        }
        
        vertices_per_cubie.push_back(count);
        start_indices.push_back(start_idx);
        start_idx += count;
    }
    
    // Draw each cubie
    const std::vector<mat4>& transforms = rubiksCube.getTransforms();
    for (int i = 0; i < rubiksCube.getCubies().size(); i++) {
        const Cubie& cubie = rubiksCube.getCubies()[i];
        
        // Skip if no visible faces
        if (vertices_per_cubie[i] == 0) continue;
        
        // Start with the cubie's transformation
        mat4 model = transforms[i];
        
        // Apply rotation animation if this cubie is on the rotating face
        if (rubiksCube.isAnimating()) {
            int rotating_face = rubiksCube.getRotatingFace();
            int rotating_layer = rubiksCube.getRotatingLayer();
            float rotation_angle = rubiksCube.getRotationAngle();
            vec3 rotation_axis = rubiksCube.getRotationAxis();
            
            std::vector<int> face_indices = rubiksCube.getFaceCubies(rotating_face, rotating_layer);
            
            // Check if this cubie is on the rotating face
            if (std::find(face_indices.begin(), face_indices.end(), i) != face_indices.end()) {
                // Calculate rotation center based on face
                vec3 center(0.0f);
                float spacing = CUBE_SIZE + CUBE_GAP;
                
                // Set center based on the face and layer
                if (rotating_face == RIGHT || rotating_face == LEFT) {
                    center.x = rotating_layer * spacing;
                } else if (rotating_face == TOP || rotating_face == BOTTOM) {
                    center.y = rotating_layer * spacing;
                } else { // FRONT or BACK
                    center.z = rotating_layer * spacing;
                }
                
                // Apply rotation around the center
                mat4 T1 = Translate(-center);
                mat4 R;
                
                if (rotation_axis.x != 0.0f)
                    R = RotateX(rotation_angle * (rotation_axis.x > 0 ? 1 : -1));
                else if (rotation_axis.y != 0.0f)
                    R = RotateY(rotation_angle * (rotation_axis.y > 0 ? 1 : -1));
                else
                    R = RotateZ(rotation_angle * (rotation_axis.z > 0 ? 1 : -1));
                
                mat4 T2 = Translate(center);
                
                model = T2 * R * T1 * model;
            }
        }
        
        // Apply view transform and send to shader
        mat4 model_view = view * model;
        glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
        
        // Draw this cubie
        glDrawArrays(GL_TRIANGLES, start_indices[i], vertices_per_cubie[i]);
    }
}

// Update animation and handle changes that need to be made to the display
void update() {
    // Update cube animation
    if (rubiksCube.isAnimating()) {
        // Update animation
        rubiksCube.updateAnimation();
        
        // Regenerate geometry if animation completed
        if (!rubiksCube.isAnimating()) {
            regenerate_geometry();
        }
    }
}

// Print help information
void printHelp() {
    printf("\n===== Rubik's Cube Controls =====\n");
    printf("Mouse:\n");
    printf("  Left Drag: Rotate a slice (≈30px to trigger)\n");
    printf("  Left Click: Rotate face clockwise\n");
    printf("  Shift+Left Click: Rotate face counter-clockwise\n");
    printf("  Right Drag: Orbit camera\n");
    printf("  Mouse Wheel: Zoom in/out\n");
    printf("Keyboard:\n");
    printf("  R/r: Right face CW/CCW\n");
    printf("  L/l: Left face CW/CCW\n");
    printf("  U/u: Upper face CW/CCW\n");
    printf("  D/d: Down face CW/CCW\n");
    printf("  F/f: Front face CW/CCW\n");
    printf("  B/b: Back face CW/CCW\n");
    printf("  M/m: Middle slice (X) CW/CCW\n");
    printf("  E/e: Middle slice (Y) CW/CCW\n");
    printf("  S/s: Middle slice (Z) CW/CCW\n");
    printf("  +/-: Zoom in/out\n");
    printf("  S: Shuffle (20 random moves)\n");
    printf("  C: Reset cube\n");
    printf("  H: Show this help message\n");
    printf("  ESC or Q: Exit the program\n");
    printf("================================\n\n");
}

// Key callback
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_Q:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            case GLFW_KEY_H:
                printHelp();
                break;
            // Face rotations - clockwise
            case GLFW_KEY_R:
                if (!(mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(RIGHT, 1, true); // True means CW
                else if ((mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(RIGHT, 1, false);  // False means CCW
                break;
            case GLFW_KEY_L:
                if (!(mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(LEFT, -1, true); // True means CW
                else if ((mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(LEFT, -1, false);  // False means CCW
                break;
            case GLFW_KEY_U:
                if (!(mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(TOP, 1, true); // True means CW
                else if ((mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(TOP, 1, false);  // False means CCW
                break;
            case GLFW_KEY_D:
                if (!(mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(BOTTOM, -1, true); // True means CW
                else if ((mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(BOTTOM, -1, false);  // False means CCW
                break;
            case GLFW_KEY_F:
                if (!(mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(FRONT, 1, true); // True means CW
                else if ((mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(FRONT, 1, false);  // False means CCW
                break;
            case GLFW_KEY_B:
                if (!(mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(BACK, -1, true); // True means CW
                else if ((mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(BACK, -1, false);  // False means CCW
                break;
            // Middle slices
            case GLFW_KEY_M:
                if (!(mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(LEFT, 0, true); // True means CW
                else if ((mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(LEFT, 0, false);  // False means CCW
                break;
            case GLFW_KEY_E:
                if (!(mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(TOP, 0, true); // True means CW
                else if ((mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating())
                    rubiksCube.startRotation(TOP, 0, false);  // False means CCW
                break;
            case GLFW_KEY_S:
                if (!(mods & GLFW_MOD_SHIFT)) {
                    // Capital S for shuffle - 20 random moves
                    if (!rubiksCube.isAnimating()) {
                        rubiksCube.randomize(20);
                        printf("Performing 20 random moves...\n");
                    }
                } else if ((mods & GLFW_MOD_SHIFT) && !rubiksCube.isAnimating()) {
                    // Shift+S for middle Z slice rotation
                    rubiksCube.startRotation(FRONT, 0, false); // False means CCW
                }
                break;
            // Other controls
            case GLFW_KEY_C:  // Reset
                rubiksCube.initialize();
                regenerate_geometry();
                break;
            case GLFW_KEY_MINUS:
            case GLFW_KEY_KP_SUBTRACT:
                cam_distance = std::min(cam_distance + 0.4f, 10.0f);
                break;
            case GLFW_KEY_KP_ADD:
            case GLFW_KEY_EQUAL:  // + is on same key as = without shift
                cam_distance = std::max(cam_distance - 0.4f, 2.0f);
                break;
        }
    }
}

// Mouse button callback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    shift_pressed = (mods & GLFW_MOD_SHIFT);
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            last_x = xpos;
            last_y = ypos;
            drag_start_x = xpos;
            drag_start_y = ypos;
            
            printf("LEFT PRESS: x=%.1f, y=%.1f, shift=%d\n", xpos, ypos, shift_pressed);
            
            // Check if click is on the cube
            bool on_cube = is_point_on_cube(xpos, ypos);
            printf("Click is %s the cube\n", on_cube ? "ON" : "NOT ON");
            
            if (on_cube) {
                // Set drag mode for face rotation - force a face selection for left mouse button
                auto pick_result = pick_face(xpos, ypos, true);  // Force face selection
                drag_face = pick_result.first;
                drag_layer = pick_result.second;
                
                printf("Selected face: %d, layer: %d\n", drag_face, drag_layer);
                
                // Only set drag_started if we have a valid face
                if (drag_face >= 0 && drag_face < 6) {
                    drag_started = true;
                    is_rotating_view = false;  // Ensure we're not rotating the view
                    printf("Valid face selected, preparing for rotation\n");
                } else {
                    // Should never happen with force_selection=true
                    is_rotating_view = true;
                    printf("No valid face selected, rotating view\n");
                }
            } else {
                // Not on cube, so rotate view
                is_rotating_view = true;
                printf("Click not on cube, rotating view\n");
            }
            
            mouse_dragging = true;
        } else if (action == GLFW_RELEASE) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            double dx = xpos - drag_start_x;
            double dy = ypos - drag_start_y;
            double distance = sqrt(dx*dx + dy*dy);
            
            printf("LEFT RELEASE: x=%.1f, y=%.1f, distance=%.1f\n", xpos, ypos, distance);
            printf("drag_face=%d, is_animating=%d\n", drag_face, rubiksCube.isAnimating());
            
            // If we didn't drag much, treat as a click
            if (distance < 5.0 && is_point_on_cube(xpos, ypos) && !rubiksCube.isAnimating() && drag_face >= 0) {
                // Standard convention: no shift = CW, shift = CCW
                bool clockwise = !shift_pressed;
                printf("CLICK ROTATION: face %d, layer %d, %s\n", 
                       drag_face, drag_layer, clockwise ? "CW" : "CCW");
                rubiksCube.startRotation(drag_face, drag_layer, clockwise);
                regenerate_geometry();
            } else {
                printf("Not triggering click rotation: distance=%.1f, on_cube=%d, animating=%d, face=%d\n",
                       distance, is_point_on_cube(xpos, ypos), rubiksCube.isAnimating(), drag_face);
            }
            
            mouse_dragging = false;
            is_rotating_view = false;
            drag_started = false;
            drag_face = -1;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            last_x = xpos;
            last_y = ypos;
            
            is_rotating_view = true;
            mouse_dragging = true;
        } else if (action == GLFW_RELEASE) {
            mouse_dragging = false;
            is_rotating_view = false;
        }
    }
}

// Cursor position callback for mouse dragging
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (mouse_dragging) {
        if (is_rotating_view) {
            // Orbit camera
            double dx = xpos - last_x;
            double dy = ypos - last_y;
            
            cam_theta -= dx * 0.01f;
            cam_phi += dy * 0.01f;
            
            // Clamp phi to avoid gimbal lock
            cam_phi = std::max(-1.5f, std::min(1.5f, cam_phi));
            
            last_x = xpos;
            last_y = ypos;
        } else if (drag_started && drag_face >= 0 && !rubiksCube.isAnimating()) {
            // Handle slice rotation based on drag direction
            double dx = xpos - drag_start_x;
            double dy = ypos - drag_start_y;
            double distance = sqrt(dx*dx + dy*dy);
            
            printf("Dragging: dx=%.1f, dy=%.1f, distance=%.1f, threshold=%.1f\n", 
                   dx, dy, distance, DRAG_THRESHOLD);
            
            if (distance > DRAG_THRESHOLD) {
                bool clockwise = false;
                
                // Determine rotation direction based on drag and face
                switch (drag_face) {
                    case RIGHT:
                        // Vertical drag for right face
                        clockwise = (dy > 0);
                        printf("RIGHT face: dy=%.1f, clockwise=%d\n", dy, clockwise);
                        break;
                    case LEFT:
                        // Vertical drag for left face
                        clockwise = (dy < 0);
                        printf("LEFT face: dy=%.1f, clockwise=%d\n", dy, clockwise);
                        break;
                    case TOP:
                        // Horizontal drag for top face
                        clockwise = (dx < 0);
                        printf("TOP face: dx=%.1f, clockwise=%d\n", dx, clockwise);
                        break;
                    case BOTTOM:
                        // Horizontal drag for bottom face
                        clockwise = (dx > 0);
                        printf("BOTTOM face: dx=%.1f, clockwise=%d\n", dx, clockwise);
                        break;
                    case FRONT:
                        // Use direction of strongest drag
                        if (fabs(dx) > fabs(dy)) {
                            // Horizontal drag - rotate around Y axis
                            clockwise = (dx < 0);
                            printf("FRONT face horizontal: dx=%.1f, clockwise=%d\n", dx, clockwise);
                        } else {
                            // Vertical drag - rotate around X axis
                            clockwise = (dy > 0);
                            printf("FRONT face vertical: dy=%.1f, clockwise=%d\n", dy, clockwise);
                        }
                        break;
                    case BACK:
                        // Use direction of strongest drag
                        if (fabs(dx) > fabs(dy)) {
                            // Horizontal drag - rotate around Y axis
                            clockwise = (dx > 0);
                            printf("BACK face horizontal: dx=%.1f, clockwise=%d\n", dx, clockwise);
                        } else {
                            // Vertical drag - rotate around X axis
                            clockwise = (dy < 0);
                            printf("BACK face vertical: dy=%.1f, clockwise=%d\n", dy, clockwise);
                        }
                        break;
                }
                
                // Apply shift modifier to invert direction if necessary
                if (shift_pressed) {
                    clockwise = !clockwise;
                    printf("Shift pressed, inverting rotation to: %s\n", clockwise ? "CW" : "CCW");
                }
                
                // Start the rotation (now using standard convention)
                printf("Starting drag rotation: face=%d, layer=%d, %s\n", 
                       drag_face, drag_layer, clockwise ? "CW" : "CCW");
                rubiksCube.startRotation(drag_face, drag_layer, clockwise);
                regenerate_geometry();
                
                // Reset drag state
                drag_started = false;
                drag_face = -1;
            }
        } else if (is_point_on_cube(xpos, ypos) && !is_rotating_view) {
            // If we lost the drag tracking for some reason, 
            // try to reacquire a face if mouse is still over the cube
            if (drag_face == -1 && !rubiksCube.isAnimating()) {
                auto pick_result = pick_face(xpos, ypos, true);
                drag_face = pick_result.first;
                drag_layer = pick_result.second;
                
                if (drag_face >= 0) {
                    drag_started = true;
                    printf("Reacquired face: %d, layer: %d\n", drag_face, drag_layer);
                }
            }
        }
    }
}

// Scroll callback for zoom
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Zoom in/out with the scroll wheel
    cam_distance = std::max(2.0f, std::min(10.0f, cam_distance - (float)yoffset * 0.4f));
}

// Window resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    
    float aspect = float(width) / height;
    mat4 projection = Perspective(45.0, aspect, 0.1, 100.0);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
}

int main() {
    // Seed the random number generator
    srand((unsigned int)time(NULL));
    
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(EXIT_FAILURE);
    }
    
    // Set up window properties
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(800, 800, "Rubik's Cube", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    // Set context and callbacks
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // Initialize GLEW
    #ifndef __APPLE__
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW initialization error: %s\n", glewGetErrorString(err));
        return 1;
    }
    #endif
    
    // Print OpenGL information
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("Vendor: %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    
    // Initialize OpenGL state
    init();
    
    // Print help information
    printHelp();
    
    // Call project-specific function
    project_done();
    
    // Animation loop
    double frameRate = 60.0;
    double currentTime, previousTime = 0.0;
    
    while (!glfwWindowShouldClose(window)) {
        currentTime = glfwGetTime();
        if (currentTime - previousTime >= 1.0/frameRate) {
            previousTime = currentTime;
            update();
        }
        
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Clean up
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &buffer);
    
    glfwTerminate();
    return 0;
}