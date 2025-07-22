#include <math.h>   // smallpt, a Path Tracer by Kevin Beason, 2008
#include <stdio.h>  //        Remove "-fopenmp" for g++ version < 4.2
#include <stdlib.h> // Make : g++ -O3 -fopenmp smallpt.cpp -o smallpt
#include <sstream>
#include <vector>

#include "crow_all.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct Vec {        // Usage: time ./smallpt 5000 && xv image.ppm
  double x, y, z;   // position, also color (r,g,b)
  Vec(double x_ = 0, double y_ = 0, double z_ = 0) {
    x = x_;
    y = y_;
    z = z_;
  }
  Vec operator+(const Vec &b) const { return Vec(x + b.x, y + b.y, z + b.z); }
  Vec operator-(const Vec &b) const { return Vec(x - b.x, y - b.y, z - b.z); }
  Vec operator*(double b) const { return Vec(x * b, y * b, z * b); }
  Vec mult(const Vec &b) const { return Vec(x * b.x, y * b.y, z * b.z); }
  Vec &norm() { return *this = *this * (1 / sqrt(x * x + y * y + z * z)); }
  double dot(const Vec &b) const {
    return x * b.x + y * b.y + z * b.z;
  } // cross:
  Vec operator%(Vec &b) {
    return Vec(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x);
  }
};

struct Ray {
  Vec o, d;
  Ray(Vec o_, Vec d_) : o(o_), d(d_) {}
};

enum Refl_t { DIFF, SPEC, REFR }; // material types, used in radiance()

struct Sphere {
  double rad;  // radius
  Vec p, e, c; // position, emission, color
  Refl_t refl; // reflection type (DIFFuse, SPECular, REFRactive)
  Sphere(double rad_, Vec p_, Vec e_, Vec c_, Refl_t refl_)
      : rad(rad_), p(p_), e(e_), c(c_), refl(refl_) {}
  double intersect(const Ray &r) const { // returns distance, 0 if nohit
    Vec op = p - r.o; // Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0
    double t, eps = 1e-4, b = op.dot(r.d), det = b * b - op.dot(op) + rad * rad;
    if (det < 0)
      return 0;
    else
      det = sqrt(det);
    return (t = b - det) > eps ? t : ((t = b + det) > eps ? t : 0);
  }
};

// Global spheres array - will be updated with parameters
std::vector<Sphere> spheres;

inline double clamp(double x) { return x < 0 ? 0 : x > 1 ? 1 : x; }

inline int toInt(double x) { return int(pow(clamp(x), 1 / 2.2) * 255 + .5); }

inline bool intersect(const Ray &r, double &t, int &id) {
  double n = spheres.size(), d, inf = t = 1e20;
  for (int i = int(n); i--;)
    if ((d = spheres[i].intersect(r)) && d < t) {
      t = d;
      id = i;
    }
  return t < inf;
}

Vec radiance(const Ray &r, int depth, unsigned short *Xi) {
  double t;   // distance to intersection
  int id = 0; // id of intersected object
  if (!intersect(r, t, id)) {
    return Vec();                  // if miss, return black
  }
  const Sphere &obj = spheres[id]; // the hit object
  Vec x = r.o + r.d * t, n = (x - obj.p).norm(),
      nl = n.dot(r.d) < 0 ? n : n * -1, f = obj.c;
  double p = f.x > f.y && f.x > f.z ? f.x : f.y > f.z ? f.y : f.z; // max refl
  if (++depth > 5) {
    if (erand48(Xi) < p) {
      f = f * (1 / p);
    }
    else {
      return obj.e;       // R.R.
    }
  }
  if (obj.refl == DIFF) { // Ideal DIFFUSE reflection
    double r1 = 2 * M_PI * erand48(Xi), r2 = erand48(Xi), r2s = sqrt(r2);
    Vec w = nl, u = ((fabs(w.x) > .1 ? Vec(0, 1) : Vec(1)) % w).norm(), v = w % u;
    Vec d = (u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1 - r2)).norm();
    return obj.e + f.mult(radiance(Ray(x, d), depth, Xi));
  } else if (obj.refl == SPEC) { // Ideal SPECULAR reflection
    return obj.e + f.mult(radiance(Ray(x, r.d - n * 2 * n.dot(r.d)), depth, Xi));
  }
  Ray reflRay(x, r.d - n * 2 * n.dot(r.d)); // Ideal dielectric REFRACTION
  bool into = n.dot(nl) > 0;                // Ray from outside going in?
  double nc = 1, nt = 1.5, nnt = into ? nc / nt : nt / nc, ddn = r.d.dot(nl), cos2t;
  if ((cos2t = 1 - nnt * nnt * (1 - ddn * ddn)) < 0) { // Total internal reflection
    return obj.e + f.mult(radiance(reflRay, depth, Xi));
  }
  Vec tdir = (r.d * nnt - n * ((into ? 1 : -1) * (ddn * nnt + sqrt(cos2t)))).norm();
  double a = nt - nc, b = nt + nc, R0 = a * a / (b * b),
         c = 1 - (into ? -ddn : tdir.dot(n));
  double Re = R0 + (1 - R0) * c * c * c * c * c, Tr = 1 - Re, P = .25 + .5 * Re,
         RP = Re / P, TP = Tr / (1 - P);
  return obj.e +
         f.mult(depth > 2
                    ? (erand48(Xi) < P ? // Russian roulette
                           radiance(reflRay, depth, Xi) * RP
                                       : radiance(Ray(x, tdir), depth, Xi) * TP)
                    : radiance(reflRay, depth, Xi) * Re +
                          radiance(Ray(x, tdir), depth, Xi) * Tr);
}

