#version 410 core
out vec4 outColor;
const float PI = 3.1415926535;

in vec3 world_Pos;

const vec2 invAtan = vec2(0.1591, 0.3183);
uniform sampler2D srcIMG;

vec2 SampleHDR_IMG(vec3 v){
	vec2 uv = vec2(atan(v.z, v.x), sin(v.y));
	uv *=invAtan;
	uv += 0.5;
	return uv;
}
void main() {
	vec2 uv = SampleHDR_IMG(normalize(world_Pos));
	vec3 color = texture(srcIMG, uv).rgb;
	outColor = vec4(color,1);
}

