#version 410 core
out vec4 outColor;
const float PI = 3.1415926535;
in vec3 normal;
in vec3 worldPos;
in vec2 texCoord;

uniform vec2 viewport;
uniform sampler2D diffTex;
uniform int  diffTexEnabled=0;
uniform sampler2D armTex;
uniform int  armTexEnabled=0;
uniform sampler2D emissionTex;
uniform int  emissionTexEnabled=0;
uniform sampler2D normalTex;
uniform int  normalTexEnabled=0;
uniform sampler2D irradianceMap;
uniform sampler3D environmentMap;
uniform sampler2D brdfMap;
uniform vec3 lightColor;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;
uniform float roughness;
uniform float metal = 0;
uniform float lightFactor = 0.1;
uniform int meshCount = 0;
//***************************************************
//            Color Space Conversion Functions
//***************************************************
float tonemap_sRGB(float u) {
	float u_ = abs(u);
	return  u_>0.0031308?( sign(u)*1.055*pow( u_,0.41667)-0.055):(12.92*u);
}
vec3 tonemap( vec3 rgb, mat3 csc, float gamma ){
	vec3 rgb_ = csc*rgb;
	if( abs( gamma-2.4) <0.01 ) // sRGB
		return vec3( tonemap_sRGB(rgb_.r), tonemap_sRGB(rgb_.g), tonemap_sRGB(rgb_.b) );
	return sign(rgb_)*pow( abs(rgb_), vec3(1./gamma) );
}
float inverseTonemap_sRGB(float u) {
	float u_ = abs(u);
	return u_>0.04045?(sign(u)*pow((u_+0.055)/1.055,2.4)):(u/12.92);
}
vec3 inverseTonemap( vec3 rgb, mat3 csc, float gamma ){
	if( abs( gamma-2.4) <0.01 ) // sRGB
		return csc*vec3( inverseTonemap_sRGB(rgb.r), inverseTonemap_sRGB(rgb.g), inverseTonemap_sRGB(rgb.b) );
	return csc*sign(rgb)*pow( abs(rgb), vec3(gamma) );
}
mat3 getTBN(vec3 N){
	vec3 Q1 = dFdx(worldPos), Q2 = dFdy(worldPos);
	vec2 st1 = dFdx(texCoord), st2 = dFdy(texCoord);
	float D = st1.s * st2.t - st2.s * st1.t;
	return mat3(normalize(Q1*st2.t - Q2*st1.t) * D, normalize(-Q1*st2.s + Q2*st1.s) * D, N);
}
vec2 VecToAngle(vec3 v){
	float theta = atan(v.z , v.x);
	float phi = atan(v.y, length(v.xz));
	return vec2(theta, phi);
}
vec2 AngleToTexCoord(vec2 tp){
	return vec2(1 - (tp.x/(2 * PI)), 0.5 - (tp.y / PI));
}
vec3 IBL(vec3 w_o, vec3 N, vec3 albedo, float roughness, vec3 F0, float metal){
	float a = roughness * roughness;
	int flip = meshCount % 2 !=0 ? 1 : -1;
	vec3 R = reflect(w_o, -N *flip);
	vec2 irrCoord = AngleToTexCoord(VecToAngle(N * flip));
	vec2 envCoord = AngleToTexCoord(VecToAngle(R * flip));
	vec3 diff = mix(albedo, vec3(0), metal)* texture(irradianceMap,irrCoord).rgb * lightFactor;
	vec3 spec = texture(environmentMap, vec3(envCoord,clamp(a, 0.1, 0.95))).rgb * lightFactor;
	vec3 brdf = texture(brdfMap, vec2(dot(N,w_o), a)).rgb;
	spec = spec * (mix(F0,albedo,metal)*brdf.x + brdf.y);
	return diff + spec;
}
void main() {
	vec3 faceN = normalize( cross( dFdx(worldPos), dFdy(worldPos) ) );
	vec3 N = normalize(normal);
	vec3 toLight = lightPosition-worldPos;
	vec3 w_i = normalize( toLight );
	vec3 w_o = normalize( cameraPosition - worldPos );
	if( dot(N,faceN) <0 ) N = -N;
	vec3 F0 = vec3(0.039);
	vec4 albedo = vec4(1);
	vec3 arm;
	vec4 emission = vec4(0);
	arm.r = 1;
	arm.g = roughness;
	arm.b = metal;
	if( diffTexEnabled>0 )
		albedo = texture( diffTex, texCoord );
	if( armTexEnabled > 0)
		arm = texture(armTex, texCoord).rgb;
	if( emissionTexEnabled > 0){
		emission = texture(emissionTex, texCoord);
	}
	mat3 tbn = getTBN(N);
	if(normalTexEnabled > 0){
		N = normalize(tbn * (texture(normalTex, texCoord).rgb * 2 - vec3(1)));
	}
	albedo.rgb = inverseTonemap(albedo.rgb, mat3(1), 2.4);
	vec3 Li = lightColor/dot(toLight,toLight);
	vec4 color = vec4(0,0,0,1);
	color.rgb = IBL(w_o, N, albedo.rgb, arm.g, F0, arm.b)*arm.r;
	color.a = albedo.a;
	outColor = vec4(tonemap(color.rgb,mat3(1),2.4),color.a);
}

