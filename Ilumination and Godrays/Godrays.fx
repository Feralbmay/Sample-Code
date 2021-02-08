float getShadowFactor_Godray(float3 rayPos, float3 rayDir, float rayHitTime, float4 iPosition) {

  float fogLitPercent = 0.0f;

  const float c_goldenRatioConjugate = 0.61803398875f;

  //move the raylight forward
  float4 projLightSpace;

  // Convert to homo space

  float2 center;
  float zref;


  // Extra samples
  uint nsamples = GodraySamples;
  float startRayOffset = 0.5f;
  float sample_factor = 64.f / GodraySamples;

  int t = (GlobalWorldTime * 10) % 64;

  //float4 noise_texture = txNoise.Sample(samLinear, iPosition.xy/1024.0f);
  //startRayOffset = frac(noise_texture.x + (float(t) * c_goldenRatioConjugate));

  float3 TestPos;

  
  //for (int j = 0; j < 1; ++j) {
    //int x = (t + (j));
    int x = t;
    startRayOffset = InterleavedGradientNoise(iPosition.xy, x);

    [loop]
    for (uint i = 0; i < nsamples; ++i) {
      //move the raylight forward
      TestPos = rayPos + rayDir * rayHitTime * ((float(i) + startRayOffset) / float(nsamples));
      projLightSpace = mul(float4(TestPos.xyz, 1), LightViewProjectionOffset);
      // Convert to homo space
      projLightSpace.xyz /= projLightSpace.w;

      center = projLightSpace.xy;
      zref = projLightSpace.z;
      if(txProjector.Sample(samLinearBorder, center).x){
        float shadow = tapShadow(center, zref);

        if (zref <= 0.1)
          shadow = 0;
        //fogLitPercent = lerp(fogLitPercent, tapShadow(center, zref), 1.0f / float(i + 1));
        fogLitPercent += shadow*sample_factor;


      }

    //}

  }



  return pow(abs(fogLitPercent), 1);
}

//Calculates the God ray of one light
float4 PS_Prepare_GodRays(float4 iPosition : SV_POSITION) : SV_Target
{
    int3 ss = uint3(iPosition.xy, 0);
    float4 acc_Godrays = txGodRays.Load(ss);

    GBuffer g;
  decodeGBuffer(iPosition.xy, g);

  float3 view_dir = mul(float4(iPosition.xy, 1, 1), CameraScreenToWorld).xyz;

  float3 ray_Dir = view_dir;


  float rayHitTime = length(g.wPos - CameraPos);
  rayHitTime = min(CameraZFar,rayHitTime);
  ray_Dir = normalize(ray_Dir);
  float c_fogDensity = GodRaysFogDensity/100;

  float3 test = CameraPos + ray_Dir * rayHitTime;

    float fogLitPercent = getShadowFactor_Godray(CameraPos, ray_Dir, rayHitTime,iPosition);

if (acc_Godrays.w != 0)
    acc_Godrays.xyz = (LightColor.xyz * fogLitPercent + acc_Godrays.xyz * acc_Godrays.w) / (fogLitPercent + acc_Godrays.w);
  else
    acc_Godrays.xyz = LightColor.xyz;

  acc_Godrays.w += fogLitPercent;


  return acc_Godrays;
}


//Applies the fog to the God Rays calculated on the PS_Prepare_GodRays function(this function accumulates the Volumetric ilumination of the lights, one by one, and merges them)
float4 PS_GodRays(float4 iPosition : SV_POSITION) : SV_Target
{
    int3 ss = uint3(iPosition.xy, 0);
    float4 acc_light = txAccLight.Load(ss);
    float4 acc_Godrays = txGodRays.Load(ss);

    int dist = GodraysNoiseReduction;

   


   acc_Godrays.w = acc_Godrays.w / ((dist * 2 + 1) * (dist * 2 + 1));




    GBuffer g;
  decodeGBuffer(iPosition.xy, g);

  float3 view_dir = mul(float4(iPosition.xy, 1, 1), CameraScreenToWorld).xyz;

  float3 ray_Dir = view_dir;

  float Disp = GodRaysFogDensity/100;


  float3 c_fogColorUnlit = lerp(acc_light.xyz, float3(1,1,1), GodRaysFogDisparity);
  float3 c_fogColorLit = acc_Godrays.xyz;
  float rayHitTime = length(g.wPos - CameraPos);
  rayHitTime = min(CameraZFar,rayHitTime);
  ray_Dir = normalize(ray_Dir);
  float c_fogDensity = GodRaysFogDensity/100;

  float3 test = CameraPos + ray_Dir * rayHitTime;

  float fogLitPercent = acc_Godrays.w;

  float3 fogColor = lerp(c_fogColorUnlit, GodRaysColorIntensity * c_fogColorLit, fogLitPercent);
  float absorb = exp(-pow(abs(rayHitTime / GodraysnearTolerance),GodraysDecay) * c_fogDensity);
  acc_light.xyz = lerp(fogColor, acc_light.xyz, absorb);


  return acc_light;
}