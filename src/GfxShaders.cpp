// ─────────────────────────────────────────────────────────────────────────────
//  GfxShaders.cpp  —  Metro Smash Advanced Graphics Pipeline
// ─────────────────────────────────────────────────────────────────────────────
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <random>

#ifdef _WIN32
#  include <windows.h>
#endif
#include <GL/gl.h>

#include "GfxShaders.h"

// ─── Global instance ──────────────────────────────────────────────────────────
GfxPipeline gGfx;

// ─── GL extension function pointers ──────────────────────────────────────────
#ifdef _WIN32
void* gfxGetProcAddress(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    if (!p) {
        HMODULE m = LoadLibraryA("opengl32.dll");
        if (m) p = (void*)GetProcAddress(m, name);
    }
    return p;
}
#else
#include <GL/glx.h>
void* gfxGetProcAddress(const char* name) {
    return (void*)glXGetProcAddress((const GLubyte*)name);
}
#endif

// ─── GL function pointer declarations ────────────────────────────────────────
typedef unsigned int  GLuint;
typedef int           GLint;
typedef unsigned int  GLenum;
typedef float         GLfloat;
typedef unsigned char GLboolean;
typedef int           GLsizei;
typedef ptrdiff_t     GLsizeiptr;
typedef char          GLchar;

