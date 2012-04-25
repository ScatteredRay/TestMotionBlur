#define D3D_DEBUG_INFO
#include "StdAfx.h"
#include ".\dxapp.h"
#include "stdlib.h"
#include "resource.h"
#include "Content.h"

#include <cstdio>


DXApp::DXApp(void)
{
	_p_physdk = NxCreatePhysicsSDK(NX_PHYSICS_SDK_VERSION, NULL, NULL);
	_p_D3D = Direct3DCreate9(D3D_SDK_VERSION);
	_max_particle_count = 400; //TODO: undo hardcoded
	width = 1024;
	height = 768;
	fFoward = 0;
	fStrafe = 0;
	Cam.location = NxVec3(-1000.0f, 500.0f,0.0f);
	Cam.ViewDir = NxVec3(0.7f, -0.3f, 0.0f);
	ShadowMapSize = 1024;
	bPaused = false;
	bTest = false;
	_screen_buffer = NULL;
	_blur_buffer = NULL;
	_particle_Mesh = NULL;
	_particle_count = 0;
	bBlurVel = true;
	_particle_color = c_particle_color;
	_particle_scale = c_particle_scale;
	_particle_size = c_particle_size;
	_particle_list = new SimParticle[_max_particle_count];//(NxActor*[])malloc(_particle_list_length);
	_particle_draw_list = new DrawParticle[_max_particle_count];//(DrawParticle[])malloc(_particle_draw_list_length);
	memset(_particle_draw_list, 0, _particle_draw_list_length);
	memset(_particle_list, 0, _particle_list_length);
}

DXApp::~DXApp(void)
{
	delete _particle_list;
	delete _particle_draw_list;
	_particle_draw_list = NULL;
	_particle_list = NULL;
	_max_particle_count = 0;
	_particle_count = 0;
	if(_screen_buffer != NULL)
		_screen_buffer->Release();
	if(_blur_buffer != NULL)
		_blur_buffer->Release();
	if(_old_blur_buffer != NULL)
		_old_blur_buffer->Release();
	if(_particle_Mesh)
		_particle_Mesh->Release();
	if(FPSFont != NULL)
	{
		FPSFont->Release();
	}
	if(_p_D3D != NULL)
	{
		_p_D3D->Release();
		_p_D3D = NULL;
	}
	if(_p_scene != NULL)
	{
		_p_physdk->releaseScene(*_p_scene); //redundant as the sdk release will get rid of this for us
		_p_scene = NULL;
	}

	if(_p_physdk != NULL)
	{
		_p_physdk->release();
		_p_physdk = NULL;
	}
}

