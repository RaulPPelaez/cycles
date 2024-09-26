# version 400 core
uniform sampler2D iChannel0;
uniform vec2 iResolution;

bool isWhite(vec4 color){
  return color.r == 1.0 && color.g == 1.0 && color.b == 1.0;
}

vec4 getColorAt(vec2 coord, float lod){
  vec4 color = texture(iChannel0, coord, lod);
  color.a = 1.0;
  //color.rgb /= 24.0;
  //If the pixel is white, return black
  if(isWhite(color)){
    return vec4(0.0, 0.0, 0.0, 1.0);
  }
  return color;
}
float rand(vec2 n) {
    return fract(sin(dot(n, vec2(12.9898,12.1414))) * 83758.5453);
}

float noise(vec2 n) {
    const vec2 d = vec2(0.0, 1.0);
    vec2 b = floor(n);
    vec2 f = smoothstep(vec2(0.0), vec2(1.0), fract(n));
    return mix(mix(rand(b), rand(b + d.yx), f.x), mix(rand(b + d.xy), rand(b + d.yy), f.x), f.y);
}

vec3 makeBloom(float lod, vec2 offset, vec2 bCoord){
  vec2 pixelSize = 1.0 / vec2(iResolution.x, iResolution.y);
  offset += pixelSize;
  float lodFactor = exp2(lod);
  vec3 bloom = vec3(0.0);
  vec2 scale = lodFactor * pixelSize;
  vec2 coord = (bCoord.xy-offset)*lodFactor;
  float totalWeight = 0.0;
  if (any(greaterThanEqual(abs(coord - 0.5), scale + 0.5)))
    return vec3(0.0);
  int range = 5;
  for (int i = -range; i < range; i++) {
    for (int j = -range; j < range; j++) {
      float wg = pow(1.0-length(vec2(i,j)) * 0.125,2.0);
      vec2 look_coord = vec2(i,j) * scale + lodFactor * pixelSize + coord;
      bloom += pow(getColorAt(look_coord, lod).rgb, vec3(2.2))*wg;
      totalWeight += wg;
    }
  }
  bloom /= totalWeight;
  return bloom;
}

// float blurIntensity(float distance){
//   float val = 0.01/(0.1+pow(distance,0.5));
//   return val>0.0?val:0.0;
// }
// vec4 applyBlur(vec2 texcoord){
//   vec4 sum = vec4(0);
//   sum.a = 1.0;
//   float blurSize = blurRadius/float(iResolution.x);
//   for(int i = -MAX_DISTANCE+1; i<MAX_DISTANCE; i++){
//     for(int j = -MAX_DISTANCE+1; j<MAX_DISTANCE; j++){
//       float shift_i = float(i)*blurSize;
//       float shift_j = float(j)*blurSize;
//       vec4 contribution = getColorAt(vec2(texcoord.x + shift_i, texcoord.y+shift_j));
//       float distance = sqrt(float(i*i + j*j))/float(MAX_DISTANCE);
//       sum += contribution*blurIntensity(distance);
//     }
//   }
//   return sum;
// }

void main(){
   vec4 sum = vec4(0);
   vec2 fragCoord = gl_FragCoord.xy;
   vec2 texcoord = fragCoord.xy/iResolution.xy;

   vec2 uv = gl_FragCoord.xy / iResolution.xy;
   vec3 blur = vec3(0);
   blur += makeBloom(2.,vec2(0.0,0.0), uv);
   //blur += makeBloom(3.,vec2(0.3,0.0), uv);
   //blur += makeBloom(4.,vec2(0.0,0.3), uv);
   // blur += makeBloom(5.,vec2(0.1,0.3), uv);
   //blur += makeBloom(6.,vec2(0.2,0.3), uv);

   gl_FragColor = vec4(pow(blur, vec3(1.0 / 2.2)),1.0);

   //Time is just 0
   // float iTime = 0.0;
   // vec4 original_color = texture2D(iChannel0, texcoord);
   // gl_FragColor = original_color;
   // vec4 blur = applyBlur(texcoord);
   // gl_FragColor = blur;// * intensity + original_color * (1.0-intensity);
}