// Framebuffers
typedef void  (*PFNGLGENFRAMEBUFFERSPROC)(GLsizei, GLuint*);
typedef void  (*PFNGLBINDFRAMEBUFFERPROC)(GLenum, GLuint);
typedef void  (*PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum,GLenum,GLenum,GLuint,GLint);
typedef void  (*PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum,GLenum,GLenum,GLuint);
typedef GLenum(*PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum);
typedef void  (*PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void  (*PFNGLDRAWBUFFERSPROC)(GLsizei, const GLenum*);

// Renderbuffers
typedef void  (*PFNGLGENRENDERBUFFERSPROC)(GLsizei, GLuint*);
typedef void  (*PFNGLBINDRENDERBUFFERPROC)(GLenum, GLuint);
typedef void  (*PFNGLRENDERBUFFERSTORAGEPROC)(GLenum,GLenum,GLsizei,GLsizei);
typedef void  (*PFNGLDELETERENDERBUFFERSPROC)(GLsizei, const GLuint*);

// Shaders
typedef GLuint(*PFNGLCREATESHADERPROC)(GLenum);
typedef void  (*PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar**, const GLint*);
typedef void  (*PFNGLCOMPILESHADERPROC)(GLuint);
typedef void  (*PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void  (*PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef GLuint(*PFNGLCREATEPROGRAMPROC)();
typedef void  (*PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void  (*PFNGLLINKPROGRAMPROC)(GLuint);
typedef void  (*PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void  (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void  (*PFNGLUSEPROGRAMPROC)(GLuint);
typedef void  (*PFNGLDELETESHADERPROC)(GLuint);
typedef void  (*PFNGLDELETEPROGRAMPROC)(GLuint);
typedef GLint (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void  (*PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void  (*PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void  (*PFNGLUNIFORM2FPROC)(GLint, GLfloat, GLfloat);
typedef void  (*PFNGLUNIFORM3FPROC)(GLint, GLfloat, GLfloat, GLfloat);
typedef void  (*PFNGLUNIFORM3FVPROC)(GLint, GLsizei, const GLfloat*);
typedef void  (*PFNGLUNIFORMMATRIX4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat*);

// VBO/VAO
typedef void  (*PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void  (*PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void  (*PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void  (*PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void  (*PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void  (*PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void  (*PFNGLVERTEXATTRIBPOINTERPROC)(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*);
typedef void  (*PFNGLACTIVETEXTUREPROC)(GLenum);
typedef void  (*PFNGLBINDATTRIBLOCATIONPROC)(GLuint, GLuint, const GLchar*);
typedef GLint (*PFNGLGETATTRIBLOCATIONPROC)(GLuint, const GLchar*);

// Texture
typedef void  (*PFNGLTEXIMAGE2DMULTISAMPLEPROC)(GLenum,GLsizei,GLenum,GLsizei,GLsizei,GLboolean);
typedef void  (*PFNGLGENERATEMIPMAPPROC)(GLenum);
typedef void  (*PFNGLDELETEVERTEXARRAYSPROC)(GLsizei,const GLuint*);
typedef void  (*PFNGLDELETEBUFFERSPROC)(GLsizei,const GLuint*);

// All functions in a struct
static struct GlExt {
    PFNGLGENFRAMEBUFFERSPROC       GenFramebuffers       = nullptr;
    PFNGLBINDFRAMEBUFFERPROC       BindFramebuffer       = nullptr;
    PFNGLFRAMEBUFFERTEXTURE2DPROC  FramebufferTexture2D  = nullptr;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC FramebufferRenderbuffer = nullptr;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus = nullptr;
    PFNGLDELETEFRAMEBUFFERSPROC    DeleteFramebuffers    = nullptr;
    PFNGLDRAWBUFFERSPROC           DrawBuffers           = nullptr;
    PFNGLGENRENDERBUFFERSPROC      GenRenderbuffers      = nullptr;
    PFNGLBINDRENDERBUFFERPROC      BindRenderbuffer      = nullptr;
    PFNGLRENDERBUFFERSTORAGEPROC   RenderbufferStorage   = nullptr;
    PFNGLDELETERENDERBUFFERSPROC   DeleteRenderbuffers   = nullptr;
    PFNGLCREATESHADERPROC          CreateShader          = nullptr;
    PFNGLSHADERSOURCEPROC          ShaderSource          = nullptr;
    PFNGLCOMPILESHADERPROC         CompileShader         = nullptr;
    PFNGLGETSHADERIVPROC           GetShaderiv           = nullptr;
    PFNGLGETSHADERINFOLOGPROC      GetShaderInfoLog      = nullptr;
    PFNGLCREATEPROGRAMPROC         CreateProgram         = nullptr;
    PFNGLATTACHSHADERPROC          AttachShader          = nullptr;
    PFNGLLINKPROGRAMPROC           LinkProgram           = nullptr;
    PFNGLGETPROGRAMIVPROC          GetProgramiv          = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC     GetProgramInfoLog     = nullptr;
    PFNGLUSEPROGRAMPROC            UseProgram            = nullptr;
    PFNGLDELETESHADERPROC          DeleteShader          = nullptr;
    PFNGLDELETEPROGRAMPROC         DeleteProgram         = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC    GetUniformLocation    = nullptr;
    PFNGLUNIFORM1IPROC             Uniform1i             = nullptr;
    PFNGLUNIFORM1FPROC             Uniform1f             = nullptr;
    PFNGLUNIFORM2FPROC             Uniform2f             = nullptr;
    PFNGLUNIFORM3FPROC             Uniform3f             = nullptr;
    PFNGLUNIFORM3FVPROC            Uniform3fv            = nullptr;
    PFNGLUNIFORMMATRIX4FVPROC      UniformMatrix4fv      = nullptr;
    PFNGLGENBUFFERSPROC            GenBuffers            = nullptr;
    PFNGLBINDBUFFERPROC            BindBuffer            = nullptr;
    PFNGLBUFFERDATAPROC            BufferData            = nullptr;
    PFNGLGENVERTEXARRAYSPROC       GenVertexArrays       = nullptr;
    PFNGLBINDVERTEXARRAYPROC       BindVertexArray       = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC   VertexAttribPointer   = nullptr;
    PFNGLACTIVETEXTUREPROC         ActiveTexture         = nullptr;
    PFNGLBINDATTRIBLOCATIONPROC    BindAttribLocation    = nullptr;
    PFNGLGETATTRIBLOCATIONPROC     GetAttribLocation     = nullptr;
    PFNGLGENERATEMIPMAPPROC        GenerateMipmap        = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC    DeleteVertexArrays    = nullptr;
    PFNGLDELETEBUFFERSPROC         DeleteBuffers         = nullptr;

    bool loaded = false;

    bool load() {
        if (loaded) return true;
#define LOAD(type, name) name = (type)gfxGetProcAddress("gl"#name); if(!name) { fprintf(stderr,"GL: gl"#name" missing\n"); return false; }
        LOAD(PFNGLGENFRAMEBUFFERSPROC,        GenFramebuffers)
        LOAD(PFNGLBINDFRAMEBUFFERPROC,        BindFramebuffer)
        LOAD(PFNGLFRAMEBUFFERTEXTURE2DPROC,   FramebufferTexture2D)
        LOAD(PFNGLFRAMEBUFFERRENDERBUFFERPROC,FramebufferRenderbuffer)
        LOAD(PFNGLCHECKFRAMEBUFFERSTATUSPROC, CheckFramebufferStatus)
        LOAD(PFNGLDELETEFRAMEBUFFERSPROC,     DeleteFramebuffers)
        LOAD(PFNGLDRAWBUFFERSPROC,            DrawBuffers)
        LOAD(PFNGLGENRENDERBUFFERSPROC,       GenRenderbuffers)
        LOAD(PFNGLBINDRENDERBUFFERPROC,       BindRenderbuffer)
        LOAD(PFNGLRENDERBUFFERSTORAGEPROC,    RenderbufferStorage)
        LOAD(PFNGLDELETERENDERBUFFERSPROC,    DeleteRenderbuffers)
        LOAD(PFNGLCREATESHADERPROC,           CreateShader)
        LOAD(PFNGLSHADERSOURCEPROC,           ShaderSource)
        LOAD(PFNGLCOMPILESHADERPROC,          CompileShader)
        LOAD(PFNGLGETSHADERIVPROC,            GetShaderiv)
        LOAD(PFNGLGETSHADERINFOLOGPROC,       GetShaderInfoLog)
        LOAD(PFNGLCREATEPROGRAMPROC,          CreateProgram)
        LOAD(PFNGLATTACHSHADERPROC,           AttachShader)
        LOAD(PFNGLLINKPROGRAMPROC,            LinkProgram)
        LOAD(PFNGLGETPROGRAMIVPROC,           GetProgramiv)
        LOAD(PFNGLGETPROGRAMINFOLOGPROC,      GetProgramInfoLog)
        LOAD(PFNGLUSEPROGRAMPROC,             UseProgram)
        LOAD(PFNGLDELETESHADERPROC,           DeleteShader)
        LOAD(PFNGLDELETEPROGRAMPROC,          DeleteProgram)
        LOAD(PFNGLGETUNIFORMLOCATIONPROC,     GetUniformLocation)
        LOAD(PFNGLUNIFORM1IPROC,              Uniform1i)
        LOAD(PFNGLUNIFORM1FPROC,              Uniform1f)
        LOAD(PFNGLUNIFORM2FPROC,              Uniform2f)
        LOAD(PFNGLUNIFORM3FPROC,              Uniform3f)
        LOAD(PFNGLUNIFORM3FVPROC,             Uniform3fv)
        LOAD(PFNGLUNIFORMMATRIX4FVPROC,       UniformMatrix4fv)
        LOAD(PFNGLGENBUFFERSPROC,             GenBuffers)
        LOAD(PFNGLBINDBUFFERPROC,             BindBuffer)
        LOAD(PFNGLBUFFERDATAPROC,             BufferData)
        LOAD(PFNGLGENVERTEXARRAYSPROC,        GenVertexArrays)
        LOAD(PFNGLBINDVERTEXARRAYPROC,        BindVertexArray)
        LOAD(PFNGLENABLEVERTEXATTRIBARRAYPROC,EnableVertexAttribArray)
        LOAD(PFNGLVERTEXATTRIBPOINTERPROC,    VertexAttribPointer)
        LOAD(PFNGLACTIVETEXTUREPROC,          ActiveTexture)
        LOAD(PFNGLBINDATTRIBLOCATIONPROC,     BindAttribLocation)
        LOAD(PFNGLGETATTRIBLOCATIONPROC,      GetAttribLocation)
        LOAD(PFNGLGENERATEMIPMAPPROC,         GenerateMipmap)
        LOAD(PFNGLDELETEVERTEXARRAYSPROC,     DeleteVertexArrays)
        LOAD(PFNGLDELETEBUFFERSPROC,          DeleteBuffers)
#undef LOAD
        loaded = true;
        return true;
    }
} gl;

// GL enums not always in gl.h
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER              0x8D40
#define GL_RENDERBUFFER             0x8D41
#define GL_COLOR_ATTACHMENT0        0x8CE0
#define GL_COLOR_ATTACHMENT1        0x8CE1
#define GL_COLOR_ATTACHMENT2        0x8CE2
#define GL_DEPTH_ATTACHMENT         0x8D00
#define GL_FRAMEBUFFER_COMPLETE     0x8CD5
#define GL_DEPTH_COMPONENT          0x1902
#define GL_DEPTH_COMPONENT24        0x81A5
#define GL_DEPTH_COMPONENT32        0x81A7
#define GL_FLOAT                    0x1406
#define GL_RGB16F                   0x881B
#define GL_RGBA16F                  0x881A
#define GL_RGB32F                   0x8815
#define GL_RGBA32F                  0x8814
#define GL_VERTEX_SHADER            0x8B31
#define GL_FRAGMENT_SHADER          0x8B30
#define GL_COMPILE_STATUS           0x8B81
#define GL_LINK_STATUS              0x8B82
#define GL_ARRAY_BUFFER             0x8892
#define GL_STATIC_DRAW              0x88B4
#define GL_DYNAMIC_DRAW             0x88E8
#define GL_CLAMP_TO_EDGE            0x812F
#define GL_TEXTURE0                 0x84C0
#define GL_TEXTURE1                 0x84C1
#define GL_TEXTURE2                 0x84C2
#define GL_TEXTURE3                 0x84C3
#define GL_TEXTURE4                 0x84C4
#define GL_TEXTURE5                 0x84C5
#define GL_DEPTH_COMPONENT16        0x81A5
#define GL_COMPARE_R_TO_TEXTURE     0x884E
#define GL_TEXTURE_COMPARE_MODE     0x884C
#define GL_TEXTURE_COMPARE_FUNC     0x884D
#define GL_LEQUAL                   0x0203
#define GL_R8                       0x8229
#define GL_RG                       0x8227
#define GL_RG8                      0x822B
#define GL_RGB8                     0x8051
#endif

// ─── ShaderProgram implementation ────────────────────────────────────────────
bool ShaderProgram::compile(const char* vert, const char* frag) {
    if (!gl.loaded) return false;

    auto compileStage = [&](GLenum type, const char* src) -> GLuint {
        GLuint sh = gl.CreateShader(type);
        gl.ShaderSource(sh, 1, &src, nullptr);
        gl.CompileShader(sh);
        GLint ok; gl.GetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            gl.GetShaderInfoLog(sh, sizeof(log), nullptr, log);
            fprintf(stderr, "[Shader] Compile error:\n%s\n", log);
            gl.DeleteShader(sh);
            return 0;
        }
        return sh;
    };

    GLuint vs = compileStage(GL_VERTEX_SHADER,   vert);
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, frag);
    if (!vs || !fs) { if(vs) gl.DeleteShader(vs); if(fs) gl.DeleteShader(fs); return false; }

    id = gl.CreateProgram();
    gl.AttachShader(id, vs);
    gl.AttachShader(id, fs);
    gl.LinkProgram(id);
    GLint ok; gl.GetProgramiv(id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        gl.GetProgramInfoLog(id, sizeof(log), nullptr, log);
        fprintf(stderr, "[Shader] Link error:\n%s\n", log);
        id = 0;
    }
    gl.DeleteShader(vs);
    gl.DeleteShader(fs);
    return id != 0;
}

void ShaderProgram::use() const {
    if (gl.loaded && id) gl.UseProgram(id);
}
void ShaderProgram::setInt(const char* n, int v) const {
    if (!gl.loaded || !id) return;
    GLint loc = gl.GetUniformLocation(id, n);
    if (loc >= 0) gl.Uniform1i(loc, v);
}
void ShaderProgram::setFloat(const char* n, float v) const {
    if (!gl.loaded || !id) return;
    GLint loc = gl.GetUniformLocation(id, n);
    if (loc >= 0) gl.Uniform1f(loc, v);
}
void ShaderProgram::setVec2(const char* n, float x, float y) const {
    if (!gl.loaded || !id) return;
    GLint loc = gl.GetUniformLocation(id, n);
    if (loc >= 0) gl.Uniform2f(loc, x, y);
}
void ShaderProgram::setVec3(const char* n, float x, float y, float z) const {
    if (!gl.loaded || !id) return;
    GLint loc = gl.GetUniformLocation(id, n);
    if (loc >= 0) gl.Uniform3f(loc, x, y, z);
}
void ShaderProgram::setVec3v(const char* n, const float* v, int count) const {
    if (!gl.loaded || !id) return;
    GLint loc = gl.GetUniformLocation(id, n);
    if (loc >= 0) gl.Uniform3fv(loc, count, v);
}
void ShaderProgram::setMat4(const char* n, const float* m) const {
    if (!gl.loaded || !id) return;
    GLint loc = gl.GetUniformLocation(id, n);
    if (loc >= 0) gl.UniformMatrix4fv(loc, 1, 0, m);
}

// ─── Framebuffer implementation ───────────────────────────────────────────────
bool Framebuffer::create(int w, int h, int numColor, bool hasDepthTex, bool isFloat) {
    if (!gl.loaded) return false;
    width = w; height = h; numColorAttachments = numColor;

    gl.GenFramebuffers(1, &fbo);
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);

    GLenum internalFmt = isFloat ? GL_RGB16F : GL_RGB8;
    GLenum fmt         = GL_RGB;
    GLenum dataType    = isFloat ? GL_FLOAT : GL_UNSIGNED_BYTE;

    // Color attachments
    glGenTextures(numColor, colorTex);
    for (int i = 0; i < numColor; i++) {
        glBindTexture(GL_TEXTURE_2D, colorTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, w, h, 0, fmt, dataType, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorTex[i], 0);
    }

    // Draw buffers
    if (numColor > 1) {
        GLenum bufs[4] = {
            GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3
        };
        gl.DrawBuffers(numColor, bufs);
    }

    // Depth
    if (hasDepthTex) {
        glGenTextures(1, &depthTex);
        glBindTexture(GL_TEXTURE_2D, depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
    } else {
        gl.GenRenderbuffers(1, &depthRbo);
        gl.BindRenderbuffer(GL_RENDERBUFFER, depthRbo);
        gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        gl.FramebufferRenderbuffer(GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo);
    }

    GLenum status = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

void Framebuffer::bind() const {
    if (gl.loaded && fbo) {
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, width, height);
    }
}
void Framebuffer::unbind() const {
    if (gl.loaded) gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Framebuffer::destroy() {
    if (!gl.loaded) return;
    if (fbo) { gl.DeleteFramebuffers(1, &fbo); fbo = 0; }
    if (numColorAttachments > 0) {
        glDeleteTextures(numColorAttachments, colorTex);
        memset(colorTex, 0, sizeof(colorTex));
    }
    if (depthTex) { glDeleteTextures(1, &depthTex); depthTex = 0; }
    if (depthRbo) { gl.DeleteRenderbuffers(1, &depthRbo); depthRbo = 0; }
}

// ─── GfxPipeline ──────────────────────────────────────────────────────────────
bool GfxPipeline::init(int w, int h) {
    screenW = w; screenH = h;
    renderW = w * settings.renderScale / 100;
    renderH = h * settings.renderScale / 100;

    // Load GL extensions
    if (!gl.load()) {
        fprintf(stderr, "[GfxPipeline] GL 2.0 extensions not available, using fallback\n");
        fallbackMode = true;
        initialized = true;
        return true; // graceful fallback, don't crash
    }

    if (!initShaders())      { fallbackMode = true; initialized = true; return true; }
    if (!initFramebuffers()) { fallbackMode = true; initialized = true; return true; }

    initSSAOKernel();
    generateNoiseTextures();
    buildQuadVAO();
    buildSkyDome();
    rain.init(RainSystem::MAX_DROPS);

    initialized = true;
    return true;
}

bool GfxPipeline::initShaders() {
    bool ok = true;
    ok &= sGeom.compile(GlslSrc::GBUF_VERT,   GlslSrc::GBUF_FRAG);
    ok &= sShadow.compile(GlslSrc::SHADOW_VERT, GlslSrc::SHADOW_FRAG);
    ok &= sLight.compile(GlslSrc::LIGHT_VERT,  GlslSrc::LIGHT_FRAG);
    ok &= sSky.compile(GlslSrc::SKY_VERT,     GlslSrc::SKY_FRAG);
    ok &= sSSAO.compile(GlslSrc::LIGHT_VERT,  GlslSrc::SSAO_FRAG);
    ok &= sGaussian.compile(GlslSrc::LIGHT_VERT, GlslSrc::GAUSSIAN_BLUR_FRAG);
    ok &= sBloomExtract.compile(GlslSrc::LIGHT_VERT, GlslSrc::BLOOM_EXTRACT_FRAG);
    ok &= sComposite.compile(GlslSrc::LIGHT_VERT, GlslSrc::COMPOSITE_FRAG);
    ok &= sRain.compile(GlslSrc::RAIN_VERT,   GlslSrc::RAIN_FRAG);
    ok &= sHaze.compile(GlslSrc::LIGHT_VERT,  GlslSrc::HAZE_FRAG);
    if (!ok) fprintf(stderr, "[GfxPipeline] Some shaders failed — using fallback\n");
    return ok;
}

bool GfxPipeline::initFramebuffers() {
    bool ok = true;
    // G-buffer: 3 float MRTs + depth
    ok &= fbGBuffer.create(renderW, renderH, 3, true, true);
    // Shadow map: depth only (special case)
    {
        gl.GenFramebuffers(1, &fbShadow.fbo);
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbShadow.fbo);
        glGenTextures(1, &fbShadow.depthTex);
        glBindTexture(GL_TEXTURE_2D, fbShadow.depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                     SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                GL_TEXTURE_2D, fbShadow.depthTex, 0);
        glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
        ok &= (gl.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        fbShadow.width = fbShadow.height = SHADOW_MAP_SIZE;
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    ok &= fbSSAO.create(renderW, renderH, 1, false, true);
    ok &= fbSSAOBlur.create(renderW, renderH, 1, false, true);
    ok &= fbLight.create(renderW, renderH, 1, false, true);
    ok &= fbBloomA.create(renderW/BLOOM_DOWNSAMPLE, renderH/BLOOM_DOWNSAMPLE, 1, false, true);
    ok &= fbBloomB.create(renderW/BLOOM_DOWNSAMPLE, renderH/BLOOM_DOWNSAMPLE, 1, false, true);
    ok &= fbHaze.create(renderW, renderH, 1, false, true);
    return ok;
}

void GfxPipeline::initSSAOKernel() {
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    for (int i = 0; i < SSAO_KERNELS; i++) {
        float x = dist(rng)*2.f - 1.f;
        float y = dist(rng)*2.f - 1.f;
        float z = dist(rng);
        float len = sqrtf(x*x+y*y+z*z);
        if (len > 0.001f) { x/=len; y/=len; z/=len; }
        float scale = (float)i / SSAO_KERNELS;
        scale = 0.1f + scale*scale*0.9f;
        ssaoKernel[i*3+0] = x * scale;
        ssaoKernel[i*3+1] = y * scale;
        ssaoKernel[i*3+2] = z * scale;
    }
}

void GfxPipeline::generateNoiseTextures() {
    // SSAO noise (4x4 random tangent vectors)
    std::mt19937 rng(123);
    std::uniform_real_distribution<float> d(-1.f, 1.f);
    float noiseData[SSAO_NOISE_SIZE * SSAO_NOISE_SIZE * 3];
    for (int i = 0; i < SSAO_NOISE_SIZE * SSAO_NOISE_SIZE; i++) {
        noiseData[i*3+0] = d(rng);
        noiseData[i*3+1] = d(rng);
        noiseData[i*3+2] = 0.f;
    }
    glGenTextures(1, &texSSAONoise);
    glBindTexture(GL_TEXTURE_2D, texSSAONoise);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SSAO_NOISE_SIZE, SSAO_NOISE_SIZE,
                 0, GL_RGB, GL_FLOAT, noiseData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x8370 /*GL_REPEAT*/);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x8370);

    // Blue noise (tileable random texture for film grain + dithering)
    std::mt19937 rng2(456);
    std::uniform_real_distribution<float> d2(0.f, 1.f);
    const int BN = 64;
    std::vector<float> bnData(BN * BN * 3);
    for (auto& v : bnData) v = d2(rng2);
    glGenTextures(1, &texBlueNoise);
    glBindTexture(GL_TEXTURE_2D, texBlueNoise);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, BN, BN, 0, GL_RGB, GL_FLOAT, bnData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x8370);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x8370);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void GfxPipeline::buildQuadVAO() {
    float quad[] = {
        -1,-1,  1,-1,  -1,1,
         1,-1,  1, 1,  -1,1
    };
    gl.GenVertexArrays(1, &vaoQuad);
    gl.GenBuffers(1, &vboQuad);
    gl.BindVertexArray(vaoQuad);
    gl.BindBuffer(GL_ARRAY_BUFFER, vboQuad);
    gl.BufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    gl.EnableVertexAttribArray(0);
    gl.VertexAttribPointer(0, 2, GL_FLOAT, 0, 0, nullptr);
    gl.BindVertexArray(0);
}

void GfxPipeline::buildSkyDome() {
    // Icosphere-like sky: just a large cube mapped sphere
    const int SLICES = 32, STACKS = 16;
    std::vector<float> verts;
    for (int j = 0; j <= STACKS; j++) {
        float phi = -1.5708f + 3.14159f * j / STACKS;
        for (int i = 0; i <= SLICES; i++) {
            float theta = 2*3.14159f * i / SLICES;
            float x = cosf(phi) * cosf(theta);
            float y = sinf(phi);
            float z = cosf(phi) * sinf(theta);
            verts.push_back(x*500); verts.push_back(y*500); verts.push_back(z*500);
        }
    }
    std::vector<unsigned int> idx;
    for (int j = 0; j < STACKS; j++) {
        for (int i = 0; i < SLICES; i++) {
            int a = j*(SLICES+1)+i, b = a+1;
            int c = (j+1)*(SLICES+1)+i, d = c+1;
            idx.push_back(a); idx.push_back(c); idx.push_back(b);
            idx.push_back(b); idx.push_back(c); idx.push_back(d);
        }
    }
    gl.GenVertexArrays(1, &vaoSkyDome);
    gl.GenBuffers(1, &vboSkyDome);
    // Store index count in vboSkyDome+1 slot by convention (simple hack)
    gl.BindVertexArray(vaoSkyDome);
    unsigned int ibo;
    gl.GenBuffers(1, &ibo);
    gl.BindBuffer(GL_ARRAY_BUFFER, vboSkyDome);
    gl.BufferData(GL_ARRAY_BUFFER, verts.size()*4, verts.data(), GL_STATIC_DRAW);
    gl.BindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, ibo);
    gl.BufferData(0x8893, idx.size()*4, idx.data(), GL_STATIC_DRAW);
    gl.EnableVertexAttribArray(0);
    gl.VertexAttribPointer(0, 3, GL_FLOAT, 0, 0, nullptr);
    gl.BindVertexArray(0);
}

void GfxPipeline::drawQuad() {
    gl.BindVertexArray(vaoQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    gl.BindVertexArray(0);
}

Vec3 GfxPipeline::getSunDir() const {
    float yaw   = settings.sunYaw;
    float pitch = settings.sunPitch * (0.5f + settings.timeOfDay * 0.5f);
    return Vec3(sinf(yaw)*cosf(pitch), sinf(pitch), cosf(yaw)*cosf(pitch));
}

Vec3 GfxPipeline::getSkyZenith() const {
    float t = settings.timeOfDay;
    // Night to day gradient
    Vec3 night  {0.01f, 0.02f, 0.08f};
    Vec3 sunrise{0.7f,  0.35f, 0.15f};
    Vec3 day    {0.20f, 0.45f, 0.75f};
    if (t < 0.3f) return Vec3::lerp(night,   sunrise, t/0.3f);
    if (t < 0.6f) return Vec3::lerp(sunrise, day,     (t-0.3f)/0.3f);
    return day;
}
Vec3 GfxPipeline::getSkyHorizon() const {
    float t = settings.timeOfDay;
    Vec3 night  {0.02f, 0.03f, 0.12f};
    Vec3 sunrise{1.0f,  0.55f, 0.20f};
    Vec3 day    {0.55f, 0.72f, 0.90f};
    if (t < 0.3f) return Vec3::lerp(night,   sunrise, t/0.3f);
    if (t < 0.6f) return Vec3::lerp(sunrise, day,     (t-0.3f)/0.3f);
    return day;
}
Vec3 GfxPipeline::getFogColor() const {
    Vec3 h = getSkyHorizon();
    return h * 0.8f;
}

void GfxPipeline::updateLightSpaceMatrix(Vec3 sunDir, Vec3 camPos) {
    // Orthographic projection from sun's perspective
    Vec3 lightPos = camPos + sunDir * (-80.f);
    Vec3 up = fabsf(sunDir.y) < 0.99f ? Vec3(0,1,0) : Vec3(1,0,0);
    Vec3 f  = (camPos - lightPos).normalized();
    Vec3 r  = f.cross(up).normalized();
    Vec3 u  = r.cross(f);

    float sz = 120.f;
    // Build ortho * view matrix
    // Ortho:
    float ortho[16] = {
        2.f/sz/2,0,0,0,
        0,2.f/sz/2,0,0,
        0,0,-2.f/(200.f),0,
        0,0,0,1
    };
    // View:
    float lx = -r.dot(lightPos), ly = -u.dot(lightPos), lz = f.dot(lightPos);
    float view[16] = {
        r.x, u.x, -f.x, 0,
        r.y, u.y, -f.y, 0,
        r.z, u.z, -f.z, 0,
        lx,  ly,  lz,   1
    };
    // lightSpaceMat = ortho * view (row-major mult)
    for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) {
        float s = 0;
        for (int k = 0; k < 4; k++)
            s += ortho[i*4+k] * view[k*4+j];
        lightSpaceMat[i*4+j] = s;
    }
}

// ─── Render pass control ──────────────────────────────────────────────────────
void GfxPipeline::beginShadowPass() {
    if (fallbackMode || !settings.enableShadows) return;
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbShadow.fbo);
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);
    sShadow.use();
}

void GfxPipeline::endShadowPass() {
    if (fallbackMode || !settings.enableShadows) return;
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    glCullFace(GL_BACK);
}

void GfxPipeline::beginGeometryPass() {
    if (fallbackMode) return;
    fbGBuffer.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    sGeom.use();
    sGeom.setFloat("uTime", 0.f);
}

void GfxPipeline::endGeometryPass() {
    if (fallbackMode) return;
    fbGBuffer.unbind();
}

void GfxPipeline::runLightingPass(Vec3 camPos, Vec3 sunDir, float gameTime) {
    if (fallbackMode) return;

    // SSAO
    if (settings.enableSSAO) runSSAO(nullptr);

    // Lighting
    fbLight.bind();
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    sLight.use();

    // Bind G-buffer textures
    gl.ActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, fbGBuffer.colorTex[0]);
    gl.ActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, fbGBuffer.colorTex[1]);
    gl.ActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, fbGBuffer.colorTex[2]);
    gl.ActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, fbShadow.depthTex);
    gl.ActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D,
        settings.enableSSAO ? fbSSAOBlur.colorTex[0] : 0);
    gl.ActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, texBlueNoise);

    sLight.setInt("uGAlbedo",    0);
    sLight.setInt("uGNormal",    1);
    sLight.setInt("uGPosition",  2);
    sLight.setInt("uShadowMap",  3);
    sLight.setInt("uSSAO",       4);
    sLight.setInt("uBlueNoise",  5);

    sLight.setVec3("uCamPos",   camPos.x, camPos.y, camPos.z);
    sLight.setVec3("uSunDir",   sunDir.x, sunDir.y, sunDir.z);

    Vec3 zenith  = getSkyZenith();
    Vec3 horizon = getSkyHorizon();
    Vec3 fog     = getFogColor();

    // Sun color: warm at sunrise, white at noon
    float t = settings.timeOfDay;
    Vec3 sunCol = t < 0.4f ?
        Vec3::lerp({1.f,0.5f,0.1f},{1.f,0.95f,0.8f}, t/0.4f) :
        Vec3{1.f, 0.95f, 0.85f};

    sLight.setVec3("uSunColor",  sunCol.x,  sunCol.y,  sunCol.z);
    sLight.setVec3("uSkyColor",  zenith.x,  zenith.y,  zenith.z);
    sLight.setVec3("uFogColor",  fog.x,     fog.y,     fog.z);
    sLight.setFloat("uFogDensity",  settings.fogDensity);
    sLight.setFloat("uTime",        gameTime);
    sLight.setFloat("uExposure",    settings.exposure);
    sLight.setInt("uEnableShadows", settings.enableShadows ? 1 : 0);
    sLight.setInt("uEnableSSAO",    settings.enableSSAO    ? 1 : 0);
    sLight.setMat4("uLightSpaceMat", lightSpaceMat);

    drawQuad();
    fbLight.unbind();
    glEnable(GL_DEPTH_TEST);
}

void GfxPipeline::runSSAO(const float* projMat) {
    if (!settings.enableSSAO) return;

    fbSSAO.bind();
    glClear(GL_COLOR_BUFFER_BIT);
    sSSAO.use();

    gl.ActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, fbGBuffer.colorTex[2]);
    gl.ActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, fbGBuffer.colorTex[1]);
    gl.ActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, texSSAONoise);
    sSSAO.setInt("uGPosition",  0);
    sSSAO.setInt("uGNormal",    1);
    sSSAO.setInt("uNoise",      2);
    sSSAO.setVec2("uNoiseScale",
        (float)renderW / SSAO_NOISE_SIZE,
        (float)renderH / SSAO_NOISE_SIZE);
    sSSAO.setFloat("uRadius", settings.ssaoRadius);
    sSSAO.setFloat("uBias",   settings.ssaoBias);
    sSSAO.setVec3v("uKernel[0]", ssaoKernel, SSAO_KERNELS);
    drawQuad();
    fbSSAO.unbind();

    // Blur SSAO
    fbSSAOBlur.bind();
    glClear(GL_COLOR_BUFFER_BIT);
    sGaussian.use();
    gl.ActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, fbSSAO.colorTex[0]);
    sGaussian.setInt("uTex", 0);
    sGaussian.setVec2("uDirection", 1.f, 0.f);
    sGaussian.setFloat("uTexelSize", 1.f / renderW);
    drawQuad();
    fbSSAOBlur.unbind();
}