void setupScene(double sphere1_x, double sphere1_y, double sphere1_z,
                double sphere2_x, double sphere2_y, double sphere2_z) {
    spheres.clear();
    
    // Scene walls (unchanged)
    spheres.emplace_back(1e5, Vec(1e5 + 1, 40.8, 81.6), Vec(), Vec(.75, .25, .25), DIFF); // Left
    spheres.emplace_back(1e5, Vec(-1e5 + 99, 40.8, 81.6), Vec(), Vec(.25, .25, .75), DIFF); // Right
    spheres.emplace_back(1e5, Vec(50, 40.8, 1e5), Vec(), Vec(.75, .75, .75), DIFF); // Back
    spheres.emplace_back(1e5, Vec(50, 40.8, -1e5 + 170), Vec(), Vec(), DIFF); // Front
    spheres.emplace_back(1e5, Vec(50, 1e5, 81.6), Vec(), Vec(.75, .75, .75), DIFF); // Bottom
    spheres.emplace_back(1e5, Vec(50, -1e5 + 81.6, 81.6), Vec(), Vec(.75, .75, .75), DIFF); // Top
    
    // Parametrized center spheres
    spheres.emplace_back(16.5, Vec(sphere1_x, sphere1_y, sphere1_z), Vec(), Vec(1, 1, 1) * .999, SPEC); // Mirror sphere
    spheres.emplace_back(16.5, Vec(sphere2_x, sphere2_y, sphere2_z), Vec(), Vec(1, 1, 1) * .999, REFR); // Glass sphere
    
    // Light (unchanged)
    spheres.emplace_back(600, Vec(50, 681.6 - .27, 81.6), Vec(12, 12, 12), Vec(), DIFF); // Light
}

bool renderToPNG(int samples, std::vector<unsigned char>& png_buffer) {
    int w = 1024, h = 768, samps = samples;
    Ray cam(Vec(50, 52, 295.6), Vec(0, -0.042612, -1).norm());
    Vec cx = Vec(w * .5135 / h), cy = (cx % cam.d).norm() * .5135, r, *c = new Vec[w * h];

    fprintf(stderr, "Rendering %dx%d with %d samples...\n", w, h, samps);

    #pragma omp parallel for schedule(dynamic, 1) private(r)
    for (int y = 0; y < h; y++) {
        fprintf(stderr, "\rProgress: %5.2f%%", 100. * y / (h - 1));
        for (unsigned short x = 0, Xi[3] = {0, 0, static_cast<unsigned short>(y * y * y)}; x < w; x++)
            for (int sy = 0, i = (h - y - 1) * w + x; sy < 2; sy++)
                for (int sx = 0; sx < 2; sx++, r = Vec()) {
                    for (int s = 0; s < samps; s++) {
                        double r1 = 2 * erand48(Xi), dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
                        double r2 = 2 * erand48(Xi), dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);
                        Vec d = cx * (((sx + .5 + dx) / 2 + x) / w - .5) +
                                cy * (((sy + .5 + dy) / 2 + y) / h - .5) + cam.d;
                        r = r + radiance(Ray(cam.o + d * 140, d.norm()), 0, Xi) * (1. / samps);
                    }
                    c[i] = c[i] + Vec(clamp(r.x), clamp(r.y), clamp(r.z)) * .25;
                }
    }
    fprintf(stderr, "\nRendering complete!\n");

    // Convert to RGB
    std::vector<unsigned char> image(w * h * 3);
    for (int i = 0; i < w * h; i++) {
        image[i * 3 + 0] = toInt(c[i].x);
        image[i * 3 + 1] = toInt(c[i].y);
        image[i * 3 + 2] = toInt(c[i].z);
    }

    // Convert to PNG in memory
    int out_len = 0;
    unsigned char* out_png = stbi_write_png_to_mem(
        image.data(), w * 3, w, h, 3, &out_len
    );

    bool success = false;
    if (out_png && out_len > 0) {
        png_buffer.assign(out_png, out_png + out_len);
        STBIW_FREE(out_png);
        success = true;
    }

    delete[] c;
    return success;
}

