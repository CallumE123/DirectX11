#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxcolors.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>

#include "resource.h"
#include "DDSTextureLoader.h"
#include "SimpleVertex.h"
#include "Lighting.h"
#include "GlobalVariables.h"

struct ConstantBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;
	XMVECTOR vLightPos;
	XMVECTOR vLightCol;
	XMVECTOR vLightAmb;
	XMVECTOR vLightDiff;
	XMVECTOR vEye;
};

// Forward declarations
bool InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ const HINSTANCE hInstance, _In_opt_ const HINSTANCE hPrevInstance, _In_ const LPWSTR   lpCmdLine, _In_ const int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = {nullptr};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return static_cast<int>(msg.wParam);
}

//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
bool InitWindow( const HINSTANCE hInstance, const int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = reinterpret_cast<HBRUSH>( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"ACW";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return false;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"ACW", L"Direct3D ACW",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );
    if( !g_hWnd )
        return false;

    ShowWindow( g_hWnd, nCmdShow );

    return true;
}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
bool CompileShaderFromFile(const WCHAR* const szFileName, const LPCSTR szEntryPoint, const LPCSTR szShaderModel,  ID3DBlob** const ppBlobOut )
{

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	const HRESULT hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, &pErrorBlob );
    if( FAILED(hr) )
    {
        if( pErrorBlob )
        {
            OutputDebugStringA( reinterpret_cast<const char*>( pErrorBlob->GetBufferPointer() ) );
            pErrorBlob->Release();
        }
        return false;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return true;
}

// Create Direct3D device and swap chain
HRESULT InitDevice()
{
    HRESULT hr = true;
    RECT rc;
    GetClientRect( g_hWnd, &rc );
    const UINT width = rc.right - rc.left;
	const UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice( nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );

        if (FAILED(hr))
        {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice( nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                                    D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        }

        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = g_pd3dDevice->QueryInterface( __uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice) );
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent( __uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory) );
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
        return hr;

    // Create swap chain
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface( __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2) );
    if ( dxgiFactory2 )
    {
        // DirectX 11.1 or later
        hr = g_pd3dDevice->QueryInterface( __uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1) );
        if (SUCCEEDED(hr))
        {
            g_pImmediateContext->QueryInterface( __uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1) );
        }

        DXGI_SWAP_CHAIN_DESC1 sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd( g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1 );
        if (SUCCEEDED(hr))
        {
            hr = g_pSwapChain1->QueryInterface( __uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain) );
        }

        dxgiFactory2->Release();
    }
    else
    {
        // DirectX 11.0 systems
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = g_hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain( g_pd3dDevice, &sd, &g_pSwapChain );
    }

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation( g_hWnd, DXGI_MWA_NO_ALT_ENTER );

    dxgiFactory->Release();

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &pBackBuffer ) );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

	// Create the Depth-Stencil texture object
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;

	// Create the Depth-Stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = static_cast<FLOAT>(width);
    vp.Height = static_cast<FLOAT>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

#pragma region Compiling the Shaders

    // Compile sphere vertex shader
    ID3DBlob* pVSBlob = nullptr;

	hr = CompileShaderFromFile(L"SphereVertex.hlsl", "main", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create sphere vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pSphereVertex);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Compile cube vertex shader
	hr = CompileShaderFromFile(L"CubeVertex.hlsl", "main", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create cube vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pCubeVertex);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Compile cube vertex shader
	hr = CompileShaderFromFile(L"GouraudVertex.hlsl", "main", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create cube vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pCubeVertex2);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Compile cube vertex shader
	hr = CompileShaderFromFile(L"DisplacementVS.hlsl", "main", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create cube vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pDisplacementVertex);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = ARRAYSIZE( layout );

    // Create the input layout
	hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
	pVSBlob->Release();
	if( FAILED( hr ) )
        return hr;

    // Set the input layout
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"PhongPixel.hlsl", "main", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pLightingPixel);
	if (FAILED(hr))
	{
		pPSBlob->Release();
		return hr;
	}

	// Compile pixel 2
	hr = CompileShaderFromFile(L"CubemapPixel.hlsl", "main", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create pixel 2
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pCubemapPixel);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Compile pixel 3
	hr = CompileShaderFromFile(L"TransparencyPixel.hlsl", "main", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create pixel 3
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pTransparentPixel);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Compile pixel 4
	hr = CompileShaderFromFile(L"BumpmapPixel.hlsl", "main", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create pixel 4
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pBumpPixel);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Compile pixel 5
	hr = CompileShaderFromFile(L"DisplacementPS.hlsl", "main", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create pixel 5
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pDisplacementPixel);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Compile pixel 6
	hr = CompileShaderFromFile(L"InkPixel.hlsl", "main", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create pixel 6
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pInkPixel);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;
	#pragma endregion

