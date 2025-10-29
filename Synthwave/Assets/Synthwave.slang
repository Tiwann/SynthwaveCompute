module Synthwave;

RWTexture2D<float4> texture;
struct Constants
{
  float time;
  float delta;
}

[[vk::push_constant]] Constants constants;

#define speed 5.0
#define disable_sound_texture_sampling

#ifndef disable_sound_texture_sampling
    #undef speed 
    // lower value of speed when using as audio visualizer
    #define speed 5.
#endif

//self-explainatory
#define audio_vibration_amplitude .125




#ifdef disable_sound_texture_sampling
#define textureMirror(a, b) float4(0)
#else
float4 textureMirror(sampler2D tex, float2 c){
    float2 cf = fract(c);
    return texture(tex,lerp(cf,1.-cf,mod(floor(c),2.)));
}
#endif


float amp(float2 p){
    return smoothstep(1.,8.,abs(p.x));   
}

float pow512(float a){
    a*=a;//^2
    a*=a;//^4
    a*=a;//^8
    a*=a;//^16
    a*=a;//^32
    a*=a;//^64
    a*=a;//^128
    a*=a;//^256
    return a*a;
}
float pow1d5(float a){
    return a*sqrt(a);
}
float hash21(float2 co){
    return fract(sin(dot(co.xy,float2(1.9898,7.233)))*45758.5433);
}
float hash(float2 uv){
    float a = amp(uv);
    #ifdef wave_thing
    float w = a>0.?(1.-.4*pow512(.51+.49*sin((.02*(uv.y+.5*uv.x)-jTime)*2.))):0.;
    #else
    float w=1.;
    #endif
    return (a>0.?
        a*pow1d5(
        //texture(iChannel0,uv/iChannelResolution[0].xy).r
        hash21(uv)
        )*w
        :0.)-(textureMirror(iChannel0,float2((uv.x*29.+uv.y)*.03125,1.)).x)*audio_vibration_amplitude;
}

float edgeMin(float dx,float2 da, float2 db,float2 uv){
    uv.x+=5.;
    float3 c = fract((round(float3(uv,uv.x+uv.y)))*(float3(0,1,2)+0.61803398875));
    float a1 = textureMirror(iChannel0,float2(c.y,0.)).x>.6?.15:1.;
    float a2 = textureMirror(iChannel0,float2(c.x,0.)).x>.6?.15:1.;
    float a3 = textureMirror(iChannel0,float2(c.z,0.)).x>.6?.15:1.;

    return min(min((1.-dx)*db.y*a3,da.x*a2),da.y*a1);
}

float2 trinoise(float2 uv){
    const float sq = sqrt(3./2.);
    uv.x *= sq;
    uv.y -= .5*uv.x;
    float2 d = fract(uv);
    uv -= d;

    bool c = dot(d,float2(1))>1.;

    float2 dd = 1.-d;
    float2 da = c?dd:d,db = c?d:dd;
    
    float nn = hash(uv+float(c));
    float n2 = hash(uv+float2(1,0));
    float n3 = hash(uv+float2(0,1));

    
    float nmid = lerp(n2,n3,d.y);
    float ns = lerp(nn,c?n2:n3,da.y);
    float dx = da.x/db.y;
    return float2(lerp(ns,nmid,dx),edgeMin(dx,da, db,uv+d));
}


float2 map(float3 p){
    float2 n = trinoise(p.xz);
    return float2(p.y-2.*n.x,n.y);
}

float3 grad(float3 p){
    const float2 e = float2(.005,0);
    float a =map(p).x;
    return float3(map(p+e.xyy).x-a
                ,map(p+e.yxy).x-a
                ,map(p+e.yyx).x-a)/e.x;
}

float2 intersect(float3 ro,float3 rd){
    float d =0.,h=0.;
    for(int i = 0;i<500;i++){ //look nice with 50 iterations
        float3 p = ro+d*rd;
        float2 s = map(p);
        h = s.x;
        d+= h*.5;
        if(abs(h)<.003*d)
            return float2(d,s.y);
        if(d>150.|| p.y>2.) break;
    }
    
    return float2(-1);
}


void addsun(float3 rd,float3 ld,inout float3 col){
    
	float sun = smoothstep(.21,.2,distance(rd,ld));
    
    if(sun>0.){
        float yd = (rd.y-ld.y);

        float a =sin(3.1*exp(-(yd)*14.)); 

        sun*=smoothstep(-.8,0.,a);

        col = lerp(col,float3(1.,.8,.4)*.75,sun);
    }
}


float starnoise(float3 rd){
    float c = 0.;
    float3 p = normalize(rd)*300.;
	for (float i=0.;i<4.;i++)
    {
        float3 q = fract(p)-.5;
        float3 id = floor(p);
        float c2 = smoothstep(.5,0.,length(q));
        c2 *= step(hash21(id.xz/id.y),.06-i*i*0.005);
        c += c2;
        p = mul(p*.6+.5*p, float3x3(3./5.,0,4./5.,0,1,0,-4./5.,0,3./5.));
    }
    c*=c;
    float g = dot(sin(rd*10.512),cos(rd.yzx*10.512));
    c*=smoothstep(-3.14,-.9,g)*.5+.5*smoothstep(-.3,1.,g);
    return c*c;
}