int DXApp::run(HINSTANCE hInst, HWND hWnd)
{
	MSG msg;
	HACCEL hAccelTable;
	hAccelTable = LoadAccelerators(hInst, (LPCTSTR)IDC_TESTDXPROJ);
	QueryPerformanceFrequency((LARGE_INTEGER*)&Frequency);


	if(!_p_physdk) return -1;

	NxSceneDesc sceneDesc;
	sceneDesc.gravity.set(0, -9800.f, 0);
	sceneDesc.collisionDetection = true;
	sceneDesc.groundPlane = true;
	_p_scene = _p_physdk->createScene(sceneDesc);
	if(!_p_scene) return -2;

	_time_step = 1.0f/400.0f;
	_p_scene->setTiming(_time_step);

	NxMaterialDesc ParticleMaterialDesc;
	ParticleMaterialDesc.restitution = 0.80f;
	//ParticleMaterialDesc.spinFriction = 0.2f;
	ParticleMaterialDesc.staticFriction = 0.8f;
	ParticleMaterialDesc.dynamicFriction = 0.5f;
	NxMaterial* ParticleMaterial = _p_scene->createMaterial(ParticleMaterialDesc);
	_particle_material = ParticleMaterial->getMaterialIndex();


	D3DPRESENT_PARAMETERS D3DParams;
	D3DParams.Windowed = true;
	D3DParams.BackBufferWidth = width;
	D3DParams.BackBufferHeight = height;
	D3DParams.MultiSampleType = D3DMULTISAMPLE_NONE;
	D3DParams.MultiSampleQuality = 0;
	D3DParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	D3DParams.Flags = 0;
	D3DParams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	D3DParams.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	D3DParams.BackBufferFormat = D3DFMT_UNKNOWN;
	D3DParams.BackBufferCount = 3;
	D3DParams.hDeviceWindow = hWnd;
	D3DParams.EnableAutoDepthStencil = true;
	D3DParams.AutoDepthStencilFormat = D3DFMT_D16;

	if(FAILED(_p_D3D->CreateDevice( D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&D3DParams,
		&_p_device)))
	{
		return -3;
	}

	D3DXCreateFont(_p_device, 12, 0, FW_NORMAL, 1, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "Arial", &FPSFont);

	_p_device->SetFVF(particle_fvf);
	if(FAILED(D3DXCompileShaderFromFile(
		strPixelShader,
		NULL,
		NULL,
		"ps_main",
		"ps_2_0",  
		0, 
		&_p_code, 
		NULL,
		&_p_constant_able )))
	{
		MessageBox(hWnd, "Error compiling Script", "Pixel Shader Compile Error", MB_ICONERROR);
		return 1;
	}

	_p_device->CreatePixelShader((DWORD*)_p_code->GetBufferPointer(), &_p_shader);
	_p_code->Release();
	_p_code = NULL;

	//SHADOW Buffer Pixel Shaders
	if(FAILED(D3DXCompileShaderFromFile(
		strShadowPS,
		NULL,
		NULL,
		"ps_main",
		"ps_2_0",  
		0, 
		&_p_code, 
		NULL,
		&_p_SB_constant_able )))
	{
		MessageBox(hWnd, "Error compiling Script", "Pixel Shader Compile Error", MB_ICONERROR);
		return 1;
	}

	_p_device->CreatePixelShader((DWORD*)_p_code->GetBufferPointer(), &_p_SB_shader);
	_p_code->Release();
	_p_code = NULL;

	//SHADOW Buffer Vertex Shaders
	if(FAILED(D3DXCompileShaderFromFile(
		strShadowVS,
		NULL,
		NULL,
		"vs_main",
		"vs_2_0",  
		0, 
		&_p_code, 
		NULL,
		&_p_SB_vertex_constant_able )))
	{
		MessageBox(hWnd, "Error compiling Script", "Vertex Shader Compile Error", MB_ICONERROR);
		return 1;
	}

	_p_device->CreateVertexShader((DWORD*)_p_code->GetBufferPointer(), &_p_SB_vshader);
	_p_code->Release();
	_p_code = NULL;

	if(FAILED(D3DXCompileShaderFromFile(
		strCombinerShader,
		NULL,
		NULL,
		"ps_main",
		"ps_2_b",  
		0, 
		&_p_code, 
		NULL,
		&_p_constant_able )))
	{
		MessageBox(hWnd, "Error compiling Script", "Pixel Shader Compile Error", MB_ICONERROR);
		return 1;
	}

	_p_device->CreatePixelShader((DWORD*)_p_code->GetBufferPointer(), &_p_combinershader);
	_p_code->Release();
	_p_code = NULL;

	if(FAILED(D3DXCompileShaderFromFile(
		strScreenAlignShader,
		NULL,
		NULL,
		"vs_main",
		"vs_2_0",  
		0, 
		&_p_code, 
		NULL,
		&_p_constant_able )))
	{
		MessageBox(hWnd, "Error compiling Script", "Vertex Shader Compile Error", MB_ICONERROR);
		return 1;
	}

	_p_device->CreateVertexShader((DWORD*)_p_code->GetBufferPointer(), &_p_alignshader);
	_p_code->Release();
	_p_code = NULL;

	_p_device->SetFVF(light_fvf);
	if(FAILED(D3DXCompileShaderFromFile(
		strLightShader,
		NULL,
		NULL,
		"ps_main",
		"ps_2_0",  
		0, 
		&_p_code, 
		NULL,
		&_p_constant_able )))
	{
		MessageBox(hWnd, "Error compiling Script", "Pixel Shader Compile Error", MB_ICONERROR);
		return 1;
	}

	_p_device->CreatePixelShader((DWORD*)_p_code->GetBufferPointer(), &_p_lightshader);
	_p_code->Release();
	_p_code = NULL;

	_p_device->SetFVF(particle_fvf);
	if(FAILED(D3DXCompileShaderFromFile(
		strVertexShader,
		NULL,
		NULL,
		"vs_main",
		"vs_2_0",  
		0, 
		&_p_code, 
		NULL,
		&_p_vertex_constant_able )))
	{
		MessageBox(hWnd, "Error compiling Script", "Vertex Shader Compile Error", MB_ICONERROR);
		return 1;
	}

	_p_device->CreateVertexShader((DWORD*)_p_code->GetBufferPointer(), &_p_vshader);
	_p_code->Release();
	_p_code = NULL;

	hMatProjection = _p_vertex_constant_able->GetConstantByName(NULL, strMatProjection);
	hMatView = _p_vertex_constant_able->GetConstantByName(NULL, strMatView);
	hMatTexture = _p_vertex_constant_able->GetConstantByName(NULL, strMatTexture);
	hMatWorld = _p_vertex_constant_able->GetConstantByName(NULL, strMatWorld);
	hLightPosition = _p_vertex_constant_able->GetConstantByName(NULL, strLightPosition);
	hDeltaPos = _p_vertex_constant_able->GetConstantByName(NULL, strDeltaPos);
	hSBMatProjection = _p_SB_vertex_constant_able->GetConstantByName(NULL, strMatProjection);
	hSBMatWorld = _p_SB_vertex_constant_able->GetConstantByName(NULL, strMatWorld);

	//Setup our particle mesh.
	{	
		LPD3DXMESH _tmp_mesh;
		if(FAILED(D3DXLoadMeshFromX(strCubeMesh, D3DXMESH_VB_MANAGED, _p_device, NULL, NULL, NULL, NULL, &_tmp_mesh)))
		{
			MessageBox(hWnd, "Error Loading Cube Model", "Mesh Load Error", MB_ICONERROR);
			return 1;
		}
		_tmp_mesh->CloneMeshFVF(D3DXMESH_VB_MANAGED, particle_fvf, _p_device, &_particle_Mesh);
		_tmp_mesh->Release();
	}

	// We now have _p_physdk, _p_scene, _p_D3D, _p_device initialized

	// Setup the Camera
	CameraAngleX = (D3DX_PI/4)*3;
	CameraAngleY = 0;

	// Light Setup
	DrawLight.location = NxVec3(500, 500, 50);
	DrawLight.point_size = _particle_size;
	DrawLight.diffuse = (((_particle_color & 0xFF000000)>>24))<<24
		| (((_particle_color & 0x00FF0000)>>16))<<16
		| (((_particle_color & 0x0000FF00)>>8))<<8
		| ((_particle_color & 0x000000FF));

	D3DCAPS9 DevCaps;
	_p_device->GetDeviceCaps(&DevCaps);
	if(FAILED(_p_device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &_screen_buffer, NULL)))
	{
		MessageBox(hWnd, "Error Creating Texture", "Texture Error", MB_ICONERROR);
		return 1;
	}

	if(DevCaps.NumSimultaneousRTs > 1)
	{	
		bMotionBlur = true;
		if(FAILED(_p_device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &_blur_buffer, NULL)))
		{
			MessageBox(hWnd, "Error Creating BLUR Texture", "Texture Error", MB_ICONERROR);
			return 1;
		}
		if(FAILED(_p_device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &_old_blur_buffer, NULL)))
		{
			MessageBox(hWnd, "Error Creating BLUR Texture", "Texture Error", MB_ICONERROR);
			return 1;
		}
		//_p_device->CreateRenderTarget(width, height, D3DFMT_D32, D3DMULTISAMPLE_NONE , 0, false, &_blur_buffer, NULL);
	}
	else
		bMotionBlur = false;


	if(FAILED(_p_device->CreateTexture(ShadowMapSize, ShadowMapSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &ShadowBuffer, NULL)))
	{
		MessageBox(hWnd, "Error Creating Shadow Buffer", "Texture Error", MB_ICONERROR);
		return 1;
	}


	while(loop_continue)
	{
		Frame();
		if(PeekMessage(&msg, NULL, 0, 0, 1)) 
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	return (int) msg.wParam;
}