#pragma region Cube Loader
    // Create vertex buffer
    SimpleVertex vertices[] =
    {
		//CUBE
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f),	XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //1
		{ XMFLOAT3(1.0f, 1.0f, -1.0f),	XMFLOAT3(0.0f, 1.0f, 0.0f),	XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //2
		{ XMFLOAT3(1.0f, 1.0f, 1.0f),	XMFLOAT3(0.0f, 1.0f, 0.0f),	XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //3
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f),	XMFLOAT3(0.0f, 1.0f, 0.0f),	XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //4
		
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f),XMFLOAT3(0.0f, -1.0f, 0.0f),XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //5
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f),XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //6
		{ XMFLOAT3(1.0f, -1.0f, 1.0f),	XMFLOAT3(0.0f, -1.0f, 0.0f),XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //7
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f),XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //8

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f),XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //8
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f),XMFLOAT3(-1.0f, 0.0f, 0.0f),XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //5
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f),XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //1
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f),	XMFLOAT3(-1.0f, 0.0f, 0.0f),XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //4

		{ XMFLOAT3(1.0f, -1.0f, 1.0f),	XMFLOAT3(1.0f, 0.0f, 0.0f),	XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //7
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f),	XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //6
		{ XMFLOAT3(1.0f, 1.0f, -1.0f),	XMFLOAT3(1.0f, 0.0f, 0.0f),	XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //2
		{ XMFLOAT3(1.0f, 1.0f, 1.0f),	XMFLOAT3(1.0f, 0.0f, 0.0f),	XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //3

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f),XMFLOAT3(0.0f, 0.0f, -1.0f),XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //5
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //6
		{ XMFLOAT3(1.0f, 1.0f, -1.0f),	XMFLOAT3(0.0f, 0.0f, -1.0f),XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //2
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //1

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f),	XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //8
		{ XMFLOAT3(1.0f, -1.0f, 1.0f),  XMFLOAT3(0.0f, 0.0f, 1.0f),	XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //7
		{ XMFLOAT3(1.0f, 1.0f, 1.0f),	XMFLOAT3(0.0f, 0.0f, 1.0f),	XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //3
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f),	XMFLOAT3(0.0f, 0.0f, 1.0f),	XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)}, //4
	};
    D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( SimpleVertex ) * 24;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if( FAILED( hr ) )
        return hr;

	// Create index buffer
	WORD indices[] =
	{
		//CUBE
		3,1,0,
		2,1,3,

		6,4,5,
		7,4,6,

		11,9,8,
		10,9,11,

		14,12,13,
		15,12,14,

		19,17,16,
		18,17,19,

		22,20,21,
		23,20,22
	};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( WORD ) * 36;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;
	hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );
	if( FAILED( hr ) )
		return hr;

	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#pragma endregion