void GfxPipeline::runPostProcessing(float gameTime) {
    if (fallbackMode) return;

    // Bloom
    if (settings.enableBloom) runBloom();

    // Final composite to default framebuffer
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenW, screenH);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    sComposite.use();
    gl.ActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, fbLight.colorTex[0]);
    gl.ActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,
        settings.enableBloom ? fbBloomB.colorTex[0] : 0);
    gl.ActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, texBlueNoise);

    sComposite.setInt("uScene",   0);
    sComposite.setInt("uBloom",   1);
    sComposite.setInt("uBlueNoise",2);
    sComposite.setFloat("uTime",  gameTime);
    sComposite.setFloat("uBloomStrength",         settings.bloomStrength);
    sComposite.setFloat("uChromaticAberration",   settings.chromaticAberration);
    sComposite.setFloat("uVignetteStrength",      settings.vignetteStrength);
    sComposite.setFloat("uFilmGrain",             settings.filmGrain);
    sComposite.setInt("uEnableBloom",             settings.enableBloom?1:0);
    sComposite.setInt("uEnableChromaticAberration",settings.enableChromaticAberration?1:0);
    sComposite.setInt("uEnableVignette",          settings.enableVignette?1:0);
    sComposite.setInt("uEnableFilmGrain",         settings.enableFilmGrain?1:0);
    sComposite.setInt("uEnableDepthOfField",      0);

    drawQuad();
    glEnable(GL_DEPTH_TEST);

    // Restore active texture to 0
    gl.ActiveTexture(GL_TEXTURE0);
}