float3 gsky(float3 rd,float3 ld,bool mask){
    float haze = exp2(-5.*(abs(rd.y)-.2*dot(rd,ld)));
    

    //float st = mask?pow512(texture(iChannel0,(rd.xy+float2(300.1,100)*rd.z)*10.).r)*(1.-min(haze,1.)):0.;
    //float st = mask?pow512(hash21((rd.xy+float2(300.1,100)*rd.z)*10.))*(1.-min(haze,1.)):0.;
    float st = mask?(starnoise(rd))*(1.-min(haze,1.)):0.;
    float3 back = float3(.4,.1,.7)*(1.-.5*textureMirror(iChannel0,float2(.5+.05*rd.x/rd.y,0.)).x
    *exp2(-.1*abs(length(rd.xz)/rd.y))
    *max(sign(rd.y),0.));
    #ifdef city
    float x = round(rd.x*30.);
    float h = hash21(float2(x-166.));
    bool building = (h*h*.125*exp2(-x*x*x*x*.0025)>rd.y);
    if(mask && building)
        back*=0.,haze=.8, mask=mask && !building;
    #endif
    float3 col=clamp(lerp(back,float3(.7,.1,.4),haze)+st,0.,1.);
    if(mask)addsun(rd,ld,col);
    return col;  
}


float mod_glsl(float x, float y)
{
    return x - y * floor(x / y);
}

float4 mainImage(float2 fragCoord, float2 dimension)
{
    float4 fragColor = float4(0);
    #ifdef AA
    for(float x = 0.;x<1.;x+=1./float(AA)){
    for(float y = 0.;y<1.;y+=1./float(AA)){
    #else
        const float AA=1.,x=0.,y=0.;
    #endif
    float2 uv = (2.*(fragCoord+float2(x,y))-dimension.xy)/dimension.y;
    
    const float shutter_speed = .25; // for motion blur
	//float dt = fract(texture(iChannel0,float(AA)*(fragCoord+float2(x,y))/iChannelResolution[0].xy).r+iTime)*shutter_speed;
    float dt = fract(hash21(float(AA)*(fragCoord+float2(x,y)))+constants.time)*shutter_speed;
    float jTime = mod_glsl(constants.time-dt*constants.delta,4000.);
    float3 ro = float3(0.,1,(-20000.+jTime*speed));
    
        #ifdef stereo
            ro+=stereo*float3(.2*(float(uv.x>0.)-.5),0.,0.); 
            const float de = .9;
            uv.x=uv.x+.5*(uv.x>0.?-de:de);
            uv*=2.;
		#endif
        
    float3 rd = normalize(float3(uv,4./3.));//float3(uv,sqrt(1.-dot(uv,uv)));
    
    float2 i = intersect(ro,rd);
    float d = i.x;
    
    float3 ld = normalize(float3(0,.125+.05*sin(.1*jTime),1));

    float3 fog = d>0.?exp2(-d*float3(.14,.1,.28)):float3(0.);
    float3 sky = gsky(rd,ld,d<0.);
    
    float3 p = ro+d*rd;
    float3 n = normalize(grad(p));
    
    float diff = dot(n,ld)+.1*n.y;
    float3 col = float3(.1,.11,.18)*diff;
    
    float3 rfd = reflect(rd,n); 
    float3 rfcol = gsky(rfd,ld,true);
    
    col = lerp(col,rfcol,.05+.95*pow(max(1.+dot(rd,n),0.),5.));
    #ifdef VAPORWAVE
    col = lerp(col,float3(.4,.5,1.),smoothstep(.05,.0,i.y));
    col = lerp(sky,col,fog);
    col = sqrt(col);
    #else
    col = lerp(col,float3(.8,.1,.92),smoothstep(.05,.0,i.y));
    col = lerp(sky,col,fog);
    //no gamma for that old cg look
    #endif
    if(d<0.)
        d=1e6;
    d=min(d,10.);
    fragColor += float4(clamp(col,0.,1.),d<0.?0.:.1+exp2(-d));
     #ifdef AA
    }
    }
    fragColor/=float(AA*AA);
#endif
    return fragColor;
}


[shader("compute")]
[numthreads(16, 16, 1)]
void compute(uint3 threadId : SV_DispatchThreadID)
{
    uint2 screenPos = threadId.xy;
    uint texWidth = 0, texHeight = 0;
    texture.GetDimensions(texWidth, texHeight);
    if (screenPos.x >= texWidth || screenPos.y >= texHeight)
        return;

    float2 texSize = float2((float)texWidth, (float)texHeight);
    float2 normalizedCoord = float2((float)screenPos.x, (float)screenPos.y) / texSize;
    float4 color = mainImage(screenPos, texSize);
    texture[screenPos] = color;
}