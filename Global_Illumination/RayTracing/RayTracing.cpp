#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "jm/jm.hpp"
#include "Model/Texture.hpp"
#include <vector>
#include <thread>
#pragma comment (lib, "glfw3")

using namespace AR;
using namespace jm;

Texture image;
vec4* buffer = nullptr;

struct Ray {
	vec3 p;
	vec3 d;
	Ray() {}
	Ray(const vec3& pp, const vec3& dd) : p(pp), d(normalize(dd)) { }
};
struct IntersectionData {
	float t;
	vec3 p, N, color;
	IntersectionData() {}
	IntersectionData(float tt) :t(tt) {}
};

struct Object {
	virtual IntersectionData intersection(const Ray& ray) const = 0;// java: interface c++: abstract class
};

struct Sphere : Object {
	vec3 c;
	float r;
	vec3 color;
	Sphere() {}
	Sphere(const vec3& cc, float rr, const vec3& colour = vec3(1, 0.6, 0)) :c(cc), r(rr), color(colour) { }
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
};

struct Plane : Object {
	vec3 p;
	vec3 N;
	vec3 color;
	Plane(const vec3& cc, const vec3& nn, const vec3& colour = vec3(1, 0.6, 0)) : p(cc), N(nn), color(colour) {}
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
};
struct Triangle : Object {
	vec3 a;
	vec3 b;
	vec3 c;
	vec3 N;
	mat4 P;
	vec3 color;
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
};

struct Light {
	vec3 p;
	vec3 color;
	float i;
	Light() {}
	Light(const vec3& pp, float ii, const vec3& colour = vec3(1, 1, 1)) : p(pp), color(colour), i(ii) {}
};

const char* __blit_alpha_frag = ""
"#version 410\n"
"out vec4 outColor;\n"
"in vec2 texCoord;\n"
"\n"
"uniform sampler2D diffTex;\n"
"uniform float scale = 1.f;\n"
"uniform mat3 CSC;\n"
"uniform float gamma;\n"
"\n"
"\n"
"//***************************************************\n"
"//            Color Space Conversion Functions\n"
"//***************************************************\n"
"float tonemap_sRGB(float u) {\n"
"	float u_ = abs(u);\n"
"	return  u_>0.0031308?( sign(u)*1.055*pow( u_,0.41667)-0.055):(12.92*u);\n"
"}\n"
"vec3 tonemap( vec3 rgb, mat3 csc, float gamma ){\n"
"	vec3 rgb_ = csc*rgb;\n"
"	if( abs( gamma-2.4) <0.01 ) // sRGB\n"
"		return vec3( tonemap_sRGB(rgb_.r), tonemap_sRGB(rgb_.g), tonemap_sRGB(rgb_.b) );\n"
"	return sign(rgb_)*pow( abs(rgb_), vec3(1./gamma) );\n"
"}\n"
"float inverseTonemap_sRGB(float u) {\n"
"	float u_ = abs(u);\n"
"	return u_>0.04045?(sign(u)*pow((u_+0.055)/1.055,2.4)):(u/12.92);\n"
"}\n"
"vec3 inverseTonemap( vec3 rgb, mat3 csc, float gamma ){\n"
"	if( abs( gamma-2.4) <0.01 ) // sRGB\n"
"		return csc*vec3( inverseTonemap_sRGB(rgb.r), inverseTonemap_sRGB(rgb.g), inverseTonemap_sRGB(rgb.b) );\n"
"	return csc*sign(rgb)*pow( abs(rgb), vec3(gamma) );\n"
"}\n"
"\n"
"\n"
"void main(void) {\n"
"	vec4 color = texture( diffTex, texCoord );\n"
"	color.rgb = color.rgb * scale / color.a;\n"
"	outColor = vec4( tonemap( color.rgb, CSC, gamma ), 1 );\n"
"}\n"
;

AutoBuildProgram __blit_alpha_Program__(__blit_vert, __blit_alpha_frag);
const int IMAGE_W = 800;
const int IMAGE_H = 600;
vec3 cameraPosition = vec3(0, 0, 8);

std::vector<Object*> objects;
std::vector<Light*> lights;

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

int main(int argc, const char* argv[]) {
	if (!glfwInit()) {
		printf("FAil\n");
		exit(EXIT_FAILURE);
	}

#ifdef __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif

	glfwWindowHint(GLFW_SAMPLES, 32);
	GLFWwindow* window = glfwCreateWindow(800, 600, "Hello", NULL, NULL);
	glfwMakeContextCurrent(window);
#ifndef __APPLE__
	glewInit();
#endif
	vec3 p1 = vec3(-1, 1.2, 3);
	vec3 p2 = vec3(1, 1.2, 3);
	vec3 p3 = vec3(0, 2.2, 3.5);
	vec3 p4 = vec3(0, 1.2, 4);
	vec3 LColor = vec3(255, 153, 0) / float(255);
	vec3 RColor = vec3(0, 102, 255) / float(255);

	objects.push_back(new Sphere(vec3(0, 0, 2), 1, vec3(0.8, 0, 0.15)));
	objects.push_back(new Sphere(vec3(-3, 0, 2), 1, vec3(0.3, 0.5, 0)));
	objects.push_back(new Sphere(vec3(3, 0, 2), 1, vec3(1, 0.2, 0)));
	objects.push_back(new Plane(vec3(0, -1, 0), vec3(0, 1, 0), vec3(0.9, 0.9, 0.9)));
	objects.push_back(new Plane(vec3(5, 0, 0), vec3(-1, 0, 0), RColor));
	objects.push_back(new Plane(vec3(-5, 0, 0), vec3(1, 0, 0), LColor));
	objects.push_back(new Triangle(p1, p2, p3));
	objects.push_back(new Triangle(p1, p4, p2));
	objects.push_back(new Triangle(p1, p4, p3));
	objects.push_back(new Triangle(p2, p3, p4));
	for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++) { //area light
		lights.push_back(new Light(vec3(-4, 10 + x / 4.f - 0.5f, 4 + y / 4.f - 0.5f), 1));
		lights.push_back(new Light(vec3(4, 10 + x / 4.f - 0.5f, 10 + y / 4.f - 0.5f), 1));
	}
	buffer = new vec4[IMAGE_W * IMAGE_H];
	image.create(IMAGE_W, IMAGE_H, 4, GL_FLOAT, buffer);

	std::thread renderThread([&]() {render(); });
	renderThread.detach();
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	while (!glfwWindowShouldClose(window)) {
		int fw, fh, ww, wh;
		glfwGetFramebufferSize(window, &fw, &fh);
		glfwGetWindowSize(window, &ww, &wh);


		image.update(buffer);
		glViewport(0, 0, fw, fh);

		//		image.blit();
		__blit_alpha_Program__.use();
		image.bind(0, __blit_alpha_Program__, "diffTex");
		__blit_alpha_Program__.setUniform("modelMat", mat4(1));
		__blit_alpha_Program__.setUniform("scale", 1.f);
		__blit_alpha_Program__.setUniform("CSC", mat3(1));
		__blit_alpha_Program__.setUniform("gamma", 2.4f);
		TriMesh::renderQuad(__blit_alpha_Program__);

		glFlush();
		glFinish();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
