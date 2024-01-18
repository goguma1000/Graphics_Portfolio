#version 410 core
out vec4 outColor;
const float PI = 3.1415926535;

in vec2 texCoord;

vec3 ImportanecSampleGGX(vec2 xi, float a){
	float phi = 2 * PI * xi.x;
	float CosTheta = sqrt((1 - xi.y) / (1 + (a * a - 1)* xi.y));
	float SinTheta = sqrt(1 - CosTheta * CosTheta);
	vec3 H = vec3(SinTheta*cos(phi), SinTheta * sin(phi), CosTheta);
	return H ; //Rodrigues equation
}

const int nSamples = 100;
float X(float v){
	return v >= 0 ? 1 : 0;
}
vec3 integrateIBL(float a ,float NoV){
	vec3 sum = vec3(0);
	for(int i = 0; i < nSamples; i++) for(int j = 0; j < nSamples; j++){
		vec2 p = vec2(i / float(nSamples), j / float(nSamples));
		vec3 N = vec3(0,0,1);
		vec3 v = vec3(sqrt(1-NoV*NoV),0,NoV);
		vec3 h = ImportanecSampleGGX(p, a);
		vec3 l = 2 * dot(v, h) * h - v;
		float VoH = dot(v,h);
		float NoH = h.z;
		if(l.z > 0){
			float tn2 = (1-NoV*NoV)/(NoV*NoV);
			float G = X(VoH/NoV) * (2/(1 + sqrt(1 + a*a*tn2)));
			float f_c = pow((1 - VoH),5);
			sum += vec3(1-f_c,f_c,0)*G*VoH/(NoH*NoV);
		}
	}
	return sum/float(nSamples*nSamples);
}

void main() {
	vec3 color = integrateIBL(texCoord.y,texCoord.x);
	outColor = vec4(color.rgb,1);
} 