#pragma region Assimp Sphere Loader
	Assimp::Importer importer;
	const aiScene* const scene = importer.ReadFile("Sphere.obj", aiProcess_Triangulate | aiProcess_CalcTangentSpace);
	aiMesh* const mesh = scene->mMeshes[0];
	
	//Mesh Vertices
	std::vector<SimpleVertex>mesh_vertices;
	for (UINT i = 0; i < mesh->mNumVertices; i++) {
		SimpleVertex vertex;
		vertex.Pos.x = mesh->mVertices[i].x;
		vertex.Pos.y = mesh->mVertices[i].y;
		vertex.Pos.z = mesh->mVertices[i].z;

		vertex.Normal.x = mesh->mNormals[i].x;
		vertex.Normal.y = mesh->mNormals[i].y;
		vertex.Normal.z = mesh->mNormals[i].z;

		vertex.TexCoord.x = mesh->mTextureCoords[0][i].x;
		vertex.TexCoord.y = mesh->mTextureCoords[0][i].y;

		vertex.Tangent.x = mesh->mTangents[i].x;
		vertex.Tangent.y = mesh->mTangents[i].y;
		vertex.Tangent.z = mesh->mTangents[i].z;

		vertex.BiNormal.x = mesh->mBitangents[i].x;
		vertex.BiNormal.y = mesh->mBitangents[i].y;
		vertex.BiNormal.z = mesh->mBitangents[i].z;

		mesh_vertices.push_back(vertex);
	}

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * static_cast<UINT>(mesh_vertices.size());
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = mesh_vertices.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer2);
	if (FAILED(hr))
		return hr;

	//Mesh Indices
	std::vector<WORD>mesh_indices;
	for (UINT i = 0; i < mesh->mNumFaces; i++) {
		aiFace const face = mesh->mFaces[i];
		for (UINT j = 0; j < face.mNumIndices; j++)
			mesh_indices.push_back(face.mIndices[j]);
	}

	nIndices = mesh_indices.size();

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * static_cast<UINT>(mesh_indices.size());
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = mesh_indices.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer2);
	if (FAILED(hr))
		return hr;
#pragma endregion

	// Create the constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer( &bd, nullptr, &g_pConstantBuffer );
    if( FAILED( hr ) )
        return hr;

#pragma region Texture Loading
	//Texture Loader
	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"Skymap.dds", nullptr, &g_pBoxTextureRV);
	if (FAILED(hr))
		return hr;

	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"stones.dds", nullptr, &g_pStonesTextureRV);
	if (FAILED(hr))
		return hr;

	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"stones_NM_height.dds", nullptr, &g_pStonesNormalRV);
	if (FAILED(hr))
		return hr;

	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"dispMap.dds", nullptr, &g_pDispMapRV);
	if (FAILED(hr))
		return hr;
#pragma endregion

	//Set the Lighting values
	g_light = Lighting();
	g_light.LightCol = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	g_light.LightAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	g_light.LightDiffuse = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	g_light.LightPos = XMFLOAT4(0.0f, 10.0f, 0.0f, 0.f);

#pragma region Create SamplerStates
	//Create the sample state
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pBoxSampler);
	if (FAILED(hr))
		return hr;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pStonesSampler);
	if (FAILED(hr))
		return hr;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pDispMapSampler);
	if (FAILED(hr))
		return hr;
#pragma endregion

#pragma region DepthStencil
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = false;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = false;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;
	g_pd3dDevice->CreateDepthStencilState(&dsDesc, &g_pDepthStencilStateBox);

	dsDesc.DepthEnable = true;
	g_pd3dDevice->CreateDepthStencilState(&dsDesc, &g_pDepthStencilStateObjects);
#pragma endregion

#pragma region AlphaBlending
	D3D11_BLEND_DESC dsBlend;
	dsBlend.AlphaToCoverageEnable = false;
	dsBlend.IndependentBlendEnable = false;
	dsBlend.RenderTarget[0].BlendEnable = true;
	dsBlend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	dsBlend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	dsBlend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	dsBlend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	dsBlend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	dsBlend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	dsBlend.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	g_pd3dDevice->CreateBlendState(&dsBlend, &g_pBlendDesc);

	dsBlend.RenderTarget[0].BlendEnable = false;
	g_pd3dDevice->CreateBlendState(&dsBlend, &g_pNoBlendDesc);
#pragma endregion

#pragma region Rasterization
	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.ScissorEnable = false;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	hr = g_pd3dDevice->CreateRasterizerState(&rasterDesc, &g_pRasterStateBox);

	D3D11_RASTERIZER_DESC rasterDesc1;
	rasterDesc1.CullMode = D3D11_CULL_FRONT;
	rasterDesc1.FillMode = D3D11_FILL_SOLID;
	rasterDesc1.ScissorEnable = false;
	rasterDesc1.DepthBias = 0;
	rasterDesc1.DepthBiasClamp = 0.0f;
	rasterDesc1.DepthClipEnable = false;
	rasterDesc1.MultisampleEnable = false;
	rasterDesc1.SlopeScaledDepthBias = 0.0f;

	hr = g_pd3dDevice->CreateRasterizerState(&rasterDesc1, &g_pRasterStateObjects);
