#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D image;
uniform vec3 spriteColor;

// set up the color a game sprite like a block or the ball
void main()
{    
    color = vec4(spriteColor, 1.0) * texture(image, TexCoords);
}  