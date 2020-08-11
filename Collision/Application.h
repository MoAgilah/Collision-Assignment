#ifndef APPLICATION_H
#define APPLICATION_H

#define WIN32_LEAN_AND_MEAN

#include <assert.h>

#include <stdio.h>
#include <windows.h>
#include <d3d11.h>

#include "CommonApp.h"
#include "CommonMesh.h"
#include <string>

#include "../Collision/imgui/imgui.h"
#include "../Collision/imgui/imgui_impl_dx11.h"
#include "../Collision/imgui/imgui_impl_win32.h"

class HeightMap;
class Ball;
class BallManager;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// An example of creating a shader of your own, that fits in
// relatively neatly with the CommonApp functionality.
//
// Edit the shader as the program runs, then Alt+Tab back to it and
// press F5 to reload the shader. Instant feedback!
//
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

class Application:
public CommonApp
{
public:
	static Application* s_pApp;
protected:
	bool HandleStart();
	void HandleStop();
	void HandleUpdate(float dt);
	void HandleRender();
private:

	float m_frameCount;

	bool m_reload;

	float m_rotationAngle;
	float m_cameraZ;
	bool m_bWireframe;

	int m_cameraState;

	HeightMap* m_pHeightMap;

	BallManager* ballMgr;
	
	//variable to trigger debug mode
	bool debugSlow;	

	//a array to hold center and the corners of the current triangle
	XMFLOAT3 debugCyclic[4];

	void ReloadShaders();
};

#endif