#pragma endregion

    // Initialize the world matrix
	g_World = XMMatrixIdentity();

    // Initialize the view matrix
	g_Eye = XMVectorSet(20.0f, 0.0f, 0.0f, 0.0f );
	g_Eye2 = XMVectorSet(0.0f, 20.0f, 0.0f, 0.0f);
	g_At = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
	g_Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	g_Up2 = XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f);
	g_View = XMMatrixLookAtLH(g_Eye, g_At, g_Up);

    // Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV2, static_cast<FLOAT>(width) / static_cast<FLOAT>(height), 0.01f, 100.0f );

    return true;
}

void CleanupDevice()
{
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();
	if (g_pDispMapSampler) g_pDispMapSampler->Release();
	if (g_pDispMapRV) g_pDispMapRV->Release();
	if (g_pStonesSampler) g_pStonesSampler->Release();
	if (g_pStonesNormalRV) g_pStonesNormalRV->Release();
	if (g_pStonesTextureRV) g_pStonesTextureRV->Release();
	if (g_pTileSampler) g_pTileSampler->Release();
	if (g_pTileTexRV) g_pTileTexRV->Release();
	if (g_pBoxSampler) g_pBoxSampler->Release();
	if (g_pBoxTextureRV) g_pBoxTextureRV->Release();
    if( g_pConstantBuffer ) g_pConstantBuffer->Release();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pIndexBuffer ) g_pIndexBuffer->Release();
	if (g_pVertexBuffer2) g_pVertexBuffer2->Release();
	if (g_pIndexBuffer2) g_pIndexBuffer2->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pSphereVertex ) g_pSphereVertex->Release();
	if (g_pSphereVertex2) g_pSphereVertex2->Release();
	if( g_pCubeVertex ) g_pCubeVertex->Release();
	if (g_pCubeVertex2) g_pCubeVertex2->Release();
	if (g_pDisplacementVertex) g_pDisplacementVertex->Release();
    if( g_pLightingPixel ) g_pLightingPixel->Release();
	if (g_pCubemapPixel) g_pCubemapPixel->Release();
	if (g_pTransparentPixel) g_pTransparentPixel->Release();
	if (g_pInkPixel) g_pInkPixel->Release();
	if (g_pBumpPixel) g_pBumpPixel->Release();
	if (g_pDisplacementPixel) g_pDisplacementPixel->Release();
	if (g_pRasterStateBox) g_pRasterStateBox->Release();
	if (g_pRasterStateObjects) g_pRasterStateObjects->Release();
	if (g_pBlendDesc) g_pBlendDesc->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain1 ) g_pSwapChain1->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext1 ) g_pImmediateContext1->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice1 ) g_pd3dDevice1->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
	
}

// Called every time the application receives a message
LRESULT CALLBACK WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

#pragma region Input/Camera Movement
void DetectInput(const float time)
{
	const float speed = 1.0f * time;

	//Keybinds F1
	if (GetAsyncKeyState(VK_F1))
		g_View = XMMatrixLookAtLH(g_Eye, g_At, g_Up);

	//Keybinds F2
	if (GetAsyncKeyState(VK_F2))
		g_View = XMMatrixLookAtLH(g_Eye2, g_At, g_Up2);

	//Keybinds Left Arrow
	if (GetAsyncKeyState(VK_LEFT) && GetAsyncKeyState(VK_CONTROL))
		g_View *= XMMatrixTranslation(5.0f * speed, 0.0f,0.0f);
	else if (GetAsyncKeyState(VK_LEFT))
		g_View *= XMMatrixRotationY(1.0f * speed);

	//Keybinds Right Arrow
	if (GetAsyncKeyState(VK_RIGHT) && GetAsyncKeyState(VK_CONTROL))
	g_View *= XMMatrixTranslation(-5.0f * speed, 0.0f, 0.0f);
	else if (GetAsyncKeyState(VK_RIGHT))
		g_View *= XMMatrixRotationY(-1.0f * speed);

	//Keybinds Up Arrow
	if (GetAsyncKeyState(VK_UP) && GetAsyncKeyState(VK_CONTROL))
		g_View *= XMMatrixTranslation(0.0f, 0.0f, -5.0f * speed);
	else if (GetAsyncKeyState(VK_UP))
		g_View *= XMMatrixRotationX(1.0f * speed);

	//Keybinds Down Arrow
	if (GetAsyncKeyState(VK_DOWN) && GetAsyncKeyState(VK_CONTROL))
		g_View *= XMMatrixTranslation(0.0f, 0.0f, 5.0f * speed);
	else if (GetAsyncKeyState(VK_DOWN))
		g_View *= XMMatrixRotationX(-1.0f * speed);

	//Keybinds Page-Up
	if (GetAsyncKeyState(VK_PRIOR) && GetAsyncKeyState(VK_CONTROL))
		g_View *= XMMatrixTranslation(0.0f, -5.0f * speed, 0.0f);

	//Keybinds Page-Down
	if (GetAsyncKeyState(VK_NEXT) && GetAsyncKeyState(VK_CONTROL))
		g_View *= XMMatrixTranslation(0.0f, 5.0f * speed, 0.0f);		
}
#pragma endregion