void GfxPipeline::runBloom() {
    float threshold = settings.bloomThreshold;
    // Extract bright pixels
    fbBloomA.bind();
    glClear(GL_COLOR_BUFFER_BIT);
    sBloomExtract.use();
    gl.ActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, fbLight.colorTex[0]);
    sBloomExtract.setInt("uScene", 0);
    sBloomExtract.setFloat("uThreshold", threshold);
    drawQuad();
    fbBloomA.unbind();

    // Ping-pong blur
    bool horizontal = true;
    for (int i = 0; i < 6; i++) {
        Framebuffer& dst = horizontal ? fbBloomB : fbBloomA;
        Framebuffer& src = horizontal ? fbBloomA : fbBloomB;
        dst.bind();
        sGaussian.use();
        gl.ActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, src.colorTex[0]);
        sGaussian.setInt("uTex", 0);
        sGaussian.setVec2("uDirection", horizontal?1.f:0.f, horizontal?0.f:1.f);
        sGaussian.setFloat("uTexelSize", 1.f / (horizontal ? fbBloomA.width : fbBloomA.height));
        drawQuad();
        dst.unbind();
        horizontal = !horizontal;
    }
}

void GfxPipeline::drawSky(const float* viewMat, const float* projMat, Vec3 camPos) {
    if (fallbackMode) return;
    glDepthMask(GL_FALSE);
    sSky.use();
    sSky.setMat4("uViewRot", viewMat);
    sSky.setMat4("uProj",    projMat);
    Vec3 sunDir = getSunDir();
    sSky.setVec3("uSunDir",     sunDir.x,        sunDir.y,        sunDir.z);
    Vec3 zen = getSkyZenith(), hor = getSkyHorizon();
    sSky.setVec3("uSkyZenith",  zen.x, zen.y, zen.z);
    sSky.setVec3("uSkyHorizon", hor.x, hor.y, hor.z);
    sSky.setVec3("uGroundColor", 0.25f, 0.22f, 0.18f);
    sSky.setFloat("uTime",         settings.timeOfDay * 1000.f);
    sSky.setFloat("uCloudSpeed",   settings.cloudSpeed);
    sSky.setFloat("uCloudCoverage",settings.cloudCoverage);
    sSky.setFloat("uCloudSharpness",settings.cloudSharpness);

    gl.BindVertexArray(vaoSkyDome);
    glDrawElements(GL_TRIANGLES, 32*16*6, GL_UNSIGNED_INT, nullptr);
    gl.BindVertexArray(0);
    glDepthMask(GL_TRUE);
}

