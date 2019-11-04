#version 330

layout(location = 0) out vec4 color;

uniform sampler2D textureSampler;
uniform vec4 lightColor;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;

in vec3 vPosition;
in vec3 vNormal;
in vec4 vColor;
in vec4 vTCoord;

// Forward declarations
float cnoise(vec2 P);
float voronoi(vec2 x);
float voronoi_fbm(vec2 p, int octaves);
float noise(vec2 p);
float noise(vec3 p);

float PingPong(float t)
{
    float r = 2.0f*mod(t, 0.5f);
    float s = step(0.5f, mod(t, 1.0f));
    return s*r + (1.0f-s)*(1.0f-r);
}

vec2 TreeUVSafe(vec2 texUV)
{
    // cylinder radial coordinate must pingpong to ensure that 
    // the UV seam does not break from non-tiling noise.
    return vec2(texUV.x, PingPong(texUV.y)/2.0); 
}

float BarkRegions(vec2 texUV)
{
    float barkRegions = voronoi_fbm(2.0*texUV, 1);
    return 1.0-smoothstep(0.0, 0.5, barkRegions);
}

vec3 TreeBarkPattern(vec3 position, vec2 texUV, float tiling)
{
    float lengthFactor = 1.0;

    texUV = vec2(lengthFactor, 1.0) * texUV;
    vec2 treeUV = tiling*texUV;

    /*
        Layer 1 - base
    */
    float barkRegions = BarkRegions(treeUV);
    float barkRegionRings = 1.0-(1.0-step(0.1, barkRegions) + step(0.4, barkRegions));
    barkRegionRings = 1.0-barkRegionRings*cnoise(treeUV*30.0);
    
    float barkTopPattern = 1.0 - 0.5*voronoi_fbm(10.0*treeUV, 1);
    float barkUnevenness = cnoise(treeUV);
    float barkMask = barkRegions * barkTopPattern * barkRegionRings;

    vec3 baseColor = 1.5*vec3(0.35f, 0.3f, 0.2f);

    vec3 grooves = baseColor*barkTopPattern;
    vec3 surfaces = baseColor;
    vec3 color = mix(grooves, surfaces, barkMask);

    /* 
        Layer 2 streaks
    */
    treeUV /= 1.5;
    vec3 streakColor = 0.8*vec3(0.9f, 0.65f, 0.45f);
    float streaks = BarkRegions(treeUV) * smoothstep(0.0, 0.5, cnoise(treeUV*3.0)) ;
    streaks *= 0.75*(1.0-0.9*cnoise(treeUV*20.0));
    color = mix(color, streakColor, streaks);


    /* 
        Layer 3 highlights
    */
    treeUV *= 1.5;
    treeUV += vec2(0.5);
    vec3 highlightColor = vec3(0.8f, 0.7f, 0.5f);
    float barkHighlights = BarkRegions(treeUV) * smoothstep(0.0, 0.5, cnoise(treeUV*2.0)) ;
    barkHighlights *= 0.8*(1.0-0.9*cnoise(treeUV*20.0));
    color = mix(color, highlightColor, barkHighlights);

    /*
        Layer 4 moss
    */
    float noiseMask = noise(75.0*position);
    float variations = cnoise(treeUV*3.0);

    vec3 mossColor = vec3(0.3,0.4,0.2);
    mossColor = mix(mossColor*0.5, mossColor, noiseMask * (0.25 + 0.75*variations));
    color = mix(color, mossColor, variations);

    return vec3(color);
}

float Treegrooves(vec2 texUV, float tiling)
{
    float lengthFactor = 0.2;
    texUV = vec2(lengthFactor, 1.0) * texUV;
    vec2 treeUVs = tiling*TreeUVSafe(texUV);

    float barkRegions = BarkRegions(treeUVs);
    return barkRegions;
}

void main() 
{
    /*
        Color calculations
    */
    //vec3 diffuseColor = texture(textureSampler, vTCoord.gr).rgb;
    vec3 diffuseColor = TreeBarkPattern(vPosition, vTCoord.rg, 30.0);
    float grooves = Treegrooves(vTCoord.rg, 30.0);

    /*
        Light calculations
    */
    vec3 lightDir = normalize(lightPosition-vPosition);
    vec3 camDir = normalize(cameraPosition-vPosition);
    vec3 normal = normalize(vNormal);

    // Give ambient regions some depth
    float cameraContrib = clamp(dot(normal, camDir), 0.0, 1.0);
    vec3 ambientLight = cameraContrib * vec3(0.2);

    // Ordinary phong diffuse model with fake SSS
    float directLightDot = clamp(dot(normal, lightDir), 0.0, 1.0);
    float lightStrength = lightColor.a;
    vec3 diffuseLight = lightStrength * directLightDot * lightColor.rgb;

    // Specular not yet implemented
    vec3 specularLight = vec3(0.0);
    vec3 totalLightContribution = ambientLight + diffuseLight + specularLight;

    // Fake ambient occlusion
    totalLightContribution = mix(totalLightContribution*0.4, totalLightContribution, grooves);
    float branchShadow = (1.0-0.2*clamp(vTCoord.r/5.0, 0.0, 1.0));
    totalLightContribution = branchShadow * totalLightContribution;

    // Fake ambient ground bounced light
    vec3 bouncedLightColor = 0.4*vec3(0.3f, 0.5f, 1.0f);
    float bouncedLightDot = clamp(dot(normal, vec3(0.0,-1.0,0.0)), 0.0, 1.0);
    float bouncedLightHeight = 1.0-clamp(vPosition.y/4.0, 0.0, 1.0);
    vec3 bouncedLightContribution = bouncedLightColor * (bouncedLightDot + bouncedLightHeight);
    
    totalLightContribution = totalLightContribution + bouncedLightContribution;

    color = vec4(totalLightContribution, 1.0f) * vec4(diffuseColor, 1.0f);
}






