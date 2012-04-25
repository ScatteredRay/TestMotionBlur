const char* strPixelShader = "Shaders/ParticleShader.ps";
const char* strVertexShader = "Shaders/BlurredShader.vs";
const char* strLightShader = "Shaders/LightShader.ps";
const char* strScreenAlignShader = "Shaders/ScreenAlignQuad.vs";
const char* strCombinerShader = "Shaders/Combiner.ps";
const char* strRockShader1 = "Shaders/ParticleShader.ps";
const char* strShadowPS = "Shaders/ShadowBufferPS.ps";
const char* strShadowVS = "Shaders/ShadowBufferVS.vs";
const char* strCubeMesh = "Assets/Cube.x";
const char* strMatLocal = "matLocal";
const char* strMatProjection = "matProjection";
const char* strMatView = "matView";
const char* strMatWorld = "matWorld";
const char* strMatTexture = "matTexture";
const char* strLightPosition = "fvLightPosition";
const char* strDeltaPos = "deltaPos";

const float CubeMeshDrawScale = 0.1f;
  const D3DCOLOR c_particle_color = 0xFF8888FF;
  const float c_particle_scale = 1.0f;
  const float c_particle_size = 20.0f;
  const int _initial_life = 200;

#include <NxVec3.h>

  const NxVec3 defQuad[4] = {NxVec3(-1, 1, 0), NxVec3(1, 1, 0), NxVec3(-1, -1, 0), NxVec3(1, -1, 0)};