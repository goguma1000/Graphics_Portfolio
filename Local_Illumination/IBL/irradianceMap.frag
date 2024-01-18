#version 410 core
out vec4 outColor;
const float PI = 3.1415926535;

in vec2 texCoord;

uniform sampler2D srcIMG;

const int nSamples = 100;

vec2 TexCoordToAngle(vec2 txCoord){
	return vec2(2*PI*(1 - txCoord.x), PI*(0.5f - txCoord.y));
}

vec3 AngleToVec(vec2 tp){
	vec3 ret = vec3(0);
	ret.x = cos(tp.y)*cos(tp.x);
	ret.y = sin(tp.y);
	ret.z = cos(tp.y)*sin(tp.x);
	return ret;
}

vec3 IntegrateIBL(vec3 N){
	vec3 sum = vec3(0);
	float wsum = 0;
	for(int i = 0; i < nSamples; i++) for(int j = 0; j < nSamples; j++){
		vec2 p = vec2(i / float(nSamples), j / float(nSamples));
		vec2 angle = TexCoordToAngle(p);
		vec3 L = AngleToVec(angle);
		float w =  max(dot(N,L),0);
		sum += texture(srcIMG,p).rgb * w * cos(angle.y);
		wsum += w;
	}
	return sum / wsum; //normalize
}

void main() {
	vec3 color = IntegrateIBL(AngleToVec(TexCoordToAngle(texCoord)));
	outColor = vec4(color.rgb,1);
}

