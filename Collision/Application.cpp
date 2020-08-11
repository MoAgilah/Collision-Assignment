#include "Application.h"
#include "HeightMap.h"
#include "Ball.h"
#include "BallManager.h"
#include "Collisions.h"

Application* Application::s_pApp = NULL;

const int CAMERA_TOP = 0;
const int CAMERA_ROTATE = 1;
const int CAMERA_MAX = 2;

static int a(0), b(30);

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

bool Application::HandleStart()
{
	s_pApp = this;

	m_frameCount = 0.0f;

	m_bWireframe = true;

	//load initial heightmap
	m_pHeightMap = new HeightMap("Resources/heightmap.bmp", 2.0f, 0.75f);

	m_cameraZ = 50.0f;
	m_rotationAngle = 0.f;

	m_reload = false;

	ReloadShaders();

	if (!this->CommonApp::HandleStart())
		return false;

	this->SetRasterizerState( false, m_bWireframe );

	m_cameraState = CAMERA_ROTATE;

	debugSlow = false;

	//create new ball manager
	ballMgr = new BallManager(m_pHeightMap);

	//intialising the debug cyclic with the tops center and 3 corners
	m_pHeightMap->GetTriangle(0, debugCyclic);

	//paiting the top as initial target
	m_pHeightMap->ColourTarget(0);

	new Collisions;

	//setup ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO();

	bool test = ImGui_ImplWin32_Init(this->m_hWnd);

	test = ImGui_ImplDX11_Init(this->GetDevice(), this->GetDeviceContext());
	ImGui::StyleColorsDark();

	return true;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void Application::HandleStop()
{
	// Shutdown
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	delete m_pHeightMap;

	if( ballMgr )
		delete ballMgr;

	delete Collisions::GetCol();

	this->CommonApp::HandleStop();
}



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void Application::ReloadShaders()
{
	if( m_pHeightMap->ReloadShader() == false )
		this->SetWindowTitle("Reload Failed - see Visual Studio output window. Press F5 to try again.");
	else
		this->SetWindowTitle("Your Shader Here. Press F5 to reload shader file.");
}

void Application::HandleUpdate(float dt)
{
	//initial set to below
	//float step = dt * 60;
	//cause a frame rate issue because of the lack of optimisation for the heightmap collision
	
	float step = 1.0f/60 * 60;	//hard coded delta time to ensure frame rate drop doesnt affect update

	if (this->IsKeyPressed(VK_ESCAPE))
	{
		PostQuitMessage(0);
	}

	//if space bar is pressed down 
	if (this->IsKeyPressed(' '))
	{
		//reduce frame rate 1 frame a second
		debugSlow = true;
	}
	else
	{
		//regular frame rate of 60 frames a second
		debugSlow = false;
	}


	if( m_cameraState == CAMERA_ROTATE )
	{
		if (this->IsKeyPressed('Q') && m_cameraZ > 38.0f )
			m_cameraZ -= 1.0f * step;
		
		if (this->IsKeyPressed('A'))
			m_cameraZ += 1.0f * step;

		if (this->IsKeyPressed('O'))
			m_rotationAngle -= .01f * step;
		
		if (this->IsKeyPressed('P'))
			m_rotationAngle += .01f * step;
	}

	
	static bool dbC = false;

	if (this->IsKeyPressed('C') )	
	{
		if( !dbC )
		{
			if( ++m_cameraState == CAMERA_MAX )
				m_cameraState = CAMERA_TOP;

			dbC = true;
		}
	}
	else dbC = false;


	static bool dbW = false;
	if (this->IsKeyPressed('W') )	
	{
		if( !dbW )
		{
			m_bWireframe = !m_bWireframe;
			this->SetRasterizerState( false, m_bWireframe );
			dbW = true;
		}
	}
	else dbW = false;
	


	if (this->IsKeyPressed(VK_F5))
	{
		if (!m_reload)
		{
			ReloadShaders();
			m_reload = true;
		}
	}
	else m_reload = false;

	//init a ball at a random position using the ball mgr
	static bool dbR = false;
	if (this->IsKeyPressed('R') )
	{
		if( dbR == false )
		{
			static int dx = 0;
			static int dy = 0;

			//init ball
			XMFLOAT3 pos = XMFLOAT3((float)((rand() % 14 - 7.0f) - 0.5), 20.0f, (float)((rand() % 14 - 7.0f) - 0.5));
			ballMgr->InitBall(pos, XMFLOAT3(0.0f, 0.2f, 0.0f), XMFLOAT3(0.0f, -0.05f, 0.0f));

			dbR = true;
		}
	}
	else dbR = false;
	
	//init a ball at a top triangle's position using the ball mgr
	static bool dbT = false;
	if (this->IsKeyPressed('T'))
	{
		if (dbT == false)
		{
			static int dx = 0;
			static int dy = 0;
			ballMgr->InitBall(XMFLOAT3(-14.0f, 20.0f, -14.0f), XMFLOAT3(0.0f, 0.2f, 0.0f), XMFLOAT3(0.0f, -0.05f, 0.0f));
			dbT = true;
		}
	}
	else dbT = false;

	static int dx = 0;
	static int dy = 0;
	static int seg = 0;
	int rowLength = 29;
	static int rowNumber = 0;
	static bool dbN[2]{false, false};
	static int trigger = 0;
	
	//forward incremental left to right
	if (this->IsKeyPressed('U') )
	{
		if (dbN[0] == false)
		{
			if( ++seg == 2 )
			{
				seg=0;
				if( ++dx==15 )  
				{
					if( ++dy ==15 ) dy=0;
					dx = 0;
					if (++rowNumber == 15)rowNumber = 0;
				}
			}

			//formula to calculate array index of targeted triangle
			int index = (((dx * 2) + dy) + seg) + (rowLength*rowNumber);

			//output array index for testing
			char text[256];
			sprintf_s(text, "arrayIndex =%i\n", index);
			OutputDebugStringA(text);

			//colour new target
			m_pHeightMap->ColourTarget(index);
			//obtain target's corners
			m_pHeightMap->GetTriangle(index, debugCyclic);
			//set trigger back to the center for next drop
			trigger = 0;
			dbN[0] = true;
		}
	}
	else dbN[0] = false;

	//backwards decremental right to left
	if (this->IsKeyPressed('I'))
	{
		if (dbN[1] == false)
		{
			if (--seg < 0)
			{
				seg = 1;

				if (--dx < 0)
				{
					if (--dy < 0) dy = 14;
					dx = 14;
					if (--rowNumber < 0)rowNumber = 14;
				}
			}

			//formula to calculate array index of targeted triangle
			int index = (((dx * 2) + dy) + seg) + (rowLength*rowNumber);

			//output array index for testing
			char text[256];
			sprintf_s(text, "arrayIndex =%i\n", index);
			OutputDebugStringA(text);

			//colour new target
			m_pHeightMap->ColourTarget(index);
			//obtain target's corners
			m_pHeightMap->GetTriangle(index, debugCyclic);
			//set trigger back to the center for next drop
			trigger = 0;
			dbN[1] = true;
		}
	}
	else dbN[1] = false;
	

	
	static bool trig = false;
	//drop the debug ball on specified corner, or center of chosen triangle
	if (this->IsKeyPressed('D'))
	{
		if (trig == false)
		{	
			ballMgr->InitDebugBall(debugCyclic[trigger], XMFLOAT3(0.0f, 0.2f, 0.0f), XMFLOAT3(0.0f, -0.05f, 0.0f));
			trig = true;
			//increment trigger to get next corner or return to center
			if (++trigger == 4) trigger = 0;
		}
		
	}
	else trig = false;
	

	static bool clear = false;
	//if the e key is pressed deactivate all balls
	if (this->IsKeyPressed('E'))
	{
		if (!clear)
		{
			ballMgr->ClearAll();
			clear = true;
		}
	}
	else clear = false;

	//cycle the choice to drop at specified position for a nice bounce effect or at target position and it's inverse
	static bool dDSwitch = false;
	static bool dDChoice = false;
	if (this->IsKeyPressed('S'))
	{
		if (!dDSwitch)
		{
			dDChoice = !dDChoice;
			dDSwitch = true;
		}
	}
	else
	{
		dDSwitch = false;
	}

	static bool doubleDrop = false;
	//if the f key is pressed drop the current ball and another in it's inverted position
	if (this->IsKeyPressed('F'))
	{
		if (doubleDrop == false)
		{
			if (!dDChoice)
				//create a ball at specified position
				ballMgr->InitBall(XMFLOAT3(7.0f, 20.0f, 0.0f), XMFLOAT3(0.0f, 0.2f, 0.0f), XMFLOAT3(0.0f, -0.05f, 0.0f));
			else
				//create a ball at the last target position
				ballMgr->InitBall(debugCyclic[trigger], XMFLOAT3(0.0f, 0.2f, 0.0f), XMFLOAT3(0.0f, -0.05f, 0.0f));
			
			if (!dDChoice)
				//assign specified inverted position and the same velocity, and gravity to the newly created ball
				ballMgr->InitBall(XMFLOAT3(-7.0f, 20.0f, 0.0f), XMFLOAT3(0.0f, 0.2f, 0.0f), XMFLOAT3(0.0f, -0.05f, 0.0f));
			else
				//create a ball at the last inverted target position
				ballMgr->InitBall(XMFLOAT3(-debugCyclic[trigger].x, debugCyclic[trigger].y, -debugCyclic[trigger].z), XMFLOAT3(0.0f, 0.2f, 0.0f), XMFLOAT3(0.0f, -0.05f, 0.0f));
			

			doubleDrop = true;
		}
	}
	else doubleDrop = false;

	static bool triDisable = false;
	//if the h key is pressed disable the last twelve triangle
	if (this->IsKeyPressed('G'))
	{
		if (!triDisable)
		{
			m_pHeightMap->DisableBelowLevel(4.0f);
			
			triDisable = true;
		}
	}
	else triDisable = false;

	static bool triEnable = false;
	//if the g key is pressed enable all triangles
	if (this->IsKeyPressed('H'))
	{
		if (!triEnable)
		{
			m_pHeightMap->EnableAll();

			triEnable = true;
		}
	}
	else triEnable = false;

	static bool increaseDemBalls = false;
	//if the up key is pressed increment available number of balls
	if (this->IsKeyPressed(VK_UP))
	{
		if (!increaseDemBalls)
		{
			ballMgr->IncreaseNumOfBalls();

			increaseDemBalls = true;
		}
	}
	else increaseDemBalls = false;

	static bool decreaseDemBalls = false;
	//if the up key is pressed increment available number of balls
	if (this->IsKeyPressed(VK_DOWN))
	{
		if (!decreaseDemBalls)
		{
			ballMgr->DecreaseNumOfBalls();

			decreaseDemBalls = true;
		}
	}
	else decreaseDemBalls = false;
	
	static bool dropAll = false;
	//if the b key is pressed enable specified number
	if (this->IsKeyPressed('B'))
	{
		if (!dropAll)
		{
			ballMgr->DropAll();
			
			dropAll = true;
		}
	}
	else dropAll = false;

	//if debug mode is active
	if (debugSlow)
	{
		/*
		every 60 frames update them balls
			reduces framerate to 1 until space bar is released
		*/
		if ((int)m_frameCount % 60 == 0)
		{
			ballMgr->UpdateBalls(step);
		}
	}
	else
	{
		ballMgr->UpdateBalls(step);
	}

	//Demo Text - utilising the ImGui library
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//create Demo test window
	ImGui::Begin("Collision's Assignment", NULL);
	ImGui::SetWindowPos(ImVec2(25.6f, 25.6f));
	ImGui::SetWindowSize(ImVec2(326.f, 300.f));

	if (ImGui::CollapsingHeader("Demo Description:", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("	A real time 3d techniques for games");
		ImGui::Text("	assignment that is culmination of");
		ImGui::Text("	the module, with the ultimate ");
		ImGui::Text("	goal of achieving robust multiple");
		ImGui::Text("	Moving sphere collision checks");
		ImGui::Text("");
	}

	if (ImGui::CollapsingHeader("The demo's control's"))
	{
		ImGui::BulletText("C - Change current camera state");
		ImGui::BulletText("W - Change to wireframe");
		ImGui::BulletText("ESC - Quit Application");
		ImGui::Text("");
	}

	if (ImGui::CollapsingHeader("The ball's control's"))
	{
		ImGui::Text(("The number of available balls: " + std::to_string(ballMgr->GetNumberOfBalls()) + "/" + std::to_string(MaxNumBalls)).c_str());
		ImGui::Text("");
		ImGui::BulletText("Up Arrow - Increase balls");
		ImGui::BulletText("Down Arrow - Decrease balls");
		ImGui::BulletText("B - drop all ball's");
		ImGui::BulletText("E - clear all ball's");

		ImGui::BulletText("D - drop ball on target");
		ImGui::BulletText("F - Double drop ball's");
		ImGui::BulletText("T - Drop ball on top triangle");
		ImGui::BulletText("R - drop ball on random target");

		ImGui::Text("");
		if (dDChoice) ImGui::Text("The current double drop state is: Target"); else ImGui::Text("The current double drop state is: Bounce");
		ImGui::Text("");
		ImGui::BulletText("S - Toggle target or demo double drop");
		ImGui::BulletText("U - Move target to the left");
		ImGui::BulletText("I - Move target to the right");

		ImGui::BulletText("G - Make hole");
		ImGui::BulletText("H - Seal hole");
	}

	

	ImGui::End();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void Application::HandleRender()
{
	XMVECTOR vCamera, vLookat;
	XMVECTOR vUpVector = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX matProj, matView;

	switch( m_cameraState )
	{
		case CAMERA_TOP:
			vCamera = XMVectorSet(0.0f, 100.0f, 0.1f, 0.0f);
			vLookat = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
			matView = XMMatrixLookAtLH(vCamera, vLookat, vUpVector);
			matProj = XMMatrixOrthographicLH(64, 36, 1.5f, 5000.0f);
			break;
		case CAMERA_ROTATE:
			vCamera = XMVectorSet(sin(m_rotationAngle)*m_cameraZ, (m_cameraZ*m_cameraZ) / 50, cos(m_rotationAngle)*m_cameraZ, 0.0f);
			vLookat = XMVectorSet(0.0f, 10.0f, 0.0f, 0.0f);
			matView = XMMatrixLookAtLH(vCamera, vLookat, vUpVector);
			matProj = XMMatrixPerspectiveFovLH(float(D3DX_PI / 7), 2, 1.5f, 5000.0f);
			break;
	}

	this->EnableDirectionalLight(1, XMFLOAT3(-1.f, -1.f, -1.f), XMFLOAT3(0.55f, 0.55f, 0.65f));
	this->EnableDirectionalLight(2, XMFLOAT3(1.f, -1.f, 1.f), XMFLOAT3(0.15f, 0.15f, 0.15f));

	this->SetViewMatrix(matView);
	this->SetProjectionMatrix(matProj);

	this->Clear(XMFLOAT4(0.05f, 0.05f, 0.5f, 1.f));

	//update all worlds
	ballMgr->UpdateWorlds();

	SetDepthStencilState(false, false);
	ballMgr->DrawBalls(this);
	
	
	SetDepthStencilState(true, true);
	m_pHeightMap->Draw(m_frameCount);

	SetDepthStencilState(true, true);
	ballMgr->DrawBalls(this);

	m_frameCount++;

	//assemble together draw data
	ImGui::EndFrame();
	ImGui::Render();

	//render draw data
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////



int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int)
{
	Application application;

	Run(&application);

	return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


