#version 330 core

// Input vertex attributes
in vec4 vPosition;
in vec4 vPickingColor;

// Output to fragment shader
out vec4 pickColor;

// Matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int cube_id;

void main()
{
    // Calculate final position
    gl_Position = projection * view * model * vPosition;
    
    // Set the picking color
    // Red channel: face ID from vPickingColor
    // Green channel: cube ID from uniform
    pickColor = vec4(vPickingColor.r, float(cube_id) / 255.0, 0.0, 1.0);
}