void DXApp::Frame()
{
	UINT64 CurrentTime;
	UINT64 DeltaCount;
	QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTime);
	DeltaCount = CurrentTime - OldCount;
	OldCount = CurrentTime;

	DeltaTime = (double)DeltaCount/(double)Frequency;

	{
		NxVec3 Temp;
		Cam.location+=Cam.ViewDir*40*fFoward;

		Temp = Cam.ViewDir;
		Temp.y = 0;
		Temp.normalize();
		Temp = Temp.cross(NxVec3(0.0f, 1.0f, 0.0f));
		Cam.location+=Temp*40*fStrafe;;
	}

	if(!bPaused)
	{
		DoParticles();
		_p_scene->simulate(_time_step);
		_p_scene->flushStream();
	}
	RenderFrame();
	if(!bPaused)
		_p_scene->fetchResults(NX_RIGID_BODY_FINISHED, true);

}


void DXApp::RenderFrame()
{
	_p_device->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
	_p_device->BeginScene();

	_p_device->SetRenderState(D3DRS_AMBIENT,RGB(255,255,255));
	_p_device->SetRenderState(D3DRS_LIGHTING,false);
	_p_device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	_p_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	_p_device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	D3DXMatrixPerspectiveFovLH(&matProjection,0.30f,(float)width/height,200.0f,2000.0f);
	//D3DXMatrixRotationX(&trans1, CameraAngleX);
	D3DXMatrixRotationYawPitchRoll(&trans1, 0.0f, CameraAngleX, CameraAngleY);
	D3DXMatrixIdentity(&matView);
	D3DXMatrixLookAtLH( &matView, (D3DXVECTOR3*)&Cam.location,
		(D3DXVECTOR3*)&(Cam.location + Cam.ViewDir),
		&D3DXVECTOR3( 0.0f, 1.0f, 0.0f ) );
	//D3DXMatrixMultiply(&matView, &trans1, &matView);
	D3DXMATRIX matLocal;
	D3DXMatrixIdentity(&matLocal);
	_p_device->GetRenderTarget(0, & _back_buffer);

	///////////////////////////////////////////////////
	//Render Shadow Buffer
		IDirect3DSurface9* _shadow_buffer;
		ShadowBuffer->GetSurfaceLevel(0, &_shadow_buffer);
		_p_device->SetRenderTarget(0, _shadow_buffer);
		_p_device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0);

		D3DXMATRIX matSBView;
		D3DXMATRIX matSBProjection;
		D3DXMATRIX matSBFull;

		D3DXMatrixLookAtLH( &matSBView, (D3DXVECTOR3*)&DrawLight.location, &D3DXVECTOR3( 0.0f, 0.0f, 0.0f ), &D3DXVECTOR3( 0.0f, 1.0f, 0.0f ) );
		D3DXMatrixPerspectiveFovLH( &matSBProjection, D3DXToRadian(30.0f), 1,200.0f,2000.0f);
		D3DXMatrixMultiply( &matSBFull, &matSBView, &matSBProjection );

		_p_SB_vertex_constant_able->SetMatrix(_p_device, hSBMatProjection, &matSBFull);

		_p_device->SetFVF(particle_fvf);
		_p_device->SetPixelShader(_p_SB_shader);
		_p_device->SetVertexShader(_p_SB_vshader);

		//now render the scene
		for(int i=0; i < _max_particle_count; i++)
		{
			if(_particle_list[i].Particle != NULL)
			{
				NxMat34 Pose = _particle_list[i].Particle->getGlobalPose();

				float MatTransform[16];
				Pose.getColumnMajor44((NxF32*)&MatTransform);
				D3DXMATRIX Transfom(MatTransform);
				D3DXMatrixScaling(&matLocal, CubeMeshDrawScale, CubeMeshDrawScale, CubeMeshDrawScale);
				D3DXMatrixMultiply(&Transfom, &matLocal, &Transfom);
				_p_SB_vertex_constant_able->SetMatrix(_p_device, hSBMatWorld, &Transfom);
				_particle_Mesh->DrawSubset(0);
			}
		}

		_shadow_buffer->Release();








	/////////////////////////////////////////////////
	//Render Scene
	//_p_device->SetTexture(0, _point_texture);
	IDirect3DSurface9* screen_surface;
	_screen_buffer->GetSurfaceLevel(0, &screen_surface);
	_p_device->SetRenderTarget(0, screen_surface);
	_p_device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF000000, 1.0f, 0);
	_p_device->SetRenderTarget(0, _back_buffer);
	_p_device->SetTexture(0, ShadowBuffer);

	IDirect3DSurface9* blur_surface;
	if(bMotionBlur)
	{
		_blur_buffer->GetSurfaceLevel(0, &blur_surface);
		_p_device->SetRenderTarget(1, blur_surface);
		_p_device->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
	}
	_p_device->SetRenderTarget(0, screen_surface);
	//Setup the Transform Matricies for the VertexShader;
	_p_vertex_constant_able->SetMatrix(_p_device, hMatProjection, &matProjection);
	_p_vertex_constant_able->SetMatrix(_p_device, hMatView, &matView);
	_p_vertex_constant_able->SetFloatArray(_p_device, hLightPosition, (float*)&DrawLight.location, 3);

	float fTexOffs = 0.5f + (0.5f / (float)ShadowMapSize);
	D3DXMATRIX matTexAdj( 0.5f,     0.0f,     0.0f, 0.0f,
						  0.0f,     -0.5f,    0.0f, 0.0f,
						  0.0f,     0.0f,     1.0f, 0.0f,
						  fTexOffs, fTexOffs, 0.0f, 1.0f );

	D3DXMATRIX matTexture;// = matView;

	//D3DXMatrixInverse( &matTexture, NULL, &matTexture );
	D3DXMatrixIdentity(&matTexture);
	D3DXMatrixMultiply( &matTexture, &matTexture, &matSBFull );
	D3DXMatrixMultiply( &matTexture, &matSBFull, &matTexAdj );

	_p_vertex_constant_able->SetMatrix(_p_device, hMatTexture, &matTexture);

	_p_device->SetFVF(particle_fvf);
	_p_device->SetPixelShader(_p_shader);
	_p_device->SetVertexShader(_p_vshader);

	{
		D3DXMATRIX Transform;

		D3DXMatrixTranslation(&Transform, DrawLight.location.x, DrawLight.location.y, DrawLight.location.z);
		_p_vertex_constant_able->SetFloatArray(_p_device, hDeltaPos, (float*)&NxVec3(0,0,0), 3);
		_p_vertex_constant_able->SetMatrix(_p_device, hMatWorld, &Transform);
		_particle_Mesh->DrawSubset(0);

		D3DXMatrixIdentity(&Transform);
		D3DXMatrixScaling(&Transform, 200.0f, 200.0f, 200.0f);
		D3DXMatrixRotationAxis(&matLocal, &D3DXVECTOR3(1.0f,0.0f,0.0f), D3DXToRadian(90));
		D3DXMatrixMultiply(&Transform, &matLocal, &Transform);
		_p_vertex_constant_able->SetMatrix(_p_device, hMatWorld, &Transform);
		_p_vertex_constant_able->SetFloatArray(_p_device, hDeltaPos, (float*)&NxVec3(0,0,0), 3);
		
		
		_p_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, defQuad, sizeof(NxVec3));
	}
	_p_device->SetFVF(particle_fvf);
	//now render the scene
	for(int i=0; i < _max_particle_count; i++)
	{
		if(_particle_list[i].Particle != NULL)
		{
			//NxVec3 Translation = _particle_list[i].Particle->getGlobalPosition();
			NxVec3 Velocity;
			if(bBlurVel)
				Velocity = _particle_list[i].Particle->getLinearVelocity();
			else
				Velocity = NxVec3(0,0,0);
			NxMat34 Pose = _particle_list[i].Particle->getGlobalPose();
			
			//Velocity *= _time_step;

			float MatTransform[16];
			Pose.getColumnMajor44((NxF32*)&MatTransform);
			D3DXMATRIX Transfom(MatTransform);
			D3DXMatrixScaling(&matLocal, CubeMeshDrawScale, CubeMeshDrawScale, CubeMeshDrawScale);
			D3DXMatrixMultiply(&Transfom, &matLocal, &Transfom);
			_p_vertex_constant_able->SetMatrix(_p_device, hMatWorld, &Transfom);
			_p_vertex_constant_able->SetFloatArray(_p_device, hDeltaPos, (float*)&Velocity, 3);
			_particle_Mesh->DrawSubset(0);
		}
	}
	_p_device->SetTexture(0, NULL);

	D3DXMatrixIdentity(&matLocal);
	//_p_vertex_constant_able->SetMatrix(_p_device, hMatLocal, &matLocal);
	_p_vertex_constant_able->SetMatrix(_p_device, hMatWorld, &matLocal);
	_p_device->SetRenderState(D3DRS_POINTSPRITEENABLE, true);
	_p_device->SetRenderState(D3DRS_POINTSCALEENABLE,true);
	_p_device->SetRenderState(D3DRS_POINTSIZE,*((DWORD*)&_particle_size));
	_p_device->SetRenderState(D3DRS_POINTSCALE_B,*((DWORD*)&_particle_scale));
	_p_device->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
	_p_device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
	_p_device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
	_p_device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	_p_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

	_p_device->SetFVF(light_fvf);
	_p_device->SetPixelShader(_p_lightshader);
	_p_device->DrawPrimitiveUP(D3DPT_POINTLIST, 1, &DrawLight, sizeof(DrawParticle));
	
	_p_device->SetRenderState(D3DRS_POINTSPRITEENABLE, false);
	_p_device->SetRenderState(D3DRS_POINTSCALEENABLE, false);
	_p_device->SetRenderState(D3DRS_POINTSIZE, 0);
	_p_device->SetRenderState(D3DRS_POINTSCALE_B, 0);
	_p_device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	_p_device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
	_p_device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ZERO);

	D3DXMatrixIdentity(&matProjection);
	_p_device->SetRenderTarget(0, _back_buffer);
	_p_device->SetRenderTarget(1, NULL);
	if(blur_surface)
	{
		blur_surface->Release();
		IDirect3DTexture9* tmp;
		tmp=_blur_buffer;
		_blur_buffer=_old_blur_buffer;
		_old_blur_buffer=tmp;
	}
	screen_surface->Release();

	_p_device->SetPixelShader(_p_combinershader);
	_p_device->SetVertexShader(_p_alignshader);
	_p_device->SetTexture(0, _screen_buffer);
	_p_device->SetTexture(1, _blur_buffer);
	_p_device->SetTexture(2, _old_blur_buffer);



	_p_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, defQuad, sizeof(NxVec3));
	
	_p_device->SetTexture(0, NULL);
	_p_device->SetTexture(1, NULL);

	//Render the FPS Counter
	RECT Rect;
	SetRect(&Rect, 10, 10, width, height);
	CHAR buffer[128];
	sprintf(buffer, "%d Instant FPS", (int)(1/DeltaTime));
	FPSFont->DrawText(NULL, buffer, -1, &Rect, DT_LEFT|DT_NOCLIP, 0xFFFFFFFF);

	_p_device->EndScene();
	_p_device->Present(NULL, NULL, NULL, NULL);
}

