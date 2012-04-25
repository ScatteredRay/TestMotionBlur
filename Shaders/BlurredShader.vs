float3 fvLightPosition;
float3 deltaPos;
float4x4 matLocal;
float4x4 matProjection;
float4x4 matView;
float4x4 matWorld;
float4x4 matTexture;

struct VS_INPUT 
{
   float4 Position : POSITION0;
   float4 Diffuse  : COLOR0;
   float3 Normal :   NORMAL0;
   
};

struct VS_OUTPUT 
{
   float4 Position : POSITION0;
   float4 Diffuse  : COLOR0;
   float2 Texcoord : TEXCOORD0;
   float3 Light      : TEXCOORD1;
   float3 Normal :   TEXCOORD2;
   float4 NewPosition :   TEXCOORD3;
   float4 Location : TEXCOORD4;
   float4 ShadowMapCoord : TEXCOORD5;
};

VS_OUTPUT vs_main( VS_INPUT Input )
{
   VS_OUTPUT Output;
   
   float4 WorldPos = mul(Input.Position, matWorld);
   float4 CameraPos = mul(WorldPos, matView);
   Output.Position = mul(CameraPos, matProjection);
   Output.ShadowMapCoord = mul(WorldPos, matTexture);
   
   Output.Diffuse = Input.Diffuse;
   Output.Texcoord = float2(0, 0);
   Output.Light = normalize(fvLightPosition - WorldPos);
   Output.Normal = normalize(mul(Input.Normal, matWorld));
   
   float3 NewPos = deltaPos + WorldPos;
   //float4 NewWorldPos = mul(NewPos, matWorld);
   float4 NewCameraPos = mul(NewPos, matView);
   Output.NewPosition = mul(NewCameraPos, matProjection); 
   Output.Location = Output.Position;
   //Output.NewPosition = Output.Position;

   return( Output );
   
}