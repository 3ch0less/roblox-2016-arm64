#include "ContextGL.h"

#include "HeadersGL.h"

#include "rbx/Debug.h"

#include <AppKit/AppKit.h>
#include <OpenGL/OpenGL.h>

LOGGROUP(Graphics)

FASTFLAGVARIABLE(UpdateContextOnFollowingFrame, false)

FASTFLAG(GraphicsGL3)

namespace RBX
{
namespace Graphics
{

static NSOpenGLPixelFormat* createPixelFormatWithApi(unsigned int api)
{
    NSOpenGLPixelFormatAttribute attribs[32];
    int ai = 0;
    
    // Specify the display ID to associate the GL context with main display. Useful if there is ambiguity
    attribs[ai++] = NSOpenGLPFAScreenMask;
    attribs[ai++] = CGDisplayIDToOpenGLDisplayMask(CGMainDisplayID());
    
    // Specifying "NoRecovery" gives us a context that cannot fall back to the software renderer.
    attribs[ai++] = NSOpenGLPFANoRecovery;
    
    attribs[ai++] = NSOpenGLPFAAccelerated;
    attribs[ai++] = NSOpenGLPFADoubleBuffer;
    
    attribs[ai++] = NSOpenGLPFAColorSize;
    attribs[ai++] = 24;
    
    attribs[ai++] = NSOpenGLPFAAlphaSize;
    attribs[ai++] = 8;
    
    attribs[ai++] = NSOpenGLPFAStencilSize;
    attribs[ai++] = 8;
    
    attribs[ai++] = NSOpenGLPFADepthSize;
    attribs[ai++] = 16;
    
    if (api)
    {
        attribs[ai++] = NSOpenGLPFAOpenGLProfile;
        attribs[ai++] = api;
    }
    
    attribs[ai++] = 0;
    
    return [[NSOpenGLPixelFormat alloc] initWithAttributes: attribs];
}

class ContextGLNSGL: public ContextGL
{
public:
    ContextGLNSGL(void* windowHandle)
    : view(0)
    , pixelFormat(0)
    , context(0)
    , updateRequired(false)
    {
        view = [(NSView*)windowHandle retain];
        RBXASSERT(view);
        
        if (FFlag::GraphicsGL3)
        {
            // OSX 10.7 graphics drivers crash on compiling some (valid) shaders
            if (NSAppKitVersionNumber >= NSAppKitVersionNumber10_8)
                pixelFormat = createPixelFormatWithApi(NSOpenGLProfileVersion3_2Core);

            if (!pixelFormat)
                pixelFormat = createPixelFormatWithApi(0);

            if (!pixelFormat)
                throw std::runtime_error("Error creating pixel format");
        }
        else
        {
            pixelFormat = createPixelFormatWithApi(0);
            if (!pixelFormat)
                throw std::runtime_error("Error creating pixel format");
        }
        
        context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
        if (!context)
            throw std::runtime_error("Error creating context");
        
        GLint swapInterval = 0;
        [context setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];

        // Retina: drawable is in pixels. Without this + pixel-sized framebuffer dims,
        // the game only paints the bottom-left quadrant of the window on HiDPI Macs.
        if ([view respondsToSelector:@selector(setWantsBestResolutionOpenGLSurface:)])
            [view setWantsBestResolutionOpenGLSurface:YES];
        
        [context setView:view];
        [context makeCurrentContext];
        [context update];
        
        if (CGLError err = CGLEnable(static_cast<CGLContextObj>([context CGLContextObj]), kCGLCEMPEngine))
            FASTLOG1(FLog::Graphics, "Error enabling MP engine: %x", err);
        
        glewInitRBX();
    }

    ~ContextGLNSGL()
    {
        if ([NSOpenGLContext currentContext] == context)
            [NSOpenGLContext clearCurrentContext];
        
        [context clearDrawable];
        
        [context release];
        [pixelFormat release];
        [view release];
    }

    virtual void setCurrent()
    {
        if ([NSOpenGLContext currentContext] != context)
            [context makeCurrentContext];
    }

    virtual void swapBuffers()
    {
        [context flushBuffer];
    }

    virtual unsigned int getMainFramebufferId()
    {
        return 0;
    }

    virtual bool isMainFramebufferRetina()
    {
        // High-DPI: backing scale > 1 (typical Retina = 2). Used so UI/canvas stays in points.
        NSWindow* window = [view window];
        CGFloat scale = window ? [window backingScaleFactor] : [[NSScreen mainScreen] backingScaleFactor];
        return scale > 1.01;
    }

    virtual std::pair<unsigned int, unsigned int> updateMainFramebuffer(unsigned int width, unsigned int height)
    {
        // Convert view bounds (points) → backing pixels so glViewport matches the drawable.
        NSRect points = [view bounds];
        NSRect pixels = [view convertRectToBacking:points];
        unsigned int pw = (unsigned int)lround(pixels.size.width);
        unsigned int ph = (unsigned int)lround(pixels.size.height);
        if (pw == 0) pw = (unsigned int)lround(points.size.width);
        if (ph == 0) ph = (unsigned int)lround(points.size.height);
        
        if (FFlag::UpdateContextOnFollowingFrame && updateRequired)
        {
            [context update];
            updateRequired = false;
        }

        if (width != pw || height != ph)
        {
            if (!FFlag::UpdateContextOnFollowingFrame)
                [context update];
            updateRequired = true;
        }
        
        return std::make_pair(pw, ph);
    }

private:
    NSView* view;
    NSOpenGLPixelFormat* pixelFormat;
    NSOpenGLContext* context;
    
    bool updateRequired;
 };

ContextGL* ContextGL::create(void* windowHandle)
{
    return new ContextGLNSGL(windowHandle);
}

}
}
