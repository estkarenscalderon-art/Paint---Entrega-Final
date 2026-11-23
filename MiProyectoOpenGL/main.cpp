#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>

// Librería Dear ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cstdio>
#include <utility>

constexpr int   W = 1000;
constexpr int   H = 800;
constexpr int   GRID_SIZE = 5; 
constexpr float PI = 3.14159265358979f;

using Pixel = uint32_t;

inline Pixel rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
    return (Pixel)r | ((Pixel)g << 8) | ((Pixel)b << 16) | ((Pixel)a << 24);
}
const Pixel BLANCO = rgba(255, 255, 255);

struct Vec2 { float x, y; };

enum class Herramienta { ManoAlzada, Linea, Circulo, Triangulo, Cuadrado, Poligono, Estrella, Relleno, Rotar, Borrador };
enum class TipoFigura { Polilinea, Poligono, Circulo, Relleno };

struct Figura
{
    TipoFigura tipo = TipoFigura::Polilinea;
    std::vector<Vec2> pts;
    Pixel color = 0;
    int grosor = 1;
    bool borrador = false;
    bool pintarCeldas = true;
};

using Buffer = std::vector<Pixel>;

inline uint8_t obtR(Pixel p) { return (uint8_t)(p & 0xFFu); }
inline uint8_t obtG(Pixel p) { return (uint8_t)((p >> 8) & 0xFFu); }
inline uint8_t obtB(Pixel p) { return (uint8_t)((p >> 16) & 0xFFu); }
inline int redondear(float v) { return (int)std::lround(v); }

// Pinta un cuadrado completo de la grilla
void pintarCelda(Buffer& b, int gx, int gy, Pixel c)
{
    int x0 = gx * GRID_SIZE;
    int y0 = gy * GRID_SIZE;
    int x1 = std::min(W, x0 + GRID_SIZE);
    int y1 = std::min(H, y0 + GRID_SIZE);

    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
            if (x >= 0 && y >= 0)
                b[y * W + x] = c;
}

// Mezcla semitransparente para dibujar la grilla sin alterar el dibujo guardado
inline void mezclarPixel(Buffer& b, int x, int y, uint8_t r, uint8_t g, uint8_t bCol, float alfa)
{
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    Pixel& dst = b[y * W + x];
    uint8_t nr = (uint8_t)std::lround(r * alfa + obtR(dst) * (1.f - alfa));
    uint8_t ng = (uint8_t)std::lround(g * alfa + obtG(dst) * (1.f - alfa));
    uint8_t nb = (uint8_t)std::lround(bCol * alfa + obtB(dst) * (1.f - alfa));
    dst = rgba(nr, ng, nb);
}

void dibujarGrilla(Buffer& b, uint8_t gr, uint8_t gg, uint8_t gb)
{
    for (int x = GRID_SIZE; x < W; x += GRID_SIZE)
        for (int y = 0; y < H; ++y)
            mezclarPixel(b, x, y, gr, gg, gb, 0.35f);

    for (int y = GRID_SIZE; y < H; y += GRID_SIZE)
        for (int x = 0; x < W; ++x)
            mezclarPixel(b, x, y, gr, gg, gb, 0.35f);
}

void ponerPixel(Buffer& b, int x, int y, Pixel c, int grosor, bool pintarCeldas)
{
    if (pintarCeldas)
    {
        if (x < 0 || x >= W || y < 0 || y >= H) return;
        int gx = x / GRID_SIZE;
        int gy = y / GRID_SIZE;
        int rad = std::max(0, (grosor - 1) / 2);
        for (int dy = -rad; dy <= rad; ++dy)
            for (int dx = -rad; dx <= rad; ++dx)
                pintarCelda(b, gx + dx, gy + dy, c);
        return;
    }

    int r = grosor / 2;
    if (r <= 0)
    {
        if (x >= 0 && x < W && y >= 0 && y < H) b[y * W + x] = c;
        return;
    }
    for (int dy = -r; dy <= r; ++dy)
        for (int dx = -r; dx <= r; ++dx)
        {
            if (dx * dx + dy * dy > r * r + r) continue;
            int px = x + dx, py = y + dy;
            if (px >= 0 && px < W && py >= 0 && py < H) b[py * W + px] = c;
        }
}

