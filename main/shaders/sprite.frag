#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D image;
uniform int textureSamplerIndex;

// set up the color a game sprite like a block or the ball, picking from the textureSampler
void main()
{    
   color = texture(image, vec2(textureSamplerIndex/10.0f - 0.01f, 0.5));
}  