bool GfxPipeline::bindGeometryShader() {
    if (fallbackMode) return false;
    sGeom.use();
    return true;
}

bool GfxPipeline::bindShadowShader() {
    if (fallbackMode || !settings.enableShadows) return false;
    sShadow.use();
    return true;
}

void GfxPipeline::setObjectUniforms(const float* modelMat, Vec3 color,
                                     float roughness, float metallic,
                                     float emissive, bool isGlass, bool isWindow) {
    if (fallbackMode) return;
    sGeom.setMat4("uModel", modelMat);
    sGeom.setVec3("aColor_u", color.x, color.y, color.z); // fallback if attrib not bound
    sGeom.setFloat("uRoughness", roughness);
    sGeom.setFloat("uMetallic",  metallic);
    sGeom.setFloat("uEmissive",  emissive);
    sGeom.setInt("uIsGlass",   isGlass   ? 1 : 0);
    sGeom.setInt("uIsWindow",  isWindow  ? 1 : 0);
}

void GfxPipeline::resize(int w, int h) {
    screenW = w; screenH = h;
    renderW = w * settings.renderScale / 100;
    renderH = h * settings.renderScale / 100;
    if (!fallbackMode && initialized) {
        fbGBuffer.destroy(); fbSSAO.destroy(); fbSSAOBlur.destroy();
        fbLight.destroy();   fbBloomA.destroy(); fbBloomB.destroy(); fbHaze.destroy();
        initFramebuffers();
    }
    glViewport(0, 0, w, h);
}

void GfxPipeline::destroy() {
    if (!initialized) return;
    fbGBuffer.destroy(); fbShadow.destroy();
    fbSSAO.destroy();    fbSSAOBlur.destroy();
    fbLight.destroy();   fbBloomA.destroy();
    fbBloomB.destroy();  fbHaze.destroy();
    if (texSSAONoise) glDeleteTextures(1, &texSSAONoise);
    if (texBlueNoise) glDeleteTextures(1, &texBlueNoise);
    if (gl.loaded) {
        if (vaoQuad)    { gl.DeleteVertexArrays(1,&vaoQuad);    gl.DeleteBuffers(1,&vboQuad); }
        if (vaoSkyDome) { gl.DeleteVertexArrays(1,&vaoSkyDome); gl.DeleteBuffers(1,&vboSkyDome); }
    }
    initialized = false;
}

// ─── Rain system ──────────────────────────────────────────────────────────────
void RainSystem::init(int count) {
    drops.resize(count);
    Vec3 dummy{0,0,0};
    for (auto& d : drops) reset(d, dummy);
}

void RainSystem::reset(Drop& d, Vec3 camPos) {
    d.pos.x = camPos.x + ((float)rand()/RAND_MAX - 0.5f) * 60.f;
    d.pos.y = camPos.y + 15.f + ((float)rand()/RAND_MAX) * 15.f;
    d.pos.z = camPos.z + ((float)rand()/RAND_MAX - 0.5f) * 60.f;
    d.life  = 1.f + (float)rand()/RAND_MAX * 2.f;
    d.alpha = 0.3f + (float)rand()/RAND_MAX * 0.4f;
}

