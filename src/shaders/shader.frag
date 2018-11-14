#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 fColor;
layout (location = 1) in vec2 fTexCoords;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 FragColor;

void main() {
	//FragColor = vec4(fTexCoords, 0.0, 1.0);
	FragColor = texture(texSampler, fTexCoords);
}