// Render a frame
void Render()
{
    // Update our time
    static float t = 0.0f;
    if( g_driverType == D3D_DRIVER_TYPE_REFERENCE )
    {
        t += XM_PI * 0.0125f;
    }
    else
    {
        static ULONGLONG timeStart = 0;
		const ULONGLONG timeCur = GetTickCount64();

        t = ( timeCur - timeStart ) / 1000.0f;
            timeStart = timeCur;
    }

	DetectInput(t);

	// Clear the back buffer
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, Colors::MidnightBlue );

	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	
	const UINT stride = sizeof(SimpleVertex);
	const UINT offset = 0;
	float temp[4] = { 1.0f,1.0f,1.0f,1.0f };

	ConstantBuffer cb;
	cb.mView = XMMatrixTranspose(g_View);
	cb.mProjection = XMMatrixTranspose(g_Projection);
	cb.vLightPos = XMLoadFloat4(&g_light.LightPos);
	cb.vLightCol = XMLoadFloat4(&g_light.LightCol);
	cb.vLightAmb = XMLoadFloat4(&g_light.LightAmbient);
	cb.vLightDiff = XMLoadFloat4(&g_light.LightDiffuse);
	cb.vEye = g_Eye;

#pragma region Main Box
	
	//Draws Cube
	XMFLOAT4 pos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	XMFLOAT4 scale = XMFLOAT4(10.0f, 10.0f, 10.0f, 1.0f);
	XMFLOAT4 rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

	XMMATRIX posMat = XMMatrixTranslation(pos.x, pos.y, pos.z);
	XMMATRIX scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX xRotMat = XMMatrixRotationX(rotation.x);
	XMMATRIX yRotMat = XMMatrixRotationY(rotation.y);
	XMMATRIX zRotMat = XMMatrixRotationZ(rotation.z);
	XMMATRIX rotMat = xRotMat * yRotMat * zRotMat;

	XMMATRIX world = scaleMat * rotMat * posMat;
	cb.mWorld = XMMatrixTranspose(world);
	
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	if (GetAsyncKeyState(VK_F6))
		g_pImmediateContext->VSSetShader(g_pCubeVertex2, nullptr, 0);
	else 
		g_pImmediateContext->VSSetShader(g_pCubeVertex, nullptr, 0);

	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);

	g_pImmediateContext->PSSetShader(g_pCubemapPixel, nullptr, 0);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	g_pImmediateContext->OMSetDepthStencilState(g_pDepthStencilStateBox, 1);
	g_pImmediateContext->RSSetState(g_pRasterStateBox);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pBoxSampler);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pBoxTextureRV);
	g_pImmediateContext->DrawIndexed(36, 0, 0); // 36 vertices needed for 12 triangles in a triangle list
#pragma endregion