int main() {
    crow::SimpleApp app;

    // Main endpoint - returns PNG image directly
    CROW_ROUTE(app, "/render")([](const crow::request& req) {
        // Parse parameters with defaults
        int samples = 25;
        double sphere1_x = 27, sphere1_y = 16.5, sphere1_z = 47;    // Mirror sphere default
        double sphere2_x = 73, sphere2_y = 16.5, sphere2_z = 78;    // Glass sphere default

        // Parse samples parameter
        if (req.url_params.get("samples")) {
            samples = std::max(1, std::min(1000, atoi(req.url_params.get("samples"))));
        }

        // Parse sphere1 coordinates (mirror sphere)
        if (req.url_params.get("s1x")) sphere1_x = atof(req.url_params.get("s1x"));
        if (req.url_params.get("s1y")) sphere1_y = atof(req.url_params.get("s1y"));
        if (req.url_params.get("s1z")) sphere1_z = atof(req.url_params.get("s1z"));

        // Parse sphere2 coordinates (glass sphere)
        if (req.url_params.get("s2x")) sphere2_x = atof(req.url_params.get("s2x"));
        if (req.url_params.get("s2y")) sphere2_y = atof(req.url_params.get("s2y"));
        if (req.url_params.get("s2z")) sphere2_z = atof(req.url_params.get("s2z"));

        // Clamp coordinates to reasonable scene bounds
        sphere1_x = std::max(20.0, std::min(80.0, sphere1_x));
        sphere1_y = std::max(16.5, std::min(65.0, sphere1_y));
        sphere1_z = std::max(30.0, std::min(120.0, sphere1_z));
        
        sphere2_x = std::max(20.0, std::min(80.0, sphere2_x));
        sphere2_y = std::max(16.5, std::min(65.0, sphere2_y));
        sphere2_z = std::max(30.0, std::min(120.0, sphere2_z));

        printf("Rendering: samples=%d, sphere1=(%.1f,%.1f,%.1f), sphere2=(%.1f,%.1f,%.1f)\n",
               samples, sphere1_x, sphere1_y, sphere1_z, sphere2_x, sphere2_y, sphere2_z);

        // Setup scene with new coordinates
        setupScene(sphere1_x, sphere1_y, sphere1_z, sphere2_x, sphere2_y, sphere2_z);

        // Render to PNG buffer
        std::vector<unsigned char> png_buffer;
        if (!renderToPNG(samples, png_buffer)) {
            return crow::response(500, "Rendering failed");
        }

        // Return PNG image directly
        crow::response res(200);
        res.body = std::string(png_buffer.begin(), png_buffer.end());
        res.set_header("Content-Type", "image/png");
        res.set_header("Content-Length", std::to_string(png_buffer.size()));
        res.set_header("Cache-Control", "no-cache"); // Force fresh renders
        return res;
    });

    // Help endpoint
    CROW_ROUTE(app, "/")([](const crow::request& req) {
        std::string help = R"(Path Tracer API

Usage: GET /render with optional parameters:

Parameters:
- samples: Number of samples (1-1000, default: 25)
- s1x, s1y, s1z: Mirror sphere position (default: 27, 16.5, 47)
- s2x, s2y, s2z: Glass sphere position (default: 73, 16.5, 78)

Examples:
/render
/render?samples=100
/render?samples=50&s1x=40&s1y=20&s1z=50
/render?samples=25&s1x=30&s1y=16.5&s1z=60&s2x=70&s2y=16.5&s2z=90

Coordinate bounds:
- X: 20-80 (scene width)
- Y: 16.5-65 (sphere radius to ceiling)
- Z: 30-120 (scene depth)

Returns: PNG image directly
)";
        return crow::response(200, help);
    });

    // Health check
    CROW_ROUTE(app, "/health")([]{
        return crow::response(200, "OK");
    });

    std::cout << "Path Tracer API Server starting on port 18080\n";
    std::cout << "Usage:\n";
    std::cout << "  GET /render?samples=N&s1x=X&s1y=Y&s1z=Z&s2x=X&s2y=Y&s2z=Z\n";
    std::cout << "  GET / (for help)\n";
    std::cout << "\nExample: curl 'http://localhost:18080/render?samples=50&s1x=40&s2x=60' > output.png\n\n";
    
    app.port(18080).multithreaded().run();
    return 0;
}
