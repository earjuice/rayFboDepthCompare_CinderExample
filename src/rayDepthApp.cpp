#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/CameraUi.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace gl;

class rayDepthApp : public App {
public:
    
    void setup() override;
    void mouseDown( MouseEvent event ) override;
    void mouseDrag( MouseEvent event ) override;
    void mouseUp( MouseEvent event ) override;
    void mouseWheel( MouseEvent event ) override;
    void update() override;
    void draw() override;
    
    void createPlane();
    void createCube();
    void createFbo(FboRef &sFbo,ivec2 size);
    void setUniforms( GlslProgRef prog);
    void drawShader();
    
    CameraPersp     mCam;
    CameraUi        uiCam;
    BatchRef		mPlane,mCube;
    GlslProgRef		mToyShader;
    FboRef          mFbo;
    
};

void rayDepthApp::setup()
{
    mCam.lookAt( vec3( 3, 2, 40 ), vec3( 0 ) );
    mCam.setFov(90.);
    uiCam.setCamera(&mCam);
    
    createPlane();
    createCube();
    createFbo(mFbo, getWindowSize());
    
    enableDepthWrite();
    enableDepthRead();
}

void rayDepthApp::mouseDown( MouseEvent event )
{
    uiCam.mouseDown(event);
}
void rayDepthApp::mouseDrag( MouseEvent event )
{
    uiCam.mouseDrag(event);
}
void rayDepthApp::mouseUp( MouseEvent event )
{
    uiCam.mouseUp(event);
}
void rayDepthApp::mouseWheel( MouseEvent event )
{
    uiCam.mouseWheel(event);
}

void rayDepthApp::update()
{
    ScopedFramebuffer sFbo(mFbo);
    ScopedViewport scpView(mFbo.get()->getSize());
    ScopedMatrices sMat;
    ScopedModelMatrix modelScope;
    setMatrices( mCam );
    //clear( ColorA( 0, 0, 0 ,0) );
    clear();
    enableDepth();

    color(1., 1., 1., 1.);
    mCube->draw();
    setUniforms(mToyShader);

}

void rayDepthApp::draw()
{
    clear( Color( 0, 0, 0 ) );

    ScopedGlslProg pGlsl(mToyShader);
    ScopedViewport sView(getWindowSize());
    ScopedColor sCol(vec4(1.f));
    ScopedMatrices scopedMatricesFract;
    enableDepthRead();
    enableDepthWrite();
    
    rotate(toRadians(90.), vec3(1.f,0.f,0.f));
    translate(vec3(getWindowWidth()/2,0,-getWindowHeight()/2));

    const ScopedTextureBind sTbind(mFbo->getColorTexture(),0);
    const ScopedTextureBind dTbind(mFbo->getTexture2d(GL_DEPTH_ATTACHMENT),1);
    mToyShader->uniform( "fboColor", 0);
    mToyShader->uniform( "fboDepth", 1);
      enableDepthRead();
     disableDepthWrite();
    mPlane->draw();

}

void rayDepthApp::createCube()
{
    auto glslDefShad = context()->getStockShader( ShaderDef().color() );
    VboMeshRef cube = VboMesh::create( geom::Cube().subdivisions(2).size(10.,10.,10.) );
    mCube =Batch::create( cube, glslDefShad);
}

void rayDepthApp::createPlane()
{
    VboMeshRef plane = VboMesh::create( geom::Plane().subdivisions(ivec2(8,4) ).size(getWindowSize()));
    GlslProg::Format fmtD;
    fmtD.vertex( loadAsset("Toy.vert") );;
    std::string fs = loadString( loadAsset( "shadertoy.inc" ) ) + loadString(loadAsset( "Toy.frag" ) );
    fmtD.fragment(fs);
    mToyShader = GlslProg::create( fmtD );
    mPlane = Batch::create( plane, mToyShader);
}
void rayDepthApp::createFbo(FboRef &sFbo,ivec2 size){
    //ivec2 size = getWindowSize()*ivec2(2);
    Texture2d::Format depthFormat;
    depthFormat.setInternalFormat( GL_DEPTH_COMPONENT32F );
    depthFormat.setCompareMode( GL_COMPARE_REF_TO_TEXTURE );
    depthFormat.setMagFilter( GL_LINEAR );
    depthFormat.setMinFilter( GL_LINEAR );
    depthFormat.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
    depthFormat.setCompareFunc( GL_LEQUAL );
    
    
    Texture::Format tfmt;
    tfmt.setMagFilter( GL_LINEAR );
    Fbo::Format fmt;
    fmt.setColorTextureFormat( tfmt );
    //fmt.enableDepthBuffer(true);
    fmt.depthTexture();
    //fmt.attachment( GL_DEPTH_ATTACHMENT, Texture2d::create( size.x,size.y , depthFormat ) );
    // fmt.setSamples( 8 );
    fmt.attachment( GL_COLOR_ATTACHMENT0, Texture2d::create( size.x,size.y ) );
    fmt.attachment( GL_COLOR_ATTACHMENT1, Texture2d::create( size.x,size.y ) );
    sFbo = Fbo::create( size.x, size.y, fmt );
}

void rayDepthApp::setUniforms( GlslProgRef prog)// mostly everything but textures
{
    auto shader = prog;//context()->getGlslProg();
    if( !shader )
        return;
    time_t now = time( 0 );
    tm *   t = gmtime( &now );
    vec4   iDate( float( t->tm_year + 1900 ), float( t->tm_mon + 1 ), float( t->tm_mday ), float( t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec ) );
    
    // Set shader uniforms.
    vec3  iResolution( vec2( getWindowSize() ), 1 );
    shader->uniform( "iResolution", iResolution );
    float iGlobalTime = (float)getElapsedSeconds();
    shader->uniform( "iGlobalTime", iGlobalTime );

    // Set camera Uniforms
    vec3 cameraPosition = mCam.getEyePoint() ;
    mat3 cameraOrientation =  glm::mat3(  mCam.getInverseViewMatrix()    ) ;
    float left, right, top, bottom, near, far;
    mCam.getFrustum( &left, &top, &right, &bottom, &near, &far );
    float viewDistance = mCam.getAspectRatio() / math< float >::abs( right - left ) * near;
    mToyShader->uniform( "uAspectRatio", mFbo->getAspectRatio() );
    mToyShader->uniform( "uCameraPosition", cameraPosition );
    mToyShader->uniform( "uCameraOrientation", cameraOrientation );
    mToyShader->uniform( "uViewDistance", viewDistance );
    mToyShader->uniform( "zNear", mCam.getNearClip());
    mToyShader->uniform( "zFar",  mCam.getFarClip());
    mToyShader->uniform( "mFov", mCam.getFov() );
    
}


CINDER_APP( rayDepthApp, RendererGl )
