#version 410 core
out vec4 outColor;
const float PI = 3.1415926535;

in vec2 texCoord;

uniform sampler2D srcIMG;
uniform float roughness;

const int nSamples = 100;

vec2 TexCoordToAngle(vec2 txCoord){
	return vec2(2*PI*(1 - txCoord.x), PI*(0.5f - txCoord.y));
}

vec2 AngleToTexCoord(vec2 tp){
	return vec2(1 - (tp.x/(2 * PI)), 0.5 - (tp.y / PI));
}

vec3 AngleToVec(vec2 tp){
	vec3 ret = vec3(0);
	ret.x = cos(tp.y)*cos(tp.x);
	ret.y = sin(tp.y);
	ret.z = cos(tp.y)*sin(tp.x);
	return ret;
}
vec2 VecToAngle(vec3 v){
	float theta = atan(v.z , v.x);
	float phi = atan(v.y, length(v.xz));
	return vec2(theta, phi);
}


vec3 ImportanceSampeGGX(vec2 xi, float a, vec3 N){
	float Theta = 2 * PI * xi.x;
	float CosPhi = sqrt((1 - xi.y) / (1 + (a * a - 1) * xi.y));
	float SinPhi = sqrt(1 - CosPhi * CosPhi);
	vec3 H = vec3(SinPhi * cos(Theta), SinPhi * sin(Theta), CosPhi);
	float cosb = N.z;
	float sinb = sqrt(1 - cosb * cosb);
	vec3 k = normalize(cross(vec3(0,0,1), N));
	return H * cosb + cross(k, H) * sinb + k * dot(k,H) * (1 - cosb);
}
vec3 IntegrateIBL(vec3 N){
	vec3 sum = vec3(0);
	float wsum = 0;
	for(int i = 0; i < nSamples; i++) for(int j = 0; j < nSamples; j++){
		vec2 xi = vec2(i / float(nSamples), j / float(nSamples));
		vec3 H = ImportanceSampeGGX(xi, roughness * roughness, N);
		vec3 L = 2 * dot(N, H) * H - N;
		float w = dot(N,H);
		sum += texture(srcIMG, AngleToTexCoord(VecToAngle(L))).rgb * w;
		wsum += w;
	}
	return sum / wsum; //normalize
}

void main() {
	vec3 color = IntegrateIBL(AngleToVec(TexCoordToAngle(texCoord)));
	outColor = vec4(color.rgb,1);
}