/*
    Noise functions
    https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
*/

//	Classic Perlin 2D Noise 
//	by Stefan Gustavson
//
vec4 permute(vec4 x) {return mod(((x*34.0)+1.0)*x, 289.0);}
vec2 fade(vec2 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}

float cnoise(vec2 P) {
  vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
  vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
  Pi = mod(Pi, 289.0); // To avoid truncation effects in permutation
  vec4 ix = Pi.xzxz;
  vec4 iy = Pi.yyww;
  vec4 fx = Pf.xzxz;
  vec4 fy = Pf.yyww;
  vec4 i = permute(permute(ix) + iy);
  vec4 gx = 2.0 * fract(i * 0.0243902439) - 1.0; // 1/41 = 0.024...
  vec4 gy = abs(gx) - 0.5;
  vec4 tx = floor(gx + 0.5);
  gx = gx - tx;
  vec2 g00 = vec2(gx.x,gy.x);
  vec2 g10 = vec2(gx.y,gy.y);
  vec2 g01 = vec2(gx.z,gy.z);
  vec2 g11 = vec2(gx.w,gy.w);
  vec4 norm = 1.79284291400159 - 0.85373472095314 * 
    vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11));
  g00 *= norm.x;
  g01 *= norm.y;
  g10 *= norm.z;
  g11 *= norm.w;
  float n00 = dot(g00, vec2(fx.x, fy.x));
  float n10 = dot(g10, vec2(fx.y, fy.y));
  float n01 = dot(g01, vec2(fx.z, fy.z));
  float n11 = dot(g11, vec2(fx.w, fy.w));
  vec2 fade_xy = fade(Pf.xy);
  vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
  float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
  return 2.3 * n_xy;
}








//	Modified version of Voronoi noise by Pietro De Nicola
//  https://www.shadertoy.com/view/lsjGWD
float t = 0.0f;

float function 			= mod(t,4.0);
bool  multiply_by_F1	= mod(t,8.0)  >= 4.0;
bool  inverse			= mod(t,16.0) >= 8.0;
float distance_type	    = mod(t/16.0,4.0);

vec2 hash(vec2 p){
	p = vec2( dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3)));
	return fract(sin(p)*43758.5453);
}

float voronoi(vec2 x){
	vec2 n = floor( x );
	vec2 f = fract( x );
	
	float F1 = 8.0;
	float F2 = 8.0;
	
	for( int j=-1; j<=1; j++ )
		for( int i=-1; i<=1; i++ ){
			vec2 g = vec2(i,j);
			vec2 o = hash( n + g );

			o = 0.5 + 0.41*sin( 6.2831*o );	
			vec2 r = g - f + o;

		float d = 	distance_type < 1.0 ? dot(r,r)  :				// euclidean^2
				  	distance_type < 2.0 ? sqrt(dot(r,r)) :			// euclidean
					distance_type < 3.0 ? abs(r.x) + abs(r.y) :		// manhattan
					distance_type < 4.0 ? max(abs(r.x), abs(r.y)) :	// chebyshev
					0.0;

		if( d<F1 ) { 
			F2 = F1; 
			F1 = d; 
		} else if( d<F2 ) {
			F2 = d;
		}
    }
	
	float c = function < 1.0 ? F1 : 
			  function < 2.0 ? F2 : 
			  function < 3.0 ? F2-F1 :
			  function < 4.0 ? (F1+F2)/2.0 : 
			  0.0;
		
	if( multiply_by_F1 )	c *= F1;
	if( inverse )			c = 1.0 - c;
	
    return c;
}

float voronoi_fbm(vec2 p, int octaves){
	float s = 0.0;
	float m = 0.0;
	float a = 0.5;
	
	for( int i=0; i<octaves; i++ ){
		s += a * voronoi(p);
		m += a;
		a *= 0.5;
		p *= 2.0;
	}
	return s/m;
}



// Generic noise
float rand(vec2 n) { 
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float noise(vec2 p){
	vec2 ip = floor(p);
	vec2 u = fract(p);
	u = u*u*(3.0-2.0*u);
	
	float res = mix(
		mix(rand(ip),rand(ip+vec2(1.0,0.0)),u.x),
		mix(rand(ip+vec2(0.0,1.0)),rand(ip+vec2(1.0,1.0)),u.x),u.y);
	return res*res;
}

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}