void DXApp::DoParticles()
{
	for(int i=0; i<_max_particle_count; i++)
	{
		if(_particle_list[i].Particle != NULL)
		{
			_particle_list[i].life_countdown--;
			if(_particle_list[i].life_countdown == 0)
			{
				_p_scene->releaseActor(*_particle_list[i].Particle);
				_particle_list[i].Particle = NULL;
				_particle_count--;
			}
		}
	}
	if(_particle_count < _max_particle_count) //Removed so that we can cleanup sleeping particles.
	{
		for(int i=0; i<_max_particle_count; i++)
		{
			if(_particle_list[i].Particle == NULL)
			{
				NxActorDesc ActorDesc;
				NxBodyDesc BodyDesc;
				ActorDesc.body = &BodyDesc;
				BodyDesc.mass = 1;
				BodyDesc.massSpaceInertia.set(1.0f);
				BodyDesc.angularDamping = 0.2f;
				ActorDesc.density = 0;
				_particle_list[i].Particle = _p_scene->createActor(ActorDesc);
				if(_particle_list[i].Particle != NULL)
				{
					_particle_list[i].life_countdown = _initial_life;
					_particle_count++;
					NxVec3 Force;
					Force.x = ((float)rand()/(float)RAND_MAX-.5f)*20000;
					Force.y = 200000+(float)rand()/(float)RAND_MAX*5000;
					Force.z = ((float)rand()/(float)RAND_MAX-.5f)*20000;
					NxVec3 Location;
					Location.x = 0;
					Location.y = 40;
					Location.z = 0;
					_particle_list[i].Particle->addForce(Force);
					_particle_list[i].Particle->moveGlobalPosition(Location);
					NxBoxShapeDesc ShapeDesc;
					ShapeDesc.materialIndex = _particle_material;
					ShapeDesc.dimensions.set( 2.0f, 2.0f, 2.0f);
					_particle_list[i].Particle->createShape(ShapeDesc);
				}
				break;
			}
		}
	}

}
void DXApp::InitRock()
{
	/*LPD3DXBUFFER RockShader;
	LPD3DXCONSTANTTABLE RockConstants;
	IDirect3DPixelShader9 *rock_shader;

	D3DXCompileShaderFromFile(strRockShader1, NULL, NULL, "ps_main", "ps_2_0", D3DXSHADER_DEBUG, &RockShader, NULL, &RockConstants);
	_p_device->CreatePixelShader(&RockShader, &_rock_shader);

	_p_device->CreateVertexBuffer(sizeof(ScreenQuadPoint)*4, D3DUSAGE_WRITEONLY, screen_quad_fvf
	ScreenQuadPoint screen_quad[4];
	screen_quad[0].location.x = -1;
	screen_quad[0].location.y = -1;
	screen_quad[0].location.z = 0;
	screen_quad[0].location.x = 1;
	screen_quad[0].location.y = -1;
	screen_quad[0].location.z = 0;
	screen_quad[0].location.x = -1;
	screen_quad[0].location.y = 1;
	screen_quad[0].location.z = 0;
	screen_quad[0].location.x = 1;
	screen_quad[0].location.y = 1;
	screen_quad[0].location.z = 0;


	_p_device->SetFVF(screen_quad_fvf);
	//Setup Rendering to Texture.
	_p_device->CreateTexture(32, 32, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &_point_texture, NULL);
	_point_texture->GetSurfaceLevel(0, &_point_surface);
	//D3DXMatrixPerspectiveFovLH(&matProjection,D3DX_PI / 4.0f,1,1,100);
	_p_device->GetRenderTarget(0, &_back_buffer);
	_p_device->SetRenderTarget(0, _point_surface);
	_p_device->Clear(0, NULL, D3DCLEAR_TARGET, 0xFFFFFF00, 1.0f, 0);

	//render the texture
	_p_device->BeginScene();

	_p_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 4, &screen_quad, sizeof(DrawParticle));

	_p_device->EndScene();

	//setup rendering to the backbuffer  
	//_p_device->SetRenderTarget(0, _back_buffer);
	*/
}

