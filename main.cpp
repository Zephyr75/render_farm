#include <math.h>   // smallpt, a Path Tracer by Kevin Beason, 2008
#include <stdio.h>  //        Remove "-fopenmp" for g++ version < 4.2
#include <stdlib.h> // Make : g++ -O3 -fopenmp smallpt.cpp -o smallpt
#include <sstream>
#include <fstream>
#include <iomanip>

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

Sphere spheres[] = {
    // Scene: radius, position, emission, color, material
    Sphere(1e5, Vec(1e5 + 1, 40.8, 81.6), Vec(), Vec(.75, .25, .25),
           DIFF), // Left
    Sphere(1e5, Vec(-1e5 + 99, 40.8, 81.6), Vec(), Vec(.25, .25, .75),
           DIFF),                                                     // Rght
    Sphere(1e5, Vec(50, 40.8, 1e5), Vec(), Vec(.75, .75, .75), DIFF), // Back
    Sphere(1e5, Vec(50, 40.8, -1e5 + 170), Vec(), Vec(), DIFF),       // Frnt
    Sphere(1e5, Vec(50, 1e5, 81.6), Vec(), Vec(.75, .75, .75), DIFF), // Botm
    Sphere(1e5, Vec(50, -1e5 + 81.6, 81.6), Vec(), Vec(.75, .75, .75),
           DIFF),                                                      // Top
    Sphere(16.5, Vec(27, 16.5, 47), Vec(), Vec(1, 1, 1) * .999, SPEC), // Mirr
    Sphere(16.5, Vec(73, 16.5, 78), Vec(), Vec(1, 1, 1) * .999, REFR), // Glas
    Sphere(600, Vec(50, 681.6 - .27, 81.6), Vec(12, 12, 12), Vec(),
           DIFF) // Lite
};

inline double clamp(double x) { return x < 0 ? 0 : x > 1 ? 1 : x; }

inline int toInt(double x) { return int(pow(clamp(x), 1 / 2.2) * 255 + .5); }

inline bool intersect(const Ray &r, double &t, int &id) {
  double n = sizeof(spheres) / sizeof(Sphere), d, inf = t = 1e20;
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

std::string render(int samples) {
    int w = 1024, h = 768, samps = samples;
    Ray cam(Vec(50, 52, 295.6), Vec(0, -0.042612, -1).norm());
    Vec cx = Vec(w * .5135 / h), cy = (cx % cam.d).norm() * .5135, r, *c = new Vec[w * h];

    printf("Rendering with %d samples...\n", samps);

    #pragma omp parallel for schedule(dynamic, 1) private(r)
    for (int y = 0; y < h; y++) {
        fprintf(stderr, "\rRendering (%d spp) %5.2f%%", samps * 4, 100. * y / (h - 1));
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

    // Prepare RGB data
    std::vector<unsigned char> image(w * h * 3);
    for (int i = 0; i < w * h; i++) {
        image[i * 3 + 0] = toInt(c[i].x);
        image[i * 3 + 1] = toInt(c[i].y);
        image[i * 3 + 2] = toInt(c[i].z);
    }

    // Generate unique filename with timestamp
    auto now = std::time(nullptr);
    std::ostringstream filename_stream;
    filename_stream << "render_" << samples << "spp_" << now << ".png";
    std::string filename = filename_stream.str();

    // Save PNG file locally
    if (stbi_write_png(filename.c_str(), w, h, 3, image.data(), w * 3)) {
        printf("Image saved as: %s\n", filename.c_str());
    } else {
        printf("Failed to save image: %s\n", filename.c_str());
        delete[] c;
        return "";
    }

    delete[] c;
    return filename;
}

int main() {
    crow::SimpleApp app;

    // Serve static files (images)
    CROW_ROUTE(app, "/images/<string>")([](const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return crow::response(404, "File not found");
        }
        
        std::ostringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        crow::response res(200, content);
        res.set_header("Content-Type", "image/png");
        res.set_header("Cache-Control", "public, max-age=3600");
        return res;
    });

    // Main rendering endpoint
    CROW_ROUTE(app, "/")([](const crow::request& req) {
        int samples = 10;
        if (req.url_params.get("samples")) {
            samples = std::max(1, std::min(1000, atoi(req.url_params.get("samples"))));
        }

        std::string filename = render(samples);
        
        if (filename.empty()) {
            return crow::response(500, "Failed to render image");
        }

        // Create HTML response with proper image display
        std::ostringstream html;
        html << "<!DOCTYPE html>\n"
             << "<html lang=\"en\">\n"
             << "<head>\n"
             << "    <meta charset=\"UTF-8\">\n"
             << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
             << "    <title>Path Tracer Render</title>\n"
             << "    <style>\n"
             << "        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }\n"
             << "        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }\n"
             << "        h1 { color: #333; text-align: center; margin-bottom: 30px; }\n"
             << "        .image-container { text-align: center; margin: 30px 0; }\n"
             << "        img { max-width: 100%; height: auto; border: 2px solid #ddd; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.2); }\n"
             << "        .controls { text-align: center; margin: 30px 0; }\n"
             << "        .controls a { display: inline-block; padding: 10px 20px; margin: 0 10px; background: #007bff; color: white; text-decoration: none; border-radius: 5px; transition: background 0.3s; }\n"
             << "        .controls a:hover { background: #0056b3; }\n"
             << "        .info { background: #e9ecef; padding: 15px; border-radius: 5px; margin: 20px 0; }\n"
             << "        .download { background: #28a745; }\n"
             << "        .download:hover { background: #218838; }\n"
             << "    </style>\n"
             << "</head>\n"
             << "<body>\n"
             << "    <div class=\"container\">\n"
             << "        <h1>Path Tracer Render</h1>\n"
             << "        <div class=\"info\">\n"
             << "            <strong>Samples:</strong> " << samples << " | "
             << "            <strong>Resolution:</strong> 1024x768 | "
             << "            <strong>File:</strong> " << filename << "\n"
             << "        </div>\n"
             << "        <div class=\"image-container\">\n"
             << "            <img src=\"/images/" << filename << "\" alt=\"Rendered scene with " << samples << " samples\" loading=\"lazy\" />\n"
             << "        </div>\n"
             << "        <div class=\"controls\">\n"
             << "            <a href=\"?samples=5\">5 samples (fast)</a>\n"
             << "            <a href=\"?samples=25\">25 samples (medium)</a>\n"
             << "            <a href=\"?samples=100\">100 samples (high)</a>\n"
             << "            <a href=\"?samples=500\">500 samples (very high)</a>\n"
             << "        </div>\n"
             << "        <div class=\"controls\">\n"
             << "            <a href=\"/images/" << filename << "\" download=\"" << filename << "\" class=\"download\">Download Image</a>\n"
             << "        </div>\n"
             << "    </div>\n"
             << "</body>\n"
             << "</html>";

        crow::response res(200, html.str());
        res.set_header("Content-Type", "text/html; charset=utf-8");
        return res;
    });

    // Health check endpoint
    CROW_ROUTE(app, "/health")([]{
        return crow::response(200, "OK");
    });

    std::cout << "Starting path tracer server on port 18080...\n";
    std::cout << "Visit http://localhost:18080 to render images\n";
    std::cout << "Use ?samples=N parameter to control quality (e.g., ?samples=50)\n";
    
    app.port(18080).multithreaded().run();
    return 0;
}
