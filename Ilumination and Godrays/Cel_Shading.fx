
//--------------------------------------------------------------------------------------
float4 shade(float4 iPosition, const bool use_shadows)
{
  // Declare some float3 to store the values from the GBuffer
  GBuffer g;
  decodeGBuffer(iPosition.xy, g);

  int3 ss = uint3(iPosition.xy, 0);
  float4 acc_light = txAccLight.Load(ss);

  float shadow_factor;
  float4 projLightSpace = mul( float4( g.wPos.xyz, 1 ), LightViewProjectionOffset  );
  projLightSpace.xyz /= projLightSpace.w; 
  if(!txProjector.Sample(samLinearBorder, projLightSpace.xy).x)
    shadow_factor = 0;
  else
    shadow_factor = use_shadows
    ? getShadowFactor(g.wPos)
    : 1.0;

  float3 N = (g.N);
  float3 L = float3(1,0,0);
  // From wPos to Light
  float3 light_dir_full = LightPos.xyz - g.wPos;
  float  distance_to_light = length(light_dir_full);
  float3 light_dir = light_dir_full / distance_to_light;

  float  NdL = dot(N, light_dir);//saturate(0.5 * (1.0 + dot(N, light_dir)));
  float  NdV = 1;//saturate(dot(N, g.view_dir));
  float3 h = normalize(light_dir + g.view_dir); // half vector



  //No ilumination
  if (NdL * shadow_factor < CSRangeInferior) //0.5
    NdL = CSLow; //0

    //Diferenciates the 2 ranges and aplies a gradient between them
    float bias = 0.1;
   float v = smoothstep(CSRangeMiddle-bias,1, NdL * shadow_factor) * (0.9);
    NdL = v;

  float  NdH = saturate(dot(N, h));
  float  VdH = saturate(dot(g.view_dir, h));
  float  LdV = saturate(dot(light_dir, g.view_dir));

  // max is to avoid reach zero
  float  a = max(0.001f, g.roughness * g.roughness);
  float3 cDiff = Diffuse(g.albedo);
  float3 cSpec = Specular(g.specular_color, h, g.view_dir, light_dir, a, NdL, NdV, NdH, VdH, LdV);

  // Nota: Esta formula NO es correcta, pero hace q a LightMaxDistance, la aportacion de la
  // luz sea cero
  float att = 1.0 - saturate(distance_to_light / LightMaxDistance);
  // Mas real...
  //att = saturate( 1 / ( distance_to_light * distance_to_light ) ); 


float3 final_color = LightColor.xyz * LightIntensity * (NdL) * (cDiff * (1.0f - cSpec) + cSpec);

if(0.95*LightMaxDistance < distance_to_light)
  final_color = final_color * (att/0.05);
  

  return float4(final_color, NdL);
}
