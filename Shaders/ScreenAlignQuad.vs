struct VS_OUTPUT
{
   float4 pos       : POSITION0;
   float2 texCoord  : TEXCOORD0;
};

VS_OUTPUT vs_main( float4 inPos: POSITION )
{
   VS_OUTPUT o = (VS_OUTPUT) 0;

   inPos.xy = sign( inPos.xy);
   o.pos = float4( inPos.xy, 0.0f, 1.0f);

   // get into range [0,1]
   o.texCoord = (float2(o.pos.x, -o.pos.y) + 1.0f)/2.0f;
   return o;
}