void lineaBresenham(Buffer& b, int x0, int y0, int x1, int y1, Pixel c, int grosor, bool pintarCeldas)
{
    if (pintarCeldas)
    {
        x0 = (x0 / GRID_SIZE) * GRID_SIZE + GRID_SIZE / 2;
        y0 = (y0 / GRID_SIZE) * GRID_SIZE + GRID_SIZE / 2;
        x1 = (x1 / GRID_SIZE) * GRID_SIZE + GRID_SIZE / 2;
        y1 = (y1 / GRID_SIZE) * GRID_SIZE + GRID_SIZE / 2;
    }

    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true)
    {
        ponerPixel(b, x0, y0, c, grosor, pintarCeldas);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void circuloBresenham(Buffer& b, int cx, int cy, int r, Pixel c, int grosor, bool pintarCeldas)
{
    if (r <= 0) { ponerPixel(b, cx, cy, c, grosor, pintarCeldas); return; }
    int x = 0, y = r, d = 3 - 2 * r;
    while (y >= x)
    {
        ponerPixel(b, cx + x, cy + y, c, grosor, pintarCeldas);
        ponerPixel(b, cx - x, cy + y, c, grosor, pintarCeldas);
        ponerPixel(b, cx + x, cy - y, c, grosor, pintarCeldas);
        ponerPixel(b, cx - x, cy - y, c, grosor, pintarCeldas);
        ponerPixel(b, cx + y, cy + x, c, grosor, pintarCeldas);
        ponerPixel(b, cx - y, cy + x, c, grosor, pintarCeldas);
        ponerPixel(b, cx + y, cy - x, c, grosor, pintarCeldas);
        ponerPixel(b, cx - y, cy - x, c, grosor, pintarCeldas);
        ++x;
        if (d > 0) { --y; d += 4 * (x - y) + 10; }
        else { d += 4 * x + 6; }
    }
}

void rellenoCeldas(Buffer& b, int sx, int sy, Pixel relleno)
{
    if (sx < 0 || sx >= W || sy < 0 || sy >= H) return;
    int cols = (W + GRID_SIZE - 1) / GRID_SIZE;
    int rows = (H + GRID_SIZE - 1) / GRID_SIZE;
    int sgx = sx / GRID_SIZE, sgy = sy / GRID_SIZE;

    auto colorCelda = [&](int gx, int gy) -> Pixel {
        int px = std::min(W - 1, gx * GRID_SIZE);
        int py = std::min(H - 1, gy * GRID_SIZE);
        return b[py * W + px];
        };

    Pixel objetivo = colorCelda(sgx, sgy);
    if (objetivo == relleno) return;

    std::vector<std::vector<uint8_t>> visitado(rows, std::vector<uint8_t>(cols, 0));
    std::vector<std::pair<int, int>> pila = { { sgx, sgy } };
    visitado[sgy][sgx] = 1;

    while (!pila.empty())
    {
        auto [gx, gy] = pila.back();
        pila.pop_back();
        pintarCelda(b, gx, gy, relleno);

        const int dx[4] = { 1, -1, 0, 0 };
        const int dy[4] = { 0, 0, 1, -1 };
        for (int k = 0; k < 4; ++k)
        {
            int nx = gx + dx[k], ny = gy + dy[k];
            if (nx >= 0 && nx < cols && ny >= 0 && ny < rows && !visitado[ny][nx] && colorCelda(nx, ny) == objetivo)
            {
                visitado[ny][nx] = 1;
                pila.push_back({ nx, ny });
            }
        }
    }
}

void rellenoPixeles(Buffer& b, int sx, int sy, Pixel relleno)
{
    if (sx < 0 || sx >= W || sy < 0 || sy >= H) return;
    const Pixel objetivo = b[sy * W + sx];
    if (objetivo == relleno) return;

    std::vector<std::pair<int, int>> pila = { { sx, sy } };
    while (!pila.empty())
    {
        auto [x, y] = pila.back();
        pila.pop_back();
        if (b[y * W + x] != objetivo) continue;

        int xl = x, xr = x;
        while (xl > 0 && b[y * W + xl - 1] == objetivo) --xl;
        while (xr < W - 1 && b[y * W + xr + 1] == objetivo) ++xr;
        for (int i = xl; i <= xr; ++i) b[y * W + i] = relleno;

        int vecinos[2] = { y - 1, y + 1 };
        for (int ny : vecinos)
        {
            if (ny < 0 || ny >= H) continue;
            bool tramo = false;
            for (int i = xl; i <= xr; ++i)
            {
                bool igual = b[ny * W + i] == objetivo;
                if (igual && !tramo) { pila.push_back({ i, ny }); tramo = true; }
                else if (!igual)     tramo = false;
            }
        }
    }
}

void dibujarFigura(Buffer& b, const Figura& f)
{
    switch (f.tipo)
    {
    case TipoFigura::Polilinea:
        if (f.pts.size() == 1)
            ponerPixel(b, redondear(f.pts[0].x), redondear(f.pts[0].y), f.color, f.grosor, f.pintarCeldas);
        for (size_t i = 0; i + 1 < f.pts.size(); ++i)
            lineaBresenham(b, redondear(f.pts[i].x), redondear(f.pts[i].y),
                redondear(f.pts[i + 1].x), redondear(f.pts[i + 1].y),
                f.color, f.grosor, f.pintarCeldas);
        break;

    case TipoFigura::Poligono:
        for (size_t i = 0; i < f.pts.size(); ++i)
        {
            const Vec2& a = f.pts[i];
            const Vec2& c = f.pts[(i + 1) % f.pts.size()];
            lineaBresenham(b, redondear(a.x), redondear(a.y), redondear(c.x), redondear(c.y),
                f.color, f.grosor, f.pintarCeldas);
        }
        break;

    case TipoFigura::Circulo:
    {
        float r = std::hypot(f.pts[1].x - f.pts[0].x, f.pts[1].y - f.pts[0].y);
        circuloBresenham(b, redondear(f.pts[0].x), redondear(f.pts[0].y), redondear(r),
            f.color, f.grosor, f.pintarCeldas);
        break;
    }

    case TipoFigura::Relleno:
        if (f.pintarCeldas)
            rellenoCeldas(b, redondear(f.pts[0].x), redondear(f.pts[0].y), f.color);
        else
            rellenoPixeles(b, redondear(f.pts[0].x), redondear(f.pts[0].y), f.color);
        break;
    }
}

Figura crearFigura(Herramienta h, Vec2 a, Vec2 b, Pixel color, int grosor, int n, bool pintarCeldas)
{
    Figura f;
    f.color = color;
    f.grosor = grosor;
    f.pintarCeldas = pintarCeldas;

    switch (h)
    {
    case Herramienta::Linea:
        f.tipo = TipoFigura::Polilinea;
        f.pts = { a, b };
        break;
    case Herramienta::Circulo:
        f.tipo = TipoFigura::Circulo;
        f.pts = { a, b };
        break;
    case Herramienta::Cuadrado:
    {
        f.tipo = TipoFigura::Poligono;
        float dx = b.x - a.x, dy = b.y - a.y;
        float lado = std::max(std::fabs(dx), std::fabs(dy));
        float sx = dx < 0 ? -1.f : 1.f;
        float sy = dy < 0 ? -1.f : 1.f;
        f.pts = { a,{ a.x + sx * lado, a.y },{ a.x + sx * lado, a.y + sy * lado },{ a.x, a.y + sy * lado } };
        break;
    }
    case Herramienta::Poligono:
    {
        f.tipo = TipoFigura::Poligono;
        float r = std::hypot(b.x - a.x, b.y - a.y);
        float a0 = std::atan2(b.y - a.y, b.x - a.x);
        for (int i = 0; i < n; ++i)
        {
            float ang = a0 + 2.f * PI * i / n;
            f.pts.push_back({ a.x + r * std::cos(ang), a.y + r * std::sin(ang) });
        }
        break;
    }
    case Herramienta::Estrella:
    {
        f.tipo = TipoFigura::Poligono;
        float r = std::hypot(b.x - a.x, b.y - a.y);
        float a0 = std::atan2(b.y - a.y, b.x - a.x);
        for (int i = 0; i < 2 * n; ++i)
        {
            float rr = (i % 2 == 0) ? r : r * 0.4f;
            float ang = a0 + PI * i / n;
            f.pts.push_back({ a.x + rr * std::cos(ang), a.y + rr * std::sin(ang) });
        }
        break;
    }
    default: break;
    }
    return f;
}

void rotarFigura(Figura& f, float ang)
{
    if ((f.tipo != TipoFigura::Polilinea && f.tipo != TipoFigura::Poligono) || f.pts.empty()) return;
    Vec2 c{ 0, 0 };
    for (const auto& p : f.pts) { c.x += p.x; c.y += p.y; }
    c.x /= f.pts.size();
    c.y /= f.pts.size();

    float co = std::cos(ang), si = std::sin(ang);
    for (auto& p : f.pts)
    {
        float x = p.x - c.x, y = p.y - c.y;
        p.x = c.x + x * co - y * si;
        p.y = c.y + x * si + y * co;
    }
}

float distSegmento(Vec2 p, Vec2 a, Vec2 b)
{
    float vx = b.x - a.x, vy = b.y - a.y;
    float wx = p.x - a.x, wy = p.y - a.y;
    float len2 = vx * vx + vy * vy;
    float t = std::clamp(len2 > 0 ? (wx * vx + wy * vy) / len2 : 0.f, 0.f, 1.f);
    return std::hypot(p.x - (a.x + t * vx), p.y - (a.y + t * vy));
}

int buscarFigura(const std::vector<Figura>& figuras, Vec2 p)
{
    const float TOL = 8.f;
    for (int i = (int)figuras.size() - 1; i >= 0; --i)
    {
        const Figura& f = figuras[i];
        if (f.borrador || (f.tipo != TipoFigura::Polilinea && f.tipo != TipoFigura::Poligono)) continue;
        size_t n = f.pts.size();
        if (n == 1 && std::hypot(p.x - f.pts[0].x, p.y - f.pts[0].y) <= TOL) return i;
        size_t segs = (f.tipo == TipoFigura::Poligono) ? n : (n > 0 ? n - 1 : 0);
        for (size_t k = 0; k < segs; ++k)
            if (distSegmento(p, f.pts[k], f.pts[(k + 1) % n]) <= TOL) return i;
    }
    return -1;
}

int main()
{
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* win = glfwCreateWindow(1120, 680, "Mini Paint", nullptr, nullptr);
    if (!win) { glfwTerminate(); return -1; }
    glfwSetWindowSizeLimits(win, 640, 420, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Error al inicializar GLAD\n";
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    std::vector<Figura> figuras;
    Buffer cache(W * H, BLANCO), pantalla(W * H, BLANCO);
    bool sucio = true;

    int   herrInt = (int)Herramienta::ManoAlzada;
    float colorTrazo[3] = { 0.f, 0.f, 0.f };
    float colorGrilla[3] = { 0.55f, 0.57f, 0.61f };
    int   grosor = 1;
    int   grosorBor = 1;
    int   lados = 5;
    int   puntas = 5;
    int   pasoRot = 15;

    bool  mostrarGrilla = true;
    bool  pintarCeldas = true;

    bool  dibujando = false;
    Vec2  inicio{ 0, 0 };
    Figura actual;
    std::vector<Vec2> ptsTriangulo;

    auto centroCelda = [](Vec2 p) -> Vec2 {
        int gx = (int)(p.x / GRID_SIZE);
        int gy = (int)(p.y / GRID_SIZE);
        return {
            std::clamp(gx * GRID_SIZE + GRID_SIZE * 0.5f, 0.f, (float)(W - 1)),
            std::clamp(gy * GRID_SIZE + GRID_SIZE * 0.5f, 0.f, (float)(H - 1))
        };
        };

    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImVec2 tamVentana = ImGui::GetIO().DisplaySize;
        const float margen = 10.f;
        const float anchoPanel = 260.f;
        float altoPanel = std::max(200.f, tamVentana.y - 2.f * margen);

        Herramienta herr = (Herramienta)herrInt;
        auto obtColor = [&]() {
            return rgba((uint8_t)(colorTrazo[0] * 255 + .5f),
                (uint8_t)(colorTrazo[1] * 255 + .5f),
                (uint8_t)(colorTrazo[2] * 255 + .5f));
            };

        // Panel izquierdo
        ImGui::SetNextWindowPos(ImVec2(margen, margen), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(anchoPanel, altoPanel), ImGuiCond_Always);
        ImGui::Begin("Herramientas", nullptr,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        auto elegir = [&](const char* etiqueta, Herramienta h) {
            if (ImGui::RadioButton(etiqueta, herrInt == (int)h))
            {
                herrInt = (int)h;
                dibujando = false;
                ptsTriangulo.clear();
            }
            };
        elegir("Mano alzada", Herramienta::ManoAlzada);
        elegir("Linea (Bresenham)", Herramienta::Linea);
        elegir("Circulo (Bresenham)", Herramienta::Circulo);
        elegir("Triangulo (3 clics)", Herramienta::Triangulo);
        elegir("Cuadrado", Herramienta::Cuadrado);
        elegir("Poligono regular", Herramienta::Poligono);
        elegir("Estrella", Herramienta::Estrella);
        elegir("Relleno", Herramienta::Relleno);
        elegir("Rotar figura", Herramienta::Rotar);
        elegir("Borrador", Herramienta::Borrador);

        ImGui::Separator();
        ImGui::ColorEdit3("Color trazo", colorTrazo, ImGuiColorEditFlags_NoInputs);
        ImGui::SliderInt("Grosor / celdas", &grosor, 1, 15);
        ImGui::SliderInt("Borrador", &grosorBor, 1, 15);
        ImGui::SliderInt("Lados poligono", &lados, 3, 12);
        ImGui::SliderInt("Puntas estrella", &puntas, 3, 12);
        ImGui::SliderInt("Rotacion (grados)", &pasoRot, 1, 90);

        ImGui::Separator();
        ImGui::Checkbox("Mostrar grilla", &mostrarGrilla);
        ImGui::Checkbox("Pintar cuadrados", &pintarCeldas);
        ImGui::ColorEdit3("Color grilla", colorGrilla, ImGuiColorEditFlags_NoInputs);

        ImGui::Separator();
        if (ImGui::Button("Deshacer", ImVec2(110, 0)) && !figuras.empty())
        {
            figuras.pop_back();
            sucio = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Limpiar", ImVec2(110, 0)))
        {
            figuras.clear();
            ptsTriangulo.clear();
            dibujando = false;
            sucio = true;
        }
        ImGui::End();

        // Ventana del lienzo
        float lienzoX = margen + anchoPanel + margen;
        float lienzoW = std::max(100.f, tamVentana.x - lienzoX - margen);
        float lienzoH = std::max(100.f, tamVentana.y - 2.f * margen);

        ImGui::SetNextWindowPos(ImVec2(lienzoX, margen), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(lienzoW, lienzoH), ImGuiCond_Always);
        ImGui::Begin("Lienzo", nullptr,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImVec2 disp = ImGui::GetContentRegionAvail();
        float escala = std::max(0.1f, std::min(disp.x / (float)W, disp.y / (float)H));
        float imgW = W * escala;
        float imgH = H * escala;

        ImVec2 pos = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(pos.x + (disp.x - imgW) * 0.5f, pos.y + (disp.y - imgH) * 0.5f));

        ImVec2 origen = ImGui::GetCursorScreenPos();
        ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(imgW, imgH));
        bool encima = ImGui::IsItemHovered();

        ImVec2 mouse = ImGui::GetIO().MousePos;
        Vec2 mpReal{ std::clamp((mouse.x - origen.x) / escala, 0.f, (float)(W - 1)),
                     std::clamp((mouse.y - origen.y) / escala, 0.f, (float)(H - 1)) };
        Vec2 mp = pintarCeldas ? centroCelda(mpReal) : mpReal;

        bool clicIzq = encima && ImGui::IsMouseClicked(0);
        bool clicDer = encima && ImGui::IsMouseClicked(1);
        bool izqAbajo = ImGui::IsMouseDown(0);
        bool izqSoltar = ImGui::IsMouseReleased(0);

        if (clicDer && herr != Herramienta::Rotar && (dibujando || !ptsTriangulo.empty()))
        {
            dibujando = false;
            ptsTriangulo.clear();
            clicDer = false;
        }

        if (clicIzq)
        {
            switch (herr)
            {
            case Herramienta::Relleno:
            {
                Figura f;
                f.tipo = TipoFigura::Relleno;
                f.pts = { mpReal };
                f.color = obtColor();
                f.pintarCeldas = pintarCeldas;
                figuras.push_back(f);
                sucio = true;
                break;
            }
            case Herramienta::Rotar:
            {
                int idx = buscarFigura(figuras, mpReal);
                if (idx >= 0) { rotarFigura(figuras[idx], pasoRot * PI / 180.f); sucio = true; }
                break;
            }
            case Herramienta::Triangulo:
                ptsTriangulo.push_back(mp);
                if (ptsTriangulo.size() == 3)
                {
                    Figura f;
                    f.tipo = TipoFigura::Poligono;
                    f.pts = ptsTriangulo;
                    f.color = obtColor();
                    f.grosor = grosor;
                    f.pintarCeldas = pintarCeldas;
                    figuras.push_back(f);
                    ptsTriangulo.clear();
                    sucio = true;
                }
                break;
            case Herramienta::ManoAlzada:
            case Herramienta::Borrador:
                dibujando = true;
                actual = Figura();
                actual.tipo = TipoFigura::Polilinea;
                actual.pts = { mp };
                actual.borrador = (herr == Herramienta::Borrador);
                actual.color = actual.borrador ? BLANCO : obtColor();
                actual.grosor = actual.borrador ? grosorBor : grosor;
                actual.pintarCeldas = pintarCeldas;
                break;
            default:
                dibujando = true;
                inicio = mp;
                break;
            }
        }

        if (clicDer && herr == Herramienta::Rotar)
        {
            int idx = buscarFigura(figuras, mpReal);
            if (idx >= 0) { rotarFigura(figuras[idx], -pasoRot * PI / 180.f); sucio = true; }
        }

        if (dibujando && izqAbajo)
        {
            if (herr == Herramienta::ManoAlzada || herr == Herramienta::Borrador)
            {
                const Vec2& ult = actual.pts.back();
                float minPaso = pintarCeldas ? GRID_SIZE * 0.5f : 1.f;
                if (std::hypot(mp.x - ult.x, mp.y - ult.y) >= minPaso)
                    actual.pts.push_back(mp);
            }
            else
            {
                actual = crearFigura(herr, inicio, mp, obtColor(), grosor,
                    herr == Herramienta::Estrella ? puntas : lados, pintarCeldas);
            }
        }

        if (dibujando && izqSoltar)
        {
            if (herr != Herramienta::ManoAlzada && herr != Herramienta::Borrador)
                actual = crearFigura(herr, inicio, mp, obtColor(), grosor,
                    herr == Herramienta::Estrella ? puntas : lados, pintarCeldas);
            if (!actual.pts.empty()) { figuras.push_back(actual); sucio = true; }
            dibujando = false;
        }
        ImGui::End();

        // Rasterizado
        if (sucio)
        {
            cache.assign(W * H, BLANCO);
            for (const Figura& f : figuras) dibujarFigura(cache, f);
            sucio = false;
        }
        pantalla = cache;

        if (dibujando && !actual.pts.empty())
            dibujarFigura(pantalla, actual);
        if (herr == Herramienta::Triangulo && !ptsTriangulo.empty())
        {
            Figura pv;
            pv.color = obtColor();
            pv.grosor = grosor;
            pv.pts = ptsTriangulo;
            pv.pts.push_back(mp);
            pv.tipo = (pv.pts.size() >= 3) ? TipoFigura::Poligono : TipoFigura::Polilinea;
            pv.pintarCeldas = pintarCeldas;
            dibujarFigura(pantalla, pv);
        }

        if (mostrarGrilla)
            dibujarGrilla(pantalla,
                (uint8_t)(colorGrilla[0] * 255 + .5f),
                (uint8_t)(colorGrilla[1] * 255 + .5f),
                (uint8_t)(colorGrilla[2] * 255 + .5f));

        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, pantalla.data());

        // Renderizado
        ImGui::Render();
        int fw, fh;
        glfwGetFramebufferSize(win, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glClearColor(0.1f, 0.1f, 0.12f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }

    glDeleteTextures(1, &tex);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}