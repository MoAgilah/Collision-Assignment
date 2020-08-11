#include "HeightMap.h"
#include "Utils.h"
#include "Collisions.h"

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

bool sortfirstX(FaceCollisionData* i, FaceCollisionData* j) { return i->m_v0.x < j->m_v0.x; }

HeightMap::HeightMap( char* filename, float gridSize, float heightRange )
{
	LoadHeightMap(filename, gridSize, heightRange);

	m_pHeightMapBuffer = NULL;

	m_pPSCBuffer = NULL;
	m_pVSCBuffer = NULL;

	m_HeightMapFaceCount = (m_HeightMapLength-1)*(m_HeightMapWidth-1)*2;

	m_pFaceData = new FaceCollisionData[m_HeightMapFaceCount];
	for (int f = 0; f < m_HeightMapFaceCount; ++f)
	{
		m_pFaceData[f].m_bDisabled = false;
		m_pFaceData[f].m_bCollided = false;
		m_pFaceData[f].m_bDebug = false;
	}
	
	m_HeightMapVtxCount = m_HeightMapFaceCount*3;
		
	for (size_t i = 0; i < NUM_TEXTURE_FILES; ++i)
	{
		m_pTextures[i] = NULL;
		m_pTextureViews[i] = NULL;
	}

	m_pSamplerState = NULL;

	m_pHeightMapBuffer = CreateDynamicVertexBuffer(Application::s_pApp->GetDevice(), sizeof Vertex_Pos3fColour4ubNormal3fTex2f * m_HeightMapVtxCount, 0 );
	
	BuildCollisionData();
	RebuildVertexData();

	for (size_t i = 0; i < NUM_TEXTURE_FILES; ++i)
	{
		LoadTextureFromFile(Application::s_pApp->GetDevice(), g_aTextureFileNames[i], &m_pTextures[i], &m_pTextureViews[i], &m_pSamplerState);
	}


	ReloadShader(); // This compiles the shader
}

void HeightMap::BuildCollisionData(void)
{
	int mapIndex = 0;
	int faceIndex = 0;
	XMVECTOR v0, v1, v2, v3;
	int i0, i1, i2, i3;
	// This is the unstripped method, I wouldn't recommend changing this to the stripped method for the collision assignment		
	for (int l = 0; l < m_HeightMapLength; ++l)
	{
		for (int w = 0; w < m_HeightMapWidth; ++w)
		{
			if (w < m_HeightMapWidth - 1 && l < m_HeightMapLength - 1)
			{
				i0 = mapIndex;
				i1 = mapIndex + m_HeightMapWidth;
				i2 = mapIndex + 1;
				i3 = mapIndex + m_HeightMapWidth + 1;
				v0 = XMLoadFloat4(&m_pHeightMap[i0]);
				v1 = XMLoadFloat4(&m_pHeightMap[i1]);
				v2 = XMLoadFloat4(&m_pHeightMap[i2]);
				v3 = XMLoadFloat4(&m_pHeightMap[i3]);
				XMVECTOR vA = v0 - v1;
				XMVECTOR vB = v1 - v2;
				XMVECTOR vC = v3 - v1;
				XMVECTOR vN1, vN2;
				vN1 = XMVector3Cross(vA, vB);
				vN1 = XMVector3Normalize(vN1);
				vN2 = XMVector3Cross(vB, vC);
				vN2 = XMVector3Normalize(vN2);
				XMStoreFloat3(&m_pFaceData[faceIndex + 0].m_v0, v0);
				XMStoreFloat3(&m_pFaceData[faceIndex + 0].m_v1, v1);
				XMStoreFloat3(&m_pFaceData[faceIndex + 0].m_v2, v2);
				XMStoreFloat3(&m_pFaceData[faceIndex + 0].m_vNormal, vN1);
				XMStoreFloat3(&m_pFaceData[faceIndex + 1].m_v0, v2);
				XMStoreFloat3(&m_pFaceData[faceIndex + 1].m_v1, v1);
				XMStoreFloat3(&m_pFaceData[faceIndex + 1].m_v2, v3);
				XMStoreFloat3(&m_pFaceData[faceIndex + 1].m_vNormal, vN2);
				faceIndex += 2;
			}
			mapIndex++;
		}
	}
}


