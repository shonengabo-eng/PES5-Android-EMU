// Sonda de EmuELEC (etapa 1a): SDL2 + GLES sobre la Mali de la caja, sin el juego.
// Uso: ./pes5_gles_probe [segundos]   (START del mando sale antes)
// Las funciones GL se piden con SDL_GL_GetProcAddress: asi no hace falta enlazar libGLESv2.
#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

typedef unsigned int GLenum, GLuint, GLbitfield;
typedef int GLint, GLsizei;
typedef unsigned char GLboolean;
typedef char GLchar;
typedef float GLfloat;
typedef long GLsizeiptr;

#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03
#define GL_NUM_EXTENSIONS 0x821D
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_MAX_DRAW_BUFFERS 0x8824
#define GL_COLOR_BUFFER_BIT 0x4000
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_FLOAT 0x1406
#define GL_TRIANGLES 4

#define GL_FUNCS(X) \
    X(const unsigned char *, GetString, (GLenum)) \
    X(const unsigned char *, GetStringi, (GLenum, GLuint)) \
    X(void, GetIntegerv, (GLenum, GLint *)) \
    X(GLenum, GetError, (void)) \
    X(void, ClearColor, (GLfloat, GLfloat, GLfloat, GLfloat)) \
    X(void, Clear, (GLbitfield)) \
    X(void, Viewport, (GLint, GLint, GLsizei, GLsizei)) \
    X(GLuint, CreateShader, (GLenum)) \
    X(void, ShaderSource, (GLuint, GLsizei, const GLchar *const *, const GLint *)) \
    X(void, CompileShader, (GLuint)) \
    X(void, GetShaderiv, (GLuint, GLenum, GLint *)) \
    X(void, GetShaderInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) \
    X(GLuint, CreateProgram, (void)) \
    X(void, AttachShader, (GLuint, GLuint)) \
    X(void, LinkProgram, (GLuint)) \
    X(void, GetProgramiv, (GLuint, GLenum, GLint *)) \
    X(void, GetProgramInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) \
    X(void, UseProgram, (GLuint)) \
    X(void, GenVertexArrays, (GLsizei, GLuint *)) \
    X(void, BindVertexArray, (GLuint)) \
    X(void, GenBuffers, (GLsizei, GLuint *)) \
    X(void, BindBuffer, (GLenum, GLuint)) \
    X(void, BufferData, (GLenum, GLsizeiptr, const void *, GLenum)) \
    X(void, VertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const void *)) \
    X(void, EnableVertexAttribArray, (GLuint)) \
    X(void, DrawArrays, (GLenum, GLint, GLsizei))

#define DECL(ret, name, args) static ret(*gl##name) args = nullptr;
GL_FUNCS(DECL)
#undef DECL

static bool loadGL()
{
    bool ok = true;
#define LOADF(ret, name, args) \
    gl##name = (ret(*) args)SDL_GL_GetProcAddress("gl" #name); \
    if (!gl##name) { std::printf("falta gl%s\n", #name); ok = false; }
    GL_FUNCS(LOADF)
#undef LOADF
    return ok;
}

static bool hasExt(const char *name)
{
    GLint n = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    for (GLint i = 0; i < n; i++) {
        const char *e = (const char *)glGetStringi(GL_EXTENSIONS, (GLuint)i);
        if (e && std::strcmp(e, name) == 0) return true;
    }
    return false;
}

static GLuint compile(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        glGetShaderInfoLog(s, sizeof(log) - 1, nullptr, log);
        std::printf("error de shader: %s\n", log);
        return 0;
    }
    return s;
}