void DXApp::HandleInput(WPARAM key, bool bDown)
{
	NxVec3 Temp;
	switch(key)
	{
	case VK_UP:
	case 'W':
		fFoward = bDown ? 1.0f : 0.0f;
		break;
	case VK_DOWN:
	case 'S':
		fFoward = bDown ? -1.0f : 0.0f;
		break;
	case VK_LEFT:
	case 'A':
		fStrafe = bDown ? 1.0f : 0.0f;
		break;
	case VK_RIGHT:
	case 'D':
		fStrafe = bDown ? -1.0f : 0.0f;
		break;
	case VK_SPACE:
		if(bDown)
			bBlurVel = !bBlurVel;
		break;
	case VK_PAUSE:
		if(bDown)
			bPaused = !bPaused;
		break;
	case VK_TAB:
		if(bDown)
			bTest = !bTest;
		break;
	default:
		break;
	}
	return;
}
void DXApp::HandleMouseMove(bool bMouseDown, int X, int Y)
{
	if(bMouseDown)
	{
		NxVec3 Temp;
		Temp = Cam.location;
		Temp.y = 0;
		Temp.normalize();
		Temp = Temp.cross(NxVec3(0.0f, 1.0f, 0.0f));
		NxQuat(((float)(Y-OldMY))/20.0f, Temp).rotate(Cam.ViewDir);

		Temp = NxVec3(0.0f, 1.0f, 0.0f);
		NxQuat(((float)(X-OldMX))/20.0f, Temp).rotate(Cam.ViewDir);
	}
	OldMX = X;
	OldMY = Y;
}