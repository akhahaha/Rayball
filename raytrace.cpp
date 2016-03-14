#define _CRT_SECURE_NO_WARNINGS

#include "matm.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

// Program constants
#define MAX_SPHERES 5
#define MAX_LIGHTS 5
#define MIN_HIT_TIME 1.0f

// STRUCTURES
struct Ray {
    vec4 origin;
    vec4 dir;
};

struct Sphere {
    string id;
    vec4 position;
    vec3 scale;
    vec4 color;
    float Ka;
    float Kd;
    float Ks;
    float Kr;
    float specularExponent;
};

struct Light {
    string id;
    vec4 position;
    vec4 color;
};

struct Intersection {
    Ray ray;
    float distance;
    vec4 point;
    Sphere *sphere;
    vec4 normal;
};


vector<vec4> g_colors;

// SCENE DATA
// Planes
float g_near;
float g_left;
float g_right;
float g_top;
float g_bottom;

// Resolution
int g_width;
int g_height;

vector<Sphere> g_spheres;
vector<Light> g_lights;
vec4 g_backgroundColor;
vec4 g_ambientIntensity;
string g_outputFilename;


// -------------------------------------------------------------------
// Input file parsing

vec4 toVec4(const string &s1, const string &s2, const string &s3) {
    stringstream ss(s1 + " " + s2 + " " + s3);
    vec4 result;
    ss >> result.x >> result.y >> result.z;
    result.w = 1.0f;
    return result;
}

float toFloat(const string &s) {
    stringstream ss(s);
    float f;
    ss >> f;
    return f;
}

enum Datatypes {
    NEAR,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    RES,
    SPHERE,
    LIGHT,
    BACK,
    AMBIENT,
    OUTPUT
};
static map<string, Datatypes> s_datatypes;

void initializeDatatypes() {
    s_datatypes["NEAR"] = NEAR;
    s_datatypes["LEFT"] = LEFT;
    s_datatypes["RIGHT"] = RIGHT;
    s_datatypes["TOP"] = TOP;
    s_datatypes["BOTTOM"] = BOTTOM;
    s_datatypes["RES"] = RES;
    s_datatypes["SPHERE"] = SPHERE;
    s_datatypes["LIGHT"] = LIGHT;
    s_datatypes["BACK"] = BACK;
    s_datatypes["AMBIENT"] = AMBIENT;
    s_datatypes["OUTPUT"] = OUTPUT;
}

void parseLine(const vector<string> &vs) {
    switch (s_datatypes[vs[0]]) {
        case NEAR:
            g_near = toFloat(vs[1]);
            break;
        case LEFT:
            g_left = toFloat(vs[1]);
            break;
        case RIGHT:
            g_right = toFloat(vs[1]);
            break;
        case TOP:
            g_top = toFloat(vs[1]);
            break;
        case BOTTOM:
            g_bottom = toFloat(vs[1]);
            break;
        case RES:
            g_width = (int) toFloat(vs[1]);
            g_height = (int) toFloat(vs[2]);
            g_colors.resize(g_width * g_height);
            break;
        case SPHERE:
            if (g_spheres.size() < MAX_SPHERES) {
                Sphere sphere;
                sphere.id = vs[1];
                sphere.position = toVec4(vs[2], vs[3], vs[4]);
                sphere.scale = vec3(toFloat(vs[5]), toFloat(vs[6]), toFloat(vs[7]));
                sphere.color = toVec4(vs[8], vs[9], vs[10]);
                sphere.Ka = toFloat(vs[11]);
                sphere.Kd = toFloat(vs[12]);
                sphere.Ks = toFloat(vs[13]);
                sphere.Kr = toFloat(vs[14]);
                sphere.specularExponent = toFloat(vs[15]);

                g_spheres.push_back(sphere);
            }
            break;
        case LIGHT:
            if (g_lights.size() < MAX_LIGHTS) {
                Light light;
                light.id = vs[1];
                light.position = toVec4(vs[2], vs[3], vs[4]);
                light.color = toVec4(vs[5], vs[6], vs[7]);

                g_lights.push_back(light);
            }
            break;
        case BACK:
            g_backgroundColor = toVec4(vs[1], vs[2], vs[3]);
            break;
        case AMBIENT:
            g_ambientIntensity = toVec4(vs[1], vs[2], vs[3]);
            break;
        case OUTPUT:
            g_outputFilename = vs[1];
            break;
    }
}

void loadFile(const char *filename) {
    ifstream is(filename);
    if (is.fail()) {
        cout << "Could not open file " << filename << endl;
        exit(1);
    }
    string s;
    vector<string> vs;
    while (!is.eof()) {
        vs.clear();
        getline(is, s);
        istringstream iss(s);
        while (!iss.eof()) {
            string sub;
            iss >> sub;
            vs.push_back(sub);
        }
        parseLine(vs);
    }
}


// -------------------------------------------------------------------
// Utilities

void setColor(int ix, int iy, const vec4 &color) {
    int iy2 = g_height - iy - 1; // Invert iy coordinate.
    g_colors[iy2 * g_width + ix] = color;
}

/**
 * Determine the nearest sphere intersection of a ray.
 */
