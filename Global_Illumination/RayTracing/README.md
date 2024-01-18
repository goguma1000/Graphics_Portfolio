# Ray Tracing
## Demo
![Alt text](https://private-user-images.githubusercontent.com/102130574/297830594-b79f319c-a3f7-4266-a8de-e93b73784ec1.png?jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3MDU2MDAzMjcsIm5iZiI6MTcwNTYwMDAyNywicGF0aCI6Ii8xMDIxMzA1NzQvMjk3ODMwNTk0LWI3OWYzMTljLWEzZjctNDI2Ni1hOGRlLWU5M2I3Mzc4NGVjMS5wbmc_WC1BbXotQWxnb3JpdGhtPUFXUzQtSE1BQy1TSEEyNTYmWC1BbXotQ3JlZGVudGlhbD1BS0lBVkNPRFlMU0E1M1BRSzRaQSUyRjIwMjQwMTE4JTJGdXMtZWFzdC0xJTJGczMlMkZhd3M0X3JlcXVlc3QmWC1BbXotRGF0ZT0yMDI0MDExOFQxNzQ3MDdaJlgtQW16LUV4cGlyZXM9MzAwJlgtQW16LVNpZ25hdHVyZT0yN2I1NWU2YzgzMDBlYTE1N2Q4YmY2YzVlOGFhNTRlMDdmOWI0MTNmMTM5YWMxMDYwNjJkMTQ3YTk0NDRmNWFjJlgtQW16LVNpZ25lZEhlYWRlcnM9aG9zdCZhY3Rvcl9pZD0wJmtleV9pZD0wJnJlcG9faWQ9MCJ9.BHHkYgms_w17sJRWkCCnQS5-c8zeziWAyxwk3oBI_Yw)

## code

### Sphere
~~~c++
virtual IntersectionData intersection(const Ray& ray)const override {
		float a = dot(ray.d, ray.d);
		float b = 2 * dot(ray.p - c, ray.d);
		float d = dot(ray.p - c, ray.p - c) - r * r;
		float D = b * b - 4 * a * d;
		if (D < 0) { return IntersectionData(-1); }

		float t1 = (-b + sqrt(D)) / (2 * a);
		float t2 = (-b - sqrt(D)) / (2 * a);
		IntersectionData ret(-1);
		if (t1 <= 0 && t2 <= 0) ret.t = -1;
		else if (t1 * t2 <= 0) ret.t = max(t1, t2);
		else ret.t = min(t1, t2);
		if (ret.t > 0) {
			ret.p = ray.p + ray.d * ret.t;
			ret.N = normalize(ret.p - c);
			ret.color = color;
		}
		return ret;
}
~~~

참고문헌: [Line-sphere intersection -Wikipedia]("https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection")

### Plane
~~~c++
virtual IntersectionData intersection(const Ray& ray) const override {
		IntersectionData ret(-1);
		float div = dot(ray.d, N);
		if (abs(div) < 0.0001) return ret;
		ret.t = dot(p - ray.p, N) / div;
		if (ret.t > 0) {
			ret.p = ray.p + ray.d * ret.t;
			ret.color = color;
			ret.N = N;
		}
		return ret;
}
~~~
참고문헌: [Line-lane intersection -Wikipedia]("https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection")

### Triangle
~~~c++
Triangle(const vec3& aa, const vec3& bb, const vec3& cc, const vec3& colour = vec3(255, 51, 51) / float(255)) : a(aa), b(bb), c(cc), color(colour) {
		N = normalize(cross(normalize(bb - aa), normalize(cc - aa)));
		P = mat4(vec4(bb - aa, 0), vec4(cc - aa, 0), vec4(N, 0), vec4(aa, 1));
		P = transpose(P);
		P = inverse(P);
		vec4 te = vec4(3, 2, 1, 1) * P;

}

virtual IntersectionData intersection(const Ray& ray) const override {
		IntersectionData ret(-1);
		vec4 o = vec4(ray.p, 1) * P;
		vec4 n = vec4(ray.d, 0) * P;
		float t = -o.z / n.z;
		float u = o.x + t * n.x;
		float v = o.y + t * n.y;
		if (u >= 0 && v >= 0 && u + v <= 1 && t >= 0) {
			ret.color = color;
			ret.N = N;
			ret.p = a;
			ret.t = t;
		}
		return ret;
}
~~~

참고문헌: [A Beautiful Ray/Triangle Intersection Method]("https://tavianator.com/2014/ray_triangle.html")

### Ray cast
~~~c++
IntersectionData IntersectionTest(const Ray& ray) {
	IntersectionData closest(-1);
	for (auto o : objects) {
		IntersectionData d = o->intersection(ray);
		if (d.t > 0) {
			if (closest.t < 0 || closest.t > d.t) closest = d;
		}
	}
	return closest;
}

vec3 castRay(const Ray& ray, int depth) {
	IntersectionData closest = IntersectionTest(ray);
	vec3 ret = vec3(0);
	if (depth <= 0) return ret;
	if (closest.t > 0) {
		for (auto l : lights) {//shadow ray
			Ray ray(closest.p + closest.N * 0.0001, l->p - closest.p);
			IntersectionData data = IntersectionTest(ray);
			if (data.t < 0 || length(data.p - ray.p) > length(l->p - ray.p)) {
				vec3 L = l->p - closest.p;
				float diff = l->i / dot(L, L) * max(dot(closest.N, normalize(L)), 0);
				vec3 R = reflect(normalize(-L), closest.N);
				vec3 V = normalize(cameraPosition - closest.p);
				float spec = jm::pow(max(dot(R, V), 0), 20.f) * l->i;
				ret += closest.color * l->color * (diff + spec / dot(L, L));
			}
		}

		vec3 R = reflect(ray.d, closest.N);
		vec3 ref = castRay(Ray(closest.p + closest.N * 0.0001, R), depth - 1) * 0.3;
		ret += ref;
	}
	return ret;
}
~~~
Scene안에 있는 모든 object와 ray가 만나는지 확인하고 가장 가까이서 만난 오브젝트의 정보를 받는다.<br>
어떤 오브젝트와 ray가 만나면 light방향으로 shadow ray를 발사하고 object들과 만나지 않으면 color값을 업데이트 한다. 이때 Phong shading을 사용하여 렌더링 한다.<br>
그 후 reflection 방향으로 ray를 발사한다.

### Render
~~~c++
void render() {
	mat4 view = lookAt(cameraPosition, vec3(0, 0, 0), vec3(0, 1, 0));
	float aspect = IMAGE_W / float(IMAGE_H);
	mat4 viewInv = inverse(view);
	for (int i = 0; i < 30; i++) {//multi-sampling
		for (int y = 0; y < IMAGE_H; y++) {
			for (int x = 0; x < IMAGE_W; x++) {//screen coordinate
				float xoff = rand() / float(RAND_MAX) - 0.5f;
				float yoff = rand() / float(RAND_MAX) - 0.5f;
				vec3 pixel = vec3(((x + xoff) / float(IMAGE_W) * 2 - 1) * aspect, (y + yoff) / float(IMAGE_H) * 2 - 1, -2);
				pixel = viewInv * vec4(pixel, 1);
				Ray ray(cameraPosition, pixel - cameraPosition);
				buffer[y * IMAGE_W + x] += vec4(castRay(ray, 4), 1);
			}
		}
	}
}
~~~
Aliasing 문제를 완화하기 위해 multi-sampling을 진행하였다.
