#include "../include/core.hpp"

typedef GLXContext (*glXCreateContextAttribsARBProc) (Display*, GLXFBConfig, GLXContext, Bool, const int* );

static PFNGLXBINDTEXIMAGEEXTPROC glXBindTexImageEXT_func = NULL;
static PFNGLXRELEASETEXIMAGEEXTPROC glXReleaseTexImageEXT_func = NULL;

GLXFBConfig GLXWorker::fbconfigs[33];



GLuint GLXWorker::loadImage (char* path) {
    GLuint textureID;
    ILuint imageID;

    ilGenImages ( 1, &imageID );
    ilBindImage ( imageID );
    if ( !ilLoadImage ( path ) ) {
        err << "Can't open image " << path;
        std::exit ( 1 );
    }

    ILinfo infoi;
    iluGetImageInfo ( &infoi );
    if ( infoi.Origin == IL_ORIGIN_UPPER_LEFT )
        iluFlipImage();

    if ( !ilConvertImage ( IL_RGB, IL_UNSIGNED_BYTE ) )
        err << "Can't convert image!", std::exit ( 1 );

    glGenTextures ( 1, &textureID );
    printf ( "texture id : %d \n", textureID );
    glBindTexture ( GL_TEXTURE_2D, textureID );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D ( GL_TEXTURE_2D, 0,
            ilGetInteger ( IL_IMAGE_FORMAT ),
            ilGetInteger ( IL_IMAGE_WIDTH ),
            ilGetInteger ( IL_IMAGE_HEIGHT ),
            0,
            ilGetInteger ( IL_IMAGE_FORMAT ),
            GL_UNSIGNED_BYTE,
            ilGetData() );
    return textureID;
}


void GLXWorker::createNewContext(Window win) {
    int dummy;
    GLXFBConfig *fbconfig = glXGetFBConfigs(core->d,
            DefaultScreen(core->d), &dummy);

    if ( fbconfig == NULL )
        printf ( "Couldn't get FBConfigs list!\n" ),
               std::exit (-1);

    auto xvi = WindowWorker::getVisualInfoForWindow(win);
    int i = 0;
    for (; i < dummy; i++ ) {
        auto id = glXGetVisualFromFBConfig(core->d, fbconfig[i]);
        if ( xvi->visualid == id->visualid )
            break;
    }

    if ( i == dummy ) {
        err << "failed to get FBConfig for opengl 3+ context!";
        std::exit(-1);
    }


    int ogl33[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 4,
        GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, None
    };
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB =
        (glXCreateContextAttribsARBProc) glXGetProcAddressARB(
                (const GLubyte *) "glXCreateContextAttribsARB" );
    GLXContext context = glXCreateContextAttribsARB (core->d,
            fbconfig[i], NULL, true, ogl33 );

    glXMakeCurrent ( core->d, win, context );

}
void GLXWorker::createDefaultContext(Window win) {
    auto xvi = WindowWorker::getVisualInfoForWindow(win);
    GLXContext ctx = glXCreateContext(core->d, xvi, NULL, 0);
    glXMakeCurrent(core->d, win, ctx);
    XFree(xvi);
}


#define uchar unsigned char


void GLXWorker::initGLX() {

    auto x = glXGetCurrentContext();
    if ( x == NULL ) 
        err << "current context is NULL!!!",
            std::exit(-1);
    glewExperimental = GL_TRUE;
    auto res = glewInit();
    glGetError(); // workaround for glew getting Invalid Enum
    if ( res != GLEW_OK ) {
        err << "failed to init glew";
        std::exit(-1);
    }

    ilInit();
    iluInit();
    ilutInit();
    ilutRenderer ( ILUT_OPENGL );

    
    glXBindTexImageEXT_func = (PFNGLXBINDTEXIMAGEEXTPROC)
        glXGetProcAddress((GLubyte *) "glXBindTexImageEXT");
    glXReleaseTexImageEXT_func = (PFNGLXRELEASETEXIMAGEEXTPROC)
        glXGetProcAddress((GLubyte*) "glXReleaseTexImageEXT");

    initFBConf();

}