Intersection calculateNearestIntersection(const Ray &ray) {
    float nearestIntersectionDistance = -1;
    Sphere *intersectedSphere = nullptr;
    for (Sphere &sphere : g_spheres) {
        // Obtain the sphere inverse transform
        mat4 sphereIVT;
        InvertMatrix(Scale(sphere.scale), sphereIVT);

        vec4 S = sphereIVT * (sphere.position - ray.origin); // -(O - C)
        vec4 C = sphereIVT * ray.dir;

        // Quadratic equation: |c|^2t^2 + 2(S.tc) + |S|^2 - 1
        float a = dot(C, C);
        float b = dot(S, C);
        float c = dot(S, S) - 1;

        // Solve equation
        float solution = -1;
        float discriminant = b * b - a * c; // Value under the root

        if (discriminant < 0) {
            // No solutions: line does not intersect
            continue;
        } else if (discriminant == 0) {
            // Single solution: line intersects at one point
            solution = b / a;
        } else {
            // Two solutions: line intersects at two points
            float root = sqrtf(discriminant);
            solution = fminf((b - root) / a, (b + root) / a);
        }

        if (solution > MIN_HIT_TIME && (nearestIntersectionDistance == -1 || solution < nearestIntersectionDistance)) {
            nearestIntersectionDistance = solution;
            intersectedSphere = &sphere;
        }
    }

    Intersection intersection;
    intersection.ray = ray;
    intersection.distance = nearestIntersectionDistance;
    if (nearestIntersectionDistance != -1) {
        intersection.point = ray.origin + ray.dir * nearestIntersectionDistance;
        intersection.sphere = intersectedSphere;

        // Calculate the normal of the intersection
        vec4 normal = intersection.point - intersection.sphere->position;
        mat4 sphereTransposeIVT;
        InvertMatrix(transpose(Scale(intersection.sphere->scale)), sphereTransposeIVT);
        normal.w = 0;
        intersection.normal = normalize(sphereTransposeIVT * sphereTransposeIVT * normal);
    }

    return intersection;
}

/**
 * Trace the color of a ray.
 */
vec4 trace(const Ray &ray) {
    vec4 color = g_backgroundColor;

    // Get the nearest intersecting sphere
    Intersection intersection = calculateNearestIntersection(ray);
    if (intersection.distance != -1) {
        // Calculate initial intersection color with ambient intensity
        color = intersection.sphere->color * intersection.sphere->Ka * g_ambientIntensity;

        // Calculate Blinn-Phong shading from light sources
        vec4 diffusion = vec4(0, 0, 0, 0);
        vec4 specular = vec4(0, 0, 0, 0);
        for (Light light : g_lights) {
            // Generate ray from intersection point to light
            Ray lightRay;
            lightRay.origin = intersection.point;
            lightRay.dir = normalize(light.position - intersection.point);

            // Determine if the light source is not obstructed
            Intersection lightIntersection = calculateNearestIntersection(lightRay);
            if (lightIntersection.distance == -1) {
                // Calculate the intensity of diffuse light
                float diffusionIntensity = dot(intersection.normal, lightRay.dir);
                if (diffusionIntensity > 0) {
                    diffusion += diffusionIntensity * light.color * intersection.sphere->color;

                    // Calculate the half vector between light vector and the view vector
                    vec4 H = normalize(lightRay.dir - ray.dir);

                    // Calculate the intensity of specular light
                    float specularIntensity = dot(intersection.normal, H);
                    specular += powf(powf(specularIntensity, intersection.sphere->specularExponent), 3) * light.color;
                }
            }
        }

        // Apply diffusion and specular values
        color += diffusion * intersection.sphere->Kd + specular * intersection.sphere->Ks;
    }

    return color;
}

/**
 * Return the direction from the origin to a pixel.
 */
vec4 getDir(int ix, int iy) {
    float x = g_left + ((float) ix / g_width) * (g_right - g_left);
    float y = g_bottom + ((float) iy / g_height) * (g_top - g_bottom);
    return vec4(x, y, -g_near, 0.0f);
}

void renderPixel(int ix, int iy) {
    Ray ray;
    ray.origin = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    ray.dir = getDir(ix, iy);
    vec4 color = trace(ray);
    setColor(ix, iy, color);
}

void render() {
    for (int iy = 0; iy < g_height; iy++)
        for (int ix = 0; ix < g_width; ix++)
            renderPixel(ix, iy);
}


// -------------------------------------------------------------------
// PPM saving

void savePPM(int Width, int Height, char *fname, unsigned char *pixels) {
    FILE *fp;
    const int maxVal = 255;

    printf("Saving image %s: %d x %d\n", fname, Width, Height);
    fp = fopen(fname, "wb");
    if (!fp) {
        printf("Unable to open file '%s'\n", fname);
        return;
    }
    fprintf(fp, "P6\n");
    fprintf(fp, "%d %d\n", Width, Height);
    fprintf(fp, "%d\n", maxVal);

    for (int j = 0; j < Height; j++) {
        fwrite(&pixels[j * Width * 3], 3, Width, fp);
    }

    fclose(fp);
}

void saveFile() {
    // Convert color components from floats to unsigned chars.
    // TODO: clamp values if out of range.
    unsigned char *buf = new unsigned char[g_width * g_height * 3];
    for (int y = 0; y < g_height; y++)
        for (int x = 0; x < g_width; x++)
            for (int i = 0; i < 3; i++)
                buf[y * g_width * 3 + x * 3 + i] = (unsigned char) (((float *) g_colors[y * g_width + x])[i] * 255.9f);

    // TODO: change file name based on input file name.
    savePPM(g_width, g_height, "output.ppm", buf);
    delete[] buf;
}


// -------------------------------------------------------------------
// Main

int main(int argc, char *argv[]) {
    initializeDatatypes();

    if (argc < 2) {
        cout << "Usage: template-rt <input_file.txt>" << endl;
        exit(1);
    }
    loadFile(argv[1]);
    render();
    saveFile();
    return 0;
}

