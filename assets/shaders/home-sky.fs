#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform vec2 resolution;
uniform float drift;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float ridge(float x, float frequency) {
    float u = x * frequency;
    return mix(hash(vec2(floor(u), frequency)), hash(vec2(floor(u)+1., frequency)), fract(u));
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;
    vec2 p = (uv - .5) * vec2(resolution.x / resolution.y, 1.);
    vec3 sky = mix(vec3(.016,.032,.083), vec3(.13,.28,.40), exp(-5.*abs(p.y+.15)));
    // An oblique band of starlight behind the eclipse.
    float dust = exp(-9.*abs(p.y-.22*p.x-.08));
    sky += vec3(.036,.06,.12)*dust*(.3+ridge(p.x+p.y, 12.));
    // Fine vertical folds catch light above the distant peaks.
    float fold = .035*sin(7.*p.x+drift*.08)+.019*sin(14.*p.x+.6);
    float altitude = p.y+.13-fold;
    float veil = exp(-abs(altitude)*15.) * smoothstep(-.045,.04,altitude);
    float rays = .48+.30*sin(32.*p.x+3.*p.y)+.17*sin(73.*p.x-4.*p.y);
    sky += vec3(.016,.22,.23)*veil*rays;
    vec2 star = p * 135.;
    float seed = hash(floor(star));
    float spark = length(fract(star)-vec2(.15+.7*seed,.15+.7*hash(floor(star)+15.)));
    sky += vec3(.65,.84,1.)*(1.-smoothstep(.014,.14,spark))*smoothstep(.90,.99,seed);
    vec2 moon = p - vec2(-.49 + drift*.008,.225);
    float radius = length(moon);
    float limb = radius-.135;
    float sun = .23+.77*max(dot(moon/max(radius,.001),normalize(vec2(-1.,.6))),0.);
    sky += vec3(.20,.64,.94)*exp(-max(limb,0.)*65.)*sun;
    sky = mix(vec3(.024,.053,.107)+vec3(.05,.11,.18)*sun*exp(-abs(limb)*42.),sky,
              smoothstep(-.001,.001,limb));
    sky += vec3(.43,.77,1.)*sun*exp(-abs(limb)*580.);

    for (int i=0; i<3; ++i) {
        float layer=float(i);
        float x=p.x+layer*3.+drift*.005*layer;
        float h=-.22-layer*.048+.106*ridge(x,6.)+.043*ridge(x,18.)+.014*ridge(x,55.);
        vec3 rock=mix(vec3(.069,.153,.231),vec3(.016,.045,.089),layer*.48);
        rock+=vec3(.026,.052,.070)*exp(-max(h-p.y,0.)*42.);
        sky=mix(sky,rock,1.-smoothstep(h-.001,h+.001,p.y));
    }
    sky += (hash(gl_FragCoord.xy)-.5)/255.;
    finalColor=vec4(sky,1.);
}
