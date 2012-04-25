#pragma once
#include "NXPhysics.h"
#include "D3D9.h"
#include "D3DX9.h"
#include "Resource.h"

#define _particle_list_length _max_particle_count*sizeof(SimParticle)
#define _particle_draw_list_length _max_particle_count*sizeof(DrawParticle)


const DWORD light_fvf={D3DFVF_XYZ|D3DFVF_PSIZE|D3DFVF_DIFFUSE};
const DWORD particle_fvf={D3DFVF_XYZ|D3DFVF_PSIZE|D3DFVF_DIFFUSE|D3DFVF_NORMAL};

struct SimParticle
{
	NxActor *Particle;
	int life_countdown;
};

struct DrawParticle
{
	NxVec3 location;
	float point_size;
	D3DCOLOR diffuse;
};

const DWORD screen_quad_fvf={D3DFVF_XYZ};

struct ScreenQuadPoint
{
	NxVec3 location;
};

struct Camera
{
	NxVec3 location;
	NxVec3 ViewDir;
};

class DXApp
{
private:
	UINT64 OldCount;
	UINT64 Frequency;
	DOUBLE DeltaTime;
	NxPhysicsSDK* _p_physdk;
	NxScene* _p_scene;
	IDirect3D9* _p_D3D;
	IDirect3DDevice9* _p_device;
	NxReal _time_step;
	FLOAT CameraAngleX;
	FLOAT CameraAngleY;
	int width;
	int height;
	int ShadowMapSize;
	bool bPaused;
	bool bTest;
	bool bMotionBlur;
	bool bBlurVel;
	Camera Cam;
	bool bMouseIsDown;
	int OldMX;
	int OldMY;
	float fFoward;
	float fStrafe;

	ID3DXFont* FPSFont;

	//<Particle members>
	LPD3DXMESH _particle_Mesh;
	SimParticle *_particle_list; //
	NxMaterialIndex _particle_material;
	DrawParticle *_particle_draw_list; //
	int _max_particle_count;
	int _particle_count;
	float _particle_size;
	float _particle_scale;
	D3DCOLOR _particle_color;
	LPD3DXCONSTANTTABLE _p_constant_able;
	LPD3DXCONSTANTTABLE _p_vertex_constant_able;
	LPD3DXCONSTANTTABLE _p_SB_constant_able;
	LPD3DXCONSTANTTABLE _p_SB_vertex_constant_able;
	D3DXHANDLE hMatLocal;
	D3DXHANDLE hMatProjection;
	D3DXHANDLE hMatView;
	D3DXHANDLE hMatTexture;
	D3DXHANDLE hMatWorld;
	D3DXHANDLE hLightPosition;
	D3DXHANDLE hDeltaPos;
	D3DXHANDLE hSBMatProjection;
	D3DXHANDLE hSBMatWorld;
	LPD3DXBUFFER _p_code;
	IDirect3DPixelShader9* _p_lightshader;
	IDirect3DPixelShader9* _p_shader;
	IDirect3DPixelShader9* _p_SB_shader;
	IDirect3DVertexShader9* _p_vshader;
	IDirect3DVertexShader9* _p_SB_vshader;
	IDirect3DTexture9* _point_texture;
	IDirect3DSurface9* _point_surface;
	IDirect3DSurface9* _old_blur_surface;
	//</Particle members>
	IDirect3DPixelShader9* _p_combinershader;
	IDirect3DVertexShader9* _p_alignshader;
	DrawParticle DrawLight;
	D3DXMATRIX trans1;
	IDirect3DSurface9* _back_buffer;
	IDirect3DTexture9* _rock_texture;
	IDirect3DTexture9* _blur_buffer;
	IDirect3DTexture9* _old_blur_buffer;
	IDirect3DTexture9* _screen_buffer;
	IDirect3DTexture9* ShadowBuffer;
	D3DXMATRIX matProjection;
	D3DXMATRIX matView;
public:
	bool loop_continue;
	DXApp(void);
	~DXApp(void);
private:
	void Frame();
	void RenderFrame();
	void DoParticles();
	void InitRock();
public:
	void HandleInput(WPARAM key, bool bDown);
	void HandleMouseBtn(bool bDown){}
	void HandleMouseMove(bool bMouseDown, int X, int Y);
	int run(HINSTANCE hInst, HWND hWnd);
};