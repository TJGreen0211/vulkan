#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fColor;
layout(location = 1) out vec2 fTexCoord;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = vec4(inPosition, 1.0) * ubo.model;// * ubo.view * ubo.proj;
	fColor = inColor;//vec3(ubo.model[0][0], 0.0, 0.0);
	fTexCoord = inTexCoord;
}