void RainSystem::update(float dt, Vec3 camPos, float intensity, float angle) {
    float speed = 25.f * intensity;
    for (auto& d : drops) {
        d.pos.y -= speed * dt;
        d.pos.x += sinf(angle) * speed * dt * 0.5f;
        d.life -= dt;
        if (d.life <= 0.f || d.pos.y < 0.f) reset(d, camPos);
    }
}

void GfxPipeline::updateRain(float dt, Vec3 camPos) {
    if (!settings.enableRain) return;
    rain.update(dt, camPos, settings.rainIntensity, settings.rainAngle);
}

void GfxPipeline::drawRain(const float* mvp) {
    if (!settings.enableRain || fallbackMode) return;
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.2f);

    sRain.use();
    sRain.setMat4("uMVP", mvp);
    sRain.setVec3("uRainColor", 0.65f, 0.72f, 0.85f);

    // Draw as lines
    std::vector<float> verts;
    verts.reserve(rain.drops.size() * 8);
    for (const auto& d : rain.drops) {
        float len = 0.5f * settings.rainIntensity;
        verts.push_back(d.pos.x); verts.push_back(d.pos.y);
        verts.push_back(d.pos.z); verts.push_back(d.alpha);
        verts.push_back(d.pos.x + sinf(settings.rainAngle)*len);
        verts.push_back(d.pos.y - len);
        verts.push_back(d.pos.z); verts.push_back(0.f);
    }

    unsigned int vbo;
    gl.GenBuffers(1, &vbo);
    gl.BindBuffer(GL_ARRAY_BUFFER, vbo);
    gl.BufferData(GL_ARRAY_BUFFER, verts.size()*4, verts.data(), GL_DYNAMIC_DRAW);

    GLint posLoc = gl.GetAttribLocation(sRain.id, "aPos");
    GLint alpLoc = gl.GetAttribLocation(sRain.id, "aAlpha");
    if (posLoc >= 0) {
        gl.EnableVertexAttribArray(posLoc);
        gl.VertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 4*4, nullptr);
    }
    if (alpLoc >= 0) {
        gl.EnableVertexAttribArray(alpLoc);
        gl.VertexAttribPointer(alpLoc, 1, GL_FLOAT, 0, 4*4, (void*)(3*4));
    }
    glDrawArrays(GL_LINES, 0, (GLsizei)(rain.drops.size() * 2));
    gl.DeleteBuffers(1, &vbo);
    glEnable(GL_LIGHTING);
}