#pragma region Sphere 1
	//Draws Sphere
	pos = XMFLOAT4(2.0f, -5.0f, 7.0f, 0.0f);
	scale = XMFLOAT4(0.15f, 0.15f, 0.15f, 0.0f);

	posMat = XMMatrixTranslation(pos.x, pos.y, pos.z);
	scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);

	world = scaleMat * rotMat * posMat;
	cb.mWorld = XMMatrixTranspose(world);

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer2, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer2, DXGI_FORMAT_R16_UINT, 0);

	g_pImmediateContext->VSSetShader(g_pSphereVertex, nullptr, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);

	g_pImmediateContext->PSSetShader(g_pLightingPixel, nullptr, 0);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	g_pImmediateContext->OMSetDepthStencilState(g_pDepthStencilStateObjects, 1);
	g_pImmediateContext->RSSetState(g_pRasterStateObjects);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pTileSampler);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTileTexRV);
	g_pImmediateContext->DrawIndexed(static_cast<UINT>(nIndices), 0, 0);
#pragma endregion

#pragma region Sphere 2
	//Draws 2nd Sphere
	pos = XMFLOAT4(2.0f, -5.0f, -7.0f, 0.0f);
	scale = XMFLOAT4(0.15f, 0.15f, 0.15f, 0.0f);
	posMat = XMMatrixTranslation(pos.x, pos.y, pos.z);
	scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);
	world = scaleMat * rotMat * posMat;
	cb.mWorld = XMMatrixTranspose(world);

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer2, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer2, DXGI_FORMAT_R16_UINT, 0);

	g_pImmediateContext->VSSetShader(g_pSphereVertex, nullptr, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	
	g_pImmediateContext->PSSetShader(g_pBumpPixel, nullptr, 0);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pStonesSampler);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pStonesTextureRV);

	g_pImmediateContext->PSSetShaderResources(1, 1, &g_pStonesNormalRV);
	g_pImmediateContext->OMSetDepthStencilState(g_pDepthStencilStateObjects, 1);
	g_pImmediateContext->RSSetState(g_pRasterStateObjects);
	g_pImmediateContext->DrawIndexed(static_cast<UINT>(nIndices), 0, 0);
#pragma endregion

#pragma region Ink
	pos = XMFLOAT4(0.0f, -10.0f, 0.0f, 0.0f);
	scale = XMFLOAT4(10.0f, 0.0f, 10.0f, 0.0f);
	if (GetAsyncKeyState(0x46) && GetAsyncKeyState(VK_SHIFT))
		posMat = XMMatrixTranslation(pos.x, (-5.0f * t), pos.z);
	else
		posMat = XMMatrixTranslation(pos.x, pos.y, pos.z);
	scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);
	world = scaleMat * rotMat * posMat;
	cb.mWorld = XMMatrixTranspose(world);

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	g_pImmediateContext->VSSetShader(g_pCubeVertex, nullptr, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);

	g_pImmediateContext->PSSetShader(g_pInkPixel, nullptr, 0);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	g_pImmediateContext->OMSetBlendState(g_pBlendDesc, temp, 0xffffffff);
	g_pImmediateContext->OMSetDepthStencilState(g_pDepthStencilStateObjects, 1);
	g_pImmediateContext->RSSetState(g_pRasterStateObjects);
	g_pImmediateContext->DrawIndexed(36, 0, 0);

	g_pImmediateContext->OMSetBlendState(g_pNoBlendDesc, temp, 0xffffffff);
#pragma endregion

#pragma region Cube 1
	//Draws Cube
	pos = XMFLOAT4(-2.0f, -5.0f, 0.0f, 0.0f);
	scale = XMFLOAT4(2.5f, 2.5f, 2.5f, 0.0f);
	posMat = XMMatrixTranslation(pos.x, pos.y, pos.z);
	scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);
	world = scaleMat * rotMat * posMat;
	cb.mWorld = XMMatrixTranspose(world);

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	g_pImmediateContext->VSSetShader(g_pCubeVertex, nullptr, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);

	g_pImmediateContext->PSSetShader(g_pTransparentPixel, nullptr, 0);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	g_pImmediateContext->OMSetBlendState(g_pBlendDesc, temp, 0xffffffff);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pBoxSampler);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pBoxTextureRV);
	g_pImmediateContext->OMSetDepthStencilState(g_pDepthStencilStateObjects, 1);
	g_pImmediateContext->RSSetState(g_pRasterStateObjects);
	g_pImmediateContext->DrawIndexed(36, 0, 0);

	g_pImmediateContext->OMSetBlendState(g_pNoBlendDesc, temp, 0xffffffff);
	
	#pragma endregion

    // Present our back buffer to our front buffer
    g_pSwapChain->Present( 0, 0 );
}