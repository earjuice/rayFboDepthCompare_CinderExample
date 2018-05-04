
#define RAY_STEPS 64

uniform sampler2D fboColor;
uniform sampler2D fboDepth;

uniform float uAspectRatio, uViewDistance, zNear, zFar, mFov;
uniform vec3 uCameraPosition;
uniform mat3 uCameraOrientation;

in vec4 vertColor;
in vec2 fragCoord;
out vec4 fragColor;

// ray computation vars
const float PI = 3.14159265359;
//const float fov = 50.0; //uniform mFov

// epsilon-type values
const float S = 0.01;
const float EPSILON = 0.01;

// const delta vectors for normal calculation
const vec3 deltax = vec3(S ,0, 0);
const vec3 deltay = vec3(0 ,S, 0);
const vec3 deltaz = vec3(0 ,0, S);



// depthSample from depthTexture.r, for instance
float linearDepth(float depthSample)
{
    depthSample = 2.0 * depthSample - 1.0;
    float zLinear = 2.0 * zNear * zFar / (zFar + zNear - depthSample * (zFar - zNear));
    return zLinear;
}

// result suitable for assigning to gl_FragDepth
float depthSample(float linearDepth)
{
    float nonLinearDepth = (zFar + zNear - 2.0 * zNear * zFar / linearDepth) / (zFar - zNear);
    nonLinearDepth = (nonLinearDepth + 1.0) / 2.0;
    return nonLinearDepth;
}

float sdBox( vec3 p, vec3 b )
{
    vec3 d = abs(p) - b;
    return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}
float sdSphere( vec3 p, float s )
{
    return length(p)-s;
}
vec2 opU( vec2 d1, vec2 d2 )
{
    return (d1.x<d2.x) ? d1 : d2;
}


float distanceToNearestSurface(vec3 p){
    vec2 res = opU( vec2( sdBox( p +vec3( sin(iGlobalTime)*5. ,0.,0.),vec3(2.5)), 1.0 ),
                   vec2( sdSphere(    p-vec3( 20,0, 0.0), 5. ), 1. ) );
    return   res.x;//sdBox(p,vec3(10.));
}

// better normal implementation with half the sample points
// used in the blog post method
vec3 computeSurfaceNormal(vec3 p){
    float d = distanceToNearestSurface(p);
    return normalize(vec3(
                          distanceToNearestSurface(p+deltax)-d,
                          distanceToNearestSurface(p+deltay)-d,
                          distanceToNearestSurface(p+deltaz)-d
                          ));
}

vec3 computeLambert(vec3 p, vec3 n, vec3 l){
    return vec3(dot(normalize(l-p), n));
}

vec4 intersectWithWorld(vec3 p, vec3 dir){
    float dist = 0.0;
    float nearest = 0.0;
    vec4 result = vec4(0.0);
    for(int i = 0; i < RAY_STEPS; i++){
        nearest = distanceToNearestSurface(p + dir*dist);
        if(nearest < EPSILON){
            vec3 hit = p+dir*dist;
            vec3 light = vec3(100.0*sin(iGlobalTime), 30.0*cos(iGlobalTime), 50.0*cos(iGlobalTime));
            result = vec4(vec3(computeLambert(hit, computeSurfaceNormal(hit), light)),dist);
            break;
        }else{
            result=vec4(0.,0.,0.,1000.);
        }
        dist += nearest;
    }
    return result;
}

void main(void )
{
    vec2 uv = -1.0 + 2.0 * gl_FragCoord.xy / iResolution.xy; // screenPos can range from -1 to 1
    uv*=.5;
    uv.x *= uAspectRatio;
    
    highp vec3 rayOrigin = uCameraPosition;
    vec3 rayDirection = uCameraOrientation * normalize( vec3( uv, -uViewDistance ) );
    vec4 pixelColour= intersectWithWorld(rayOrigin, rayDirection);// raymarched scene

    vec4  fboColTex=texture( fboColor, fragCoord.xy  );//fbo Color
    float fboDepthTex=texture( fboDepth, fragCoord.xy  ).r;//fbo Depth
    
    fboDepthTex=linearDepth(fboDepthTex);//uncomment for linearDepth

    vec3 final=fboColTex.rgb;
    if(pixelColour.w<fboDepthTex )final= pixelColour.rgb;//Depth comparison
    
    fragColor = vec4(final, 1.0);
}
