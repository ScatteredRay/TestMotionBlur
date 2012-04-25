float4x4 matWorld;
float4x4 matProjection;

struct VS_INPUT 
{
   float4 Position : POSITION0;
   float4 Diffuse  : COLOR0;
   float3 Normal :   NORMAL0;
   
};

struct VS_OUTPUT 
{
   float4 Position : POSITION0;
   float Depth   : TEXCOORD0;
};

VS_OUTPUT vs_main( VS_INPUT Input )
{
   VS_OUTPUT Output;
   
   float4 WorldPos = mul(Input.Position, matWorld);
   Output.Position = mul(WorldPos, matProjection);
   Output.Depth = Output.Position.z;
   
   return( Output );
   
}