void HeightMap::RebuildVertexData(void)
{
	D3D11_MAPPED_SUBRESOURCE map;

	if (SUCCEEDED(Application::s_pApp->GetDeviceContext()->Map(m_pHeightMapBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
	{
		Vertex_Pos3fColour4ubNormal3fTex2f* pMapVtxs = (Vertex_Pos3fColour4ubNormal3fTex2f*)map.pData;

		int vtxIndex = 0;
		int mapIndex = 0;

		XMVECTOR v0, v1, v2, v3, v4, v5;
		float tX0, tY0, tX1, tY1, tX2, tY2, tX3, tY3;
		int i0, i1, i2, i3;

		VertexColour c0, c1, c2, c3;
		XMVECTOR vN1, vN2;

		static VertexColour STANDARD_COLOUR(255, 255, 255, 255);
		static VertexColour COLLISION_COLOUR(255, 0, 0, 255);
		static VertexColour DEBUG_COLOUR(0, 255, 0, 255);

		VertexColour CUBE_COLOUR;

		// This is the unstripped method, I wouldn't recommend changing this to the stripped method for the collision assignment
		for (int f = 0; f < m_HeightMapFaceCount; f += 2)
		{
			v0 = XMLoadFloat3(&m_pFaceData[f + 0].m_v0);
			v1 = XMLoadFloat3(&m_pFaceData[f + 0].m_v1);
			v2 = XMLoadFloat3(&m_pFaceData[f + 0].m_v2);
			v3 = XMLoadFloat3(&m_pFaceData[f + 1].m_v0);
			v4 = XMLoadFloat3(&m_pFaceData[f + 1].m_v1);
			v5 = XMLoadFloat3(&m_pFaceData[f + 1].m_v2);

			if (m_pFaceData[f + 0].m_bDisabled)
				v0 = v1 = v2 = XMVectorZero();

			if (m_pFaceData[f + 1].m_bDisabled)
				v3 = v4 = v5 = XMVectorZero();

			vN1 = XMLoadFloat3(&m_pFaceData[f + 0].m_vNormal);
			vN2 = XMLoadFloat3(&m_pFaceData[f + 1].m_vNormal);

			tX0 = 0.0f;
			tY0 = 0.0f;
			tX1 = 0.0f;
			tY1 = 1.0f;
			tX2 = 1.0f;
			tY2 = 0.0f;
			tX3 = 1.0f;
			tY3 = 1.0f;

			if (m_pFaceData[f + 0].m_bDebug)
			{
				c0 = m_pFaceData[f + 0].m_bCollided ? COLLISION_COLOUR : DEBUG_COLOUR;
			}
			else
			{
				c0 = m_pFaceData[f + 0].m_bCollided ? COLLISION_COLOUR : STANDARD_COLOUR;
			}

			if (m_pFaceData[f + 1].m_bDebug)
			{
				c1 = m_pFaceData[f + 1].m_bCollided ? COLLISION_COLOUR : DEBUG_COLOUR;
			}
			else
			{
				c1 = m_pFaceData[f + 1].m_bCollided ? COLLISION_COLOUR : STANDARD_COLOUR;
			}

			pMapVtxs[vtxIndex + 0] = Vertex_Pos3fColour4ubNormal3fTex2f(v0, c0, vN1, XMFLOAT2(tX0, tY0));
			pMapVtxs[vtxIndex + 1] = Vertex_Pos3fColour4ubNormal3fTex2f(v1, c0, vN1, XMFLOAT2(tX1, tY1));
			pMapVtxs[vtxIndex + 2] = Vertex_Pos3fColour4ubNormal3fTex2f(v2, c0, vN1, XMFLOAT2(tX2, tY2));
			pMapVtxs[vtxIndex + 3] = Vertex_Pos3fColour4ubNormal3fTex2f(v3, c1, vN2, XMFLOAT2(tX2, tY2));
			pMapVtxs[vtxIndex + 4] = Vertex_Pos3fColour4ubNormal3fTex2f(v4, c1, vN2, XMFLOAT2(tX1, tY1));
			pMapVtxs[vtxIndex + 5] = Vertex_Pos3fColour4ubNormal3fTex2f(v5, c1, vN2, XMFLOAT2(tX3, tY3));

			vtxIndex += 6;
		}

		Application::s_pApp->GetDeviceContext()->Unmap(m_pHeightMapBuffer, 0);
	}
}

int HeightMap::DisableBelowLevel(float fYLevel)
{
	int nHidden = 0;

	for (int f = 0; f < m_HeightMapFaceCount; ++f)
	{
		if (m_pFaceData[f].m_v0.y < fYLevel && m_pFaceData[f].m_v1.y < fYLevel && m_pFaceData[f].m_v2.y < fYLevel)
		{
			m_pFaceData[f].m_bDisabled = true;
			nHidden++;
		}
	}

	RebuildVertexData();
	return nHidden;
}

int HeightMap::EnableAll(void)
{
	int nHidden = 0;

	for (int f = 0; f < m_HeightMapFaceCount; ++f)
	{
		if (m_pFaceData[f].m_bDisabled == true)
		{
			m_pFaceData[f].m_bDisabled = false;
			nHidden++;
		}
	}

	RebuildVertexData();
	return nHidden;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

HeightMap::~HeightMap()
{
	if( m_pHeightMap )
		delete m_pHeightMap;

	for (size_t i = 0; i < NUM_TEXTURE_FILES; ++i)
	{
		Release( m_pTextures[i] );
		Release( m_pTextureViews[i] );
	}

	Release(m_pHeightMapBuffer);

	DeleteShader();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void HeightMap::Draw( float frameCount )
{

	D3DXMATRIX worldMtx;
	D3DXMatrixIdentity(&worldMtx);

	ID3D11DeviceContext* pContext = Application::s_pApp->GetDeviceContext();

	Application::s_pApp->SetWorldMatrix(worldMtx);

	// Fill in the `myGlobals' cbuffer.
	//
	// The D3D11_MAP_WRITE_DISCARD flag is best for performance, but
	// leaves the buffer contents indeterminate. The entire buffer
	// needs to be set up for each draw.
	//
	// (This is the reason you need to "your" globals
	// into your own cbuffer - the cbuffer set up by CommonApp is
	// mapped using D3D11_MAP_WRITE_DISCARD too. If you set "your"
	// values correctly by hand, they will likely disappear when
	// the CommonApp maps the buffer to set its own variables.)
	if (m_pPSCBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE map;
		if (SUCCEEDED(pContext->Map(m_pPSCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
			{
				// Set the buffer contents. There is only one variable to set in this case.
				SetCBufferFloat(map, m_psFrameCount, frameCount);
				pContext->Unmap(m_pPSCBuffer, 0);
			}
		}

		if (m_pPSCBuffer)
		{
			ID3D11Buffer *apConstantBuffers[] = {
				m_pPSCBuffer,
			};

			pContext->PSSetConstantBuffers(m_psCBufferSlot, 1, apConstantBuffers);
		}

		if (m_pVSCBuffer)
		{
			D3D11_MAPPED_SUBRESOURCE map;
			if (SUCCEEDED(pContext->Map(m_pVSCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
			{
				// Set the buffer contents. There is only one variable to set
				// in this case.
				SetCBufferFloat(map, m_vsFrameCount, frameCount);

				pContext->Unmap(m_pVSCBuffer, 0);
			}
		}

		if (m_pVSCBuffer)
		{
			ID3D11Buffer *apConstantBuffers[] = {
				m_pVSCBuffer,
			};

			pContext->VSSetConstantBuffers(m_vsCBufferSlot, 1, apConstantBuffers);
		}


	if (m_psTexture0 >= 0)
		pContext->PSSetShaderResources(m_psTexture0, 1, &m_pTextureViews[0]);

	if (m_psTexture1 >= 0)
		pContext->PSSetShaderResources(m_psTexture1, 1, &m_pTextureViews[1]);

	if (m_psTexture2 >= 0)
		pContext->PSSetShaderResources(m_psTexture2, 1, &m_pTextureViews[2]);

	if (m_psMaterialMap >= 0)
		pContext->PSSetShaderResources(m_psMaterialMap, 1, &m_pTextureViews[3]);
	
	if (m_vsMaterialMap >= 0)
		pContext->VSSetShaderResources(m_vsMaterialMap, 1, &m_pTextureViews[3]);


	m_pSamplerState = Application::s_pApp->GetSamplerState( true, true, true);

	Application::s_pApp->DrawWithShader(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, m_pHeightMapBuffer, sizeof( Vertex_Pos3fColour4ubNormal3fTex2f ), 
		NULL, 0, m_HeightMapVtxCount, NULL, m_pSamplerState, &m_shader);
}

bool HeightMap::ReloadShader( void )
{
	DeleteShader();

	ID3D11VertexShader *pVS = NULL;
	ID3D11PixelShader *pPS = NULL;
	ID3D11InputLayout *pIL = NULL;
	ShaderDescription vs, ps;

	ID3D11Device* pDevice = Application::s_pApp->GetDevice();

	// When the CommonApp draw functions set any of the light arrays,
	// they assume that the arrays are of CommonApp::MAX_NUM_LIGHTS
	// in size. Using a shader compiler #define is an easy way to
	// get this value to the shader.

	char maxNumLightsValue[100];
	_snprintf_s(maxNumLightsValue, sizeof maxNumLightsValue, _TRUNCATE, "%d", CommonApp::MAX_NUM_LIGHTS);

	D3D_SHADER_MACRO aMacros[] = {
		{"MAX_NUM_LIGHTS", maxNumLightsValue, },
		{NULL},
	};

	if (!CompileShadersFromFile(pDevice, "./Resources/ExampleShader.hlsl", "VSMain", &pVS, &vs, g_aVertexDesc_Pos3fColour4ubNormal3fTex2f,
		g_vertexDescSize_Pos3fColour4ubNormal3fTex2f, &pIL, "PSMain", &pPS, &ps, aMacros))
	{

		return false;// false;
	}

	Application::s_pApp->CreateShaderFromCompiledShader(&m_shader, pVS, &vs, pIL, pPS, &ps);

	// Find cbuffer(s) for the globals that won't get set by the CommonApp
	// code. These are shader-specific; you have to know what you are
	// looking for, if you're going to set it...
	//
	// There are separate sets of constants for vertex and pixel shaders,
	// which need setting up separately. See the CommonApp code for
	// an example of doing both. In this case, we know that the vertex
	// shader doesn't do anything interesting, so we only check the pixel
	// shader constants.

	ps.FindCBuffer("MyApp", &m_psCBufferSlot);
	ps.FindFloat(m_psCBufferSlot, "g_frameCount", &m_psFrameCount);

	vs.FindCBuffer("MyApp", &m_vsCBufferSlot);
	vs.FindFloat(m_vsCBufferSlot, "g_frameCount", &m_vsFrameCount);

	ps.FindTexture( "g_texture0", &m_psTexture0 );
	ps.FindTexture( "g_texture1", &m_psTexture1 );
	ps.FindTexture( "g_texture2", &m_psTexture2 );
	ps.FindTexture( "g_materialMap", &m_psMaterialMap );
	
	vs.FindTexture( "g_materialMap", &m_vsMaterialMap );
	
	// Create the cbuffer, using the shader description to find out how
	// large it needs to be.
	m_pPSCBuffer = CreateBuffer(pDevice, ps.GetCBufferSizeBytes(m_psCBufferSlot), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, NULL);
	m_pVSCBuffer = CreateBuffer(pDevice, vs.GetCBufferSizeBytes(m_vsCBufferSlot), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, NULL);

	return true;

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void HeightMap::DeleteShader()
{
	Release(m_pPSCBuffer);
	Release(m_pVSCBuffer);

	m_shader.Reset();
}

//////////////////////////////////////////////////////////////////////
// LoadHeightMap
// Original code sourced from rastertek.com
//////////////////////////////////////////////////////////////////////
bool HeightMap::LoadHeightMap(char* filename, float gridSize, float heightRange )
{
	FILE* filePtr;
	int error;
	int count;
	BITMAPFILEHEADER bitmapFileHeader;
	BITMAPINFOHEADER bitmapInfoHeader;
	int imageSize, i, j, k, index;
	unsigned char* bitmapImage;
	unsigned char height;


	// Open the height map file in binary.
	error = fopen_s(&filePtr, filename, "rb");
	if(error != 0)
	{
		return false;
	}

	// Read in the file header.
	count = fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);
	if(count != 1)
	{
		return false;
	}

	// Read in the bitmap info header.
	count = fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);
	if(count != 1)
	{
		return false;
	}

	// Save the dimensions of the terrain.
	m_HeightMapWidth = bitmapInfoHeader.biWidth;
	m_HeightMapLength = bitmapInfoHeader.biHeight;

	// Calculate the size of the bitmap image data.
	imageSize = m_HeightMapWidth * m_HeightMapLength * 3;

	// Allocate memory for the bitmap image data.
	bitmapImage = new unsigned char[imageSize];
	if(!bitmapImage)
	{
		return false;
	}

	// Move to the beginning of the bitmap data.
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

	// Read in the bitmap image data.
	count = fread(bitmapImage, 1, imageSize, filePtr);
	if(count != imageSize)
	{
		return false;
	}

	// Close the file.
	error = fclose(filePtr);
	if(error != 0)
	{
		return false;
	}

	// Create the structure to hold the height map data.
	m_pHeightMap = new XMFLOAT4[m_HeightMapWidth * m_HeightMapLength];

	if(!m_pHeightMap)
	{
		return false;
	}

	// Initialize the position in the image data buffer.
	k=0;


	// Read the image data into the height map.
	for(j=0; j<m_HeightMapLength; j++)
	{
		for(i=0; i<m_HeightMapWidth; i++)
		{
			height = bitmapImage[k];
			
			index = (m_HeightMapWidth * j) + i;

			m_pHeightMap[index].x = (i-(((float)m_HeightMapWidth-1)/2))*gridSize;
			m_pHeightMap[index].y = (float)height / 6 * heightRange;
			m_pHeightMap[index].z = (j-(((float)m_HeightMapLength-1)/2))*gridSize;
			m_pHeightMap[index].w = 0;

			k+=3;
		}
	}


	// Release the bitmap image data.
	delete [] bitmapImage;
	bitmapImage = 0;

	return true;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


bool HeightMap::PointOverQuad(XMVECTOR& vPos, XMVECTOR& v0, XMVECTOR& v1, XMVECTOR& v2)
{
	if (XMVectorGetX(vPos) < max(XMVectorGetX(v1), XMVectorGetX(v2)) && XMVectorGetX(vPos) > min(XMVectorGetX(v1), XMVectorGetX(v2)))
		if (XMVectorGetZ(vPos) < max(XMVectorGetZ(v1), XMVectorGetZ(v2)) && XMVectorGetZ(vPos) > min(XMVectorGetZ(v1), XMVectorGetZ(v2)))
			return true;

	return false;
}

bool HeightMap::RayCollision(Ball* b,XMVECTOR& colPos, XMVECTOR& colNormN)
{

	XMVECTOR v0, v1, v2, v3;
	int i0, i1, i2, i3;
	float colDist = 0.0f;

	// This resets the collision colouring
	for (int f = 0; f < m_HeightMapFaceCount; ++f)
		m_pFaceData[f].m_bCollided = false;

#ifdef COLOURTEST
	// This is just a piece of test code for the map colouring
	static float frame = 0;

	for (int f = 0; f < m_HeightMapFaceCount; ++f)
	{
		if ((int)frame%m_HeightMapFaceCount == f)
			m_pFaceData[f].m_bCollided = true;
	}

	RebuildVertexData();

	frame += 0.1f;

	// end of test code
#endif

	// This is a brute force solution that checks against every triangle in the heightmap
	for (int f = 0; f < m_HeightMapFaceCount; ++f)
	{
		//alternative to original to use robust poly intersection
		if (!m_pFaceData[f].m_bDisabled && Collisions::GetCol()->TestSpherePolyIntersection(b, m_pFaceData[f],colDist))
		{
			//pass back normal
			colNormN = XMLoadFloat3(&m_pFaceData[f].m_vNormal);
			//resolve the intersection with the heightmap
			Collisions::GetCol()->ResolveIntersection(b, colDist);
			//tell the face data it has been collided with
			m_pFaceData[f].m_bCollided = true;
			//rebuild vertex data
			RebuildVertexData();
			return true;
		}
	}
	return false;
}

void HeightMap::ColourTarget(int index) 
{
	//disable previous target
	for (int i = 0; i < m_HeightMapFaceCount; i++)
	{
		m_pFaceData[i].m_bDebug = false;
	}

	//set new target
	m_pFaceData[index].m_bDebug = true;
	
	//redraw the grid
	RebuildVertexData();
}

void HeightMap::GetTriangle(int index, XMFLOAT3 con[4])
{
	//get the exact point of the center point from the three corners
	con[0] = XMFLOAT3((m_pFaceData[index].m_v0.x + m_pFaceData[index].m_v1.x + m_pFaceData[index].m_v2.x)/3,
		(m_pFaceData[index].m_v0.y + m_pFaceData[index].m_v1.y + m_pFaceData[index].m_v2.y)/3,
		(m_pFaceData[index].m_v0.z + m_pFaceData[index].m_v1.z + m_pFaceData[index].m_v2.z)/3);
	
	//using the index provided obtain corner 1 vertices
	con[1] = m_pFaceData[index].m_v0;
	
	//using the index provided obtain corner 2 vertices
 	con[2] = m_pFaceData[index].m_v1;
	
	//using the index provided obtain corner 3 vertices
	con[3] = m_pFaceData[index].m_v2;

	//increase y by 20 to allow it to drop
	con[0].y = 20;
	con[1].y = 20;
	con[2].y = 20;
	con[3].y = 20;
}