GLuint GLXWorker::compileShader(const char *src, GLuint type) {
    GLuint shader = glCreateShader(type); 
    glShaderSource ( shader, 1, &src, NULL );

    int s;
    char b1[512];
    glCompileShader ( shader );
    glGetShaderiv ( shader, GL_COMPILE_STATUS, &s );
    glGetShaderInfoLog ( shader, 512, NULL, b1 );


    if ( s == GL_FALSE ) {
        std::stringstream srcStream, errorStream;
        std::string line;
        err << "shader compilation failed!";
        err << "src: *****************************";
        srcStream << src;
        while(std::getline(srcStream, line))
            err << line;
        err << "**********************************";
        errorStream << b1;
        while(std::getline(errorStream, line))
            err << line;
        err << "**********************************";
        return -1;
    }
    return shader;
}

GLuint GLXWorker::loadShader(const char *path, GLuint type) {
    std::fstream file(path);
    if(!file.is_open())
        err << "Cannot open shader file. Aborting", std::exit(1);
    std::string str, line; 
    
    while(std::getline(file, line))
        str += line, str += '\n';

    return GLXWorker::compileShader(str.c_str(), type);
}


void GLXWorker::endFrame(Window win) {
    glXSwapBuffers(core->d, win);
}

GLuint GLXWorker::textureFromPixmap(Pixmap pixmap,
        int w, int h, XVisualInfo* xvi) {

    auto fbconf = fbconfigs[xvi->depth]; 
    if (fbconf == nullptr)
        return -1;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
//    glGenerateTextureMipmap(tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_MIPMAP);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_MIPMAP);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


    auto gpix = glxPixmap(pixmap, fbconf, w, h);
    //err << "generated glxpixmap";

    glXBindTexImageEXT_func (core->d, gpix, GLX_FRONT_LEFT_EXT, NULL);

    //err << "bound pixmap to texture";
    glXDestroyPixmap(core->d, gpix);

    return tex;    
}

void GLXWorker::initFBConf() {
    int n;
    auto fbConfigs = glXGetFBConfigs(core->d, DefaultScreen(core->d), &n);
    for (int i = 0; i <= 32; i++)
    {
        int db, stencil, depth = 0, alpha = 0;
        int value;

        fbconfigs[i] = nullptr;

        db      = 10000;
        stencil = 10000;
        depth   = 10000;

        for (int j = 0; j < n; j++)
        {
            XVisualInfo *vi;
            int     visualDepth;

            vi = glXGetVisualFromFBConfig (core->d, fbConfigs[j]);
            if (vi == NULL)
                continue; 

            visualDepth = vi->depth;

            XFree (vi);

            if (visualDepth != i)
                continue;

            glXGetFBConfigAttrib (core->d, fbConfigs[j],
                    GLX_ALPHA_SIZE, &alpha);
            glXGetFBConfigAttrib (core->d, fbConfigs[j],
                    GLX_BUFFER_SIZE, &value);
            if (value != i && (value - alpha) != i)
                continue; 

            value = 0;
            if (i == 32) {
                glXGetFBConfigAttrib (core->d, fbConfigs[j],
                        GLX_BIND_TO_TEXTURE_RGBA_EXT, &value);

                if (!value) {

                    glXGetFBConfigAttrib (core->d, fbConfigs[j],
                            GLX_BIND_TO_TEXTURE_RGB_EXT, &value);
                    if (!value)  
                        continue;
                }   
            }   

            glXGetFBConfigAttrib (core->d, fbConfigs[j],
                    GLX_DOUBLEBUFFER, &value);
            if (value > db)
                continue; 

            db = value;

            glXGetFBConfigAttrib (core->d, fbConfigs[j],
                    GLX_STENCIL_SIZE, &value);
            if (value > stencil)
                continue; 

            stencil = value;

            glXGetFBConfigAttrib (core->d, fbConfigs[j],
                    GLX_DEPTH_SIZE, &value);
            if (value > depth)
                continue; 

            depth = value;


            glXGetFBConfigAttrib (core->d,
                    fbConfigs[j],
                    GLX_BIND_TO_TEXTURE_TARGETS_EXT,
                    &value);
            if(!value)
                continue;

            fbconfigs[i] = fbConfigs[j];
        }
    }
}


GLXPixmap GLXWorker::glxPixmap(Pixmap pixmap, GLXFBConfig config, int w, int h) {
   const int pixmapAttribs[] = {
      GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
      GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
      None
   };

   return glXCreatePixmap(core->d, config, pixmap, pixmapAttribs);
}