int main(int argc, char **argv)
{
    const double seconds = argc > 1 ? std::atof(argv[1]) : 15.0;
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
        std::printf("SDL_Init fallo: %s\n", SDL_GetError());
        return 1;
    }
    SDL_version v;
    SDL_GetVersion(&v);
    std::printf("SDL %d.%d.%d, driver de video: %s\n", v.major, v.minor, v.patch, SDL_GetCurrentVideoDriver());

    SDL_DisplayMode dm;
    int w = 1280, h = 720;
    if (SDL_GetDesktopDisplayMode(0, &dm) == 0) { w = dm.w; h = dm.h; }
    std::printf("pantalla: %dx%d\n", w, h);

    // GLES 3.2, si no 3.1, si no 3.0
    SDL_Window *win = nullptr;
    SDL_GLContext ctx = nullptr;
    const int minors[3] = {2, 1, 0};
    for (int i = 0; i < 3 && !ctx; i++) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minors[i]);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        win = SDL_CreateWindow("pes5", 0, 0, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
        if (!win) { std::printf("ventana (GLES 3.%d) fallo: %s\n", minors[i], SDL_GetError()); continue; }
        ctx = SDL_GL_CreateContext(win);
        if (!ctx) {
            std::printf("contexto GLES 3.%d fallo: %s\n", minors[i], SDL_GetError());
            SDL_DestroyWindow(win);
            win = nullptr;
        } else {
            std::printf("contexto GLES 3.%d creado\n", minors[i]);
        }
    }
    if (!ctx) { SDL_Quit(); return 2; }
    SDL_GL_MakeCurrent(win, ctx);
    SDL_GL_SetSwapInterval(1);

    if (!loadGL()) { std::printf("faltan funciones GL\n"); return 3; }

    std::printf("GL_VENDOR:   %s\n", (const char *)glGetString(GL_VENDOR));
    std::printf("GL_RENDERER: %s\n", (const char *)glGetString(GL_RENDERER));
    std::printf("GL_VERSION:  %s\n", (const char *)glGetString(GL_VERSION));
    std::printf("GLSL:        %s\n", (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
    GLint maxTex = 0, maxDraw = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTex);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDraw);
    std::printf("max textura: %d, max draw buffers: %d\n", maxTex, maxDraw);

    // extensiones que el backend GLES de PES5 va a querer
    const char *want[] = {"GL_ARM_shader_framebuffer_fetch", "GL_EXT_shader_framebuffer_fetch",
                          "GL_EXT_geometry_shader", "GL_EXT_draw_buffers_indexed", "GL_EXT_color_buffer_float",
                          "GL_EXT_buffer_storage", "GL_OES_get_program_binary", "GL_EXT_clip_cull_distance",
                          "GL_EXT_multisampled_render_to_texture", "GL_EXT_texture_filter_anisotropic"};
    for (const char *e : want) std::printf("  %-42s %s\n", e, hasExt(e) ? "SI" : "no");

    // triangulo con shaders GLSL ES 3.00
    const char *vs = "#version 300 es\n"
                     "layout(location=0) in vec2 pos; layout(location=1) in vec3 col; out vec3 v;\n"
                     "void main(){ v = col; gl_Position = vec4(pos, 0.0, 1.0); }\n";
    const char *fs = "#version 300 es\n"
                     "precision mediump float; in vec3 v; out vec4 o;\n"
                     "void main(){ o = vec4(v, 1.0); }\n";
    GLuint prog = 0;
    GLuint a = compile(GL_VERTEX_SHADER, vs), b = compile(GL_FRAGMENT_SHADER, fs);
    if (a && b) {
        prog = glCreateProgram();
        glAttachShader(prog, a);
        glAttachShader(prog, b);
        glLinkProgram(prog);
        GLint ok = 0;
        glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024] = {0};
            glGetProgramInfoLog(prog, sizeof(log) - 1, nullptr, log);
            std::printf("error al enlazar: %s\n", log);
            prog = 0;
        } else {
            std::printf("shaders GLSL ES 3.00: compilan y enlazan\n");
        }
    }
    const GLfloat tri[] = {-0.6f, -0.6f, 1, 0, 0,   0.6f, -0.6f, 0, 1, 0,   0.0f, 0.6f, 0, 0, 1};
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tri), tri, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, 0, 5 * sizeof(GLfloat), (const void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, 0, 5 * sizeof(GLfloat), (const void *)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // mandos
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            SDL_GameController *gc = SDL_GameControllerOpen(i);
            std::printf("mando %d (GameController): %s\n", i, gc ? SDL_GameControllerName(gc) : "no abre");
        } else {
            SDL_Joystick *js = SDL_JoystickOpen(i);
            std::printf("mando %d (joystick sin mapa): %s\n", i, js ? SDL_JoystickName(js) : "no abre");
        }
    }

    const Uint32 t0 = SDL_GetTicks();
    long frames = 0;
    bool run = true;
    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) run = false;
            else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                std::printf("boton: %s\n", SDL_GameControllerGetStringForButton((SDL_GameControllerButton)e.cbutton.button));
                if (e.cbutton.button == SDL_CONTROLLER_BUTTON_START) run = false;
            } else if (e.type == SDL_JOYBUTTONDOWN) {
                std::printf("joy boton %d\n", e.jbutton.button);
            }
        }
        const double t = (SDL_GetTicks() - t0) / 1000.0;
        if (t >= seconds) break;
        glViewport(0, 0, w, h);
        glClearColor(0.15f + 0.1f * (float)std::sin(t), 0.15f, 0.25f + 0.1f * (float)std::cos(t), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        if (prog) {
            glUseProgram(prog);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        SDL_GL_SwapWindow(win);
        frames++;
    }
    const double el = (SDL_GetTicks() - t0) / 1000.0;
    std::printf("fotogramas: %ld en %.1f s = %.1f fps, glGetError=0x%x\n", frames, el, el > 0 ? frames / el : 0.0, glGetError());

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    std::printf("fin\n");
    return 0;
}
