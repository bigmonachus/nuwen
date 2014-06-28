// 2014 Sergio Gonzalez

#include "ph_vr.h"
#include "ph_gl.h"

namespace ph {
namespace vr {

static GLuint m_quad_vao;
static GLuint m_quad_program;
static GLuint m_compute_program;
static GLuint m_size[2];
/* static int g_warp_size[2] = {8, 4}; */

GLuint init(int width, int height, const char** shader_paths, int num_shaders) {
    enum Location {
        Location_pos,
        Location_tex,
    };

    m_size[0] = (GLuint)width;
    m_size[1] = (GLuint)height;

    // Create / fill texture
    {
        // Create texture
        GLuint texobj;
        GLCHK (glActiveTexture (GL_TEXTURE0) );
        GLCHK (glGenTextures   (1, &texobj) );
        GLCHK (glBindTexture   (GL_TEXTURE_2D, texobj) );

        // Note for the future: These are needed.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Pass a null pointer, texture will be filled by compute shader
        // Note: Internal format was vital for compute shader.
        GLCHK ( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height,
                    0, GL_RGBA, GL_FLOAT, NULL) );

        // Bind it to image unit
        glBindImageTexture(0, texobj, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    }
    // Create main program
    {
        enum {
            vert,
            frag,
        };
        GLuint shaders[2];
        const char* path[2];
        path[vert] = "glsl/quad.v.glsl";
        path[frag] = "glsl/quad.f.glsl";
        shaders[vert] = ph::gl::compile_shader(path[vert], GL_VERTEX_SHADER);
        shaders[frag] = ph::gl::compile_shader(path[frag], GL_FRAGMENT_SHADER);

        m_quad_program = glCreateProgram();

        ph::gl::link_program(m_quad_program, shaders, 2); GLCHK();

        ph_expect(Location_pos == glGetAttribLocation(m_quad_program, "position"));
        ph_expect(Location_tex == glGetUniformLocation(m_quad_program, "tex"));

        GLCHK ( glUseProgram(m_quad_program) );
        glUniform1i(Location_tex, /*GL_TEXTURE_0*/0);

    }
    // Create a quad.
    {
        glPointSize(3);
        const GLfloat u = 1.0f;
        GLfloat vert_data[] = {
            -u, u,
            -u, -u,
            u, -u,
            u, u,
        };
        glGenVertexArrays(1, &m_quad_vao);
        glBindVertexArray(m_quad_vao);

        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        GLCHK (glBufferData (GL_ARRAY_BUFFER, sizeof(vert_data), vert_data, GL_STATIC_DRAW));

        GLCHK (glEnableVertexAttribArray (Location_pos) );
        GLCHK (glVertexAttribPointer     (/*attrib location*/Location_pos,
                    /*size*/2, GL_FLOAT, /*normalize*/GL_FALSE, /*stride*/0, /*ptr*/0));
    }
    // Create compute shader
    {
        GLuint* shaders = phalloc(GLuint, num_shaders);
        for (int i = 0; i < num_shaders; ++i) {
            shaders[i] = gl::compile_shader(shader_paths[i], GL_COMPUTE_SHADER);
        }

        m_compute_program = glCreateProgram();
        ph::gl::link_program(m_compute_program, shaders, num_shaders);
        phree(shaders);

        ph_expect(Location_tex == glGetUniformLocation(m_compute_program, "tex"));
        ph_expect(0 == glGetUniformLocation(m_compute_program, "screen_size"));

        glUseProgram(m_compute_program);

        GLCHK ( glUniform1i(Location_tex, 0) );  // Location: 0, Texture Unit: 0
        float fsize[2] = {(float)width, (float)height};
        glUniform2fv(0, 1, &fsize[0]);
    }
    return m_compute_program;
}

void draw() {
    // Draw screen quad
    {
        GLCHK (glUseProgram      (vr::m_quad_program) );
        GLCHK (glBindVertexArray (vr::m_quad_vao) );
        GLCHK (glDrawArrays      (GL_TRIANGLE_FAN, 0, 4) );
    }
}


} // ns vr
} // ns ph
