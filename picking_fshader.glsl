#version 330 core

// Input from vertex shader
in vec4 pickColor;

// Output color
out vec4 fragColor;

void main()
{
    // Output the picking color directly
    fragColor = pickColor;
}