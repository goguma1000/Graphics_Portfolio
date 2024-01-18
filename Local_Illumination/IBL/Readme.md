# IBL
## Demo
<img src = "https://github.com/goguma1000/Graphics_Portfolio/blob/main/srcIMG/IBL_Demo_1.gif?raw=true"  align = 'center'/> | <img src = "https://private-user-images.githubusercontent.com/102130574/297830700-991e01e4-a1c7-46e2-bf48-9186b52e8db1.gif?jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3MDU2MDAwODIsIm5iZiI6MTcwNTU5OTc4MiwicGF0aCI6Ii8xMDIxMzA1NzQvMjk3ODMwNzAwLTk5MWUwMWU0LWExYzctNDZlMi1iZjQ4LTkxODZiNTJlOGRiMS5naWY_WC1BbXotQWxnb3JpdGhtPUFXUzQtSE1BQy1TSEEyNTYmWC1BbXotQ3JlZGVudGlhbD1BS0lBVkNPRFlMU0E1M1BRSzRaQSUyRjIwMjQwMTE4JTJGdXMtZWFzdC0xJTJGczMlMkZhd3M0X3JlcXVlc3QmWC1BbXotRGF0ZT0yMDI0MDExOFQxNzQzMDJaJlgtQW16LUV4cGlyZXM9MzAwJlgtQW16LVNpZ25hdHVyZT1kYTE0ZmE5ZGEyMGQ5ZTEzNGFhMjMxNzE1MGY4MjlmM2UyMTg1ZDJiNzhkMWViNjhjMzcxZjU2YTlhMjFiMTYzJlgtQW16LVNpZ25lZEhlYWRlcnM9aG9zdCZhY3Rvcl9pZD0wJmtleV9pZD0wJnJlcG9faWQ9MCJ9.flzGdjpbN9IjhYdjqMAnpb4f0Gye15prhK885B043t4" width = '318' height = '180' align = "center">
---|---|

## Code
### Irradiance map

~~~ c++
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
~~~
Diffuse rendering equation에서 $\int L_i* \cos{\theta}$ 부분을 normal에 대한 함수로 만들기 위해 irradiance map 생성.
solid angle 문제 때문에 $\cos{\phi}$를 곱한다.

<br>

### Environment map

~~~ c++
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
~~~

Specular 같은 경우 sampling 결과에 따라 highlight의 존재 여부가 달라지기 때문에 highlight가 잘 보이는 부분을 더 sampling하기 위해 importance sampling을 한다.


<br>

~~~ c++
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
~~~ 

처음 normal이 (0,0,1)이라고 가정하고 (Theta, Phi)를 구하고 실제 normal과의 각도 차이만큼 회전하여 normal 주변으로 sampling한다.

### Pre-Compute BRDF

~~~ c++
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
~~~

Normal ● View와 Roughness에 따른 BRDF값을 미리 계산한다.
<br>

### Render

~~~ c++
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
~~~

Diffuse는 normal방향으로 텍스쳐를 가져오고, specular는 reflection 방향으로 텍스쳐를 가져온다.<br>
metal의 경우 diffuse가 없기 때문에 metal값을 기준으로 albedo값을 0으로 한다.<br>

