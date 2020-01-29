#pragma once

#pragma region Global Variables

HINSTANCE                 g_hInst = nullptr;
HWND                      g_hWnd = nullptr;
D3D_DRIVER_TYPE           g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL         g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*             g_pd3dDevice = nullptr;
ID3D11Device1*            g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*      g_pImmediateContext = nullptr;
ID3D11DeviceContext1*     g_pImmediateContext1 = nullptr;
IDXGISwapChain*           g_pSwapChain = nullptr;
IDXGISwapChain1*          g_pSwapChain1 = nullptr;
ID3D11RenderTargetView*   g_pRenderTargetView = nullptr;
ID3D11Texture2D*		  g_pDepthStencil = nullptr;
ID3D11DepthStencilView*   g_pDepthStencilView = nullptr;
ID3D11VertexShader*       g_pSphereVertex = nullptr;
ID3D11VertexShader*       g_pSphereVertex2 = nullptr;
ID3D11VertexShader*       g_pCubeVertex = nullptr;
ID3D11VertexShader*       g_pCubeVertex2 = nullptr;
ID3D11VertexShader*       g_pDisplacementVertex = nullptr;
ID3D11PixelShader*        g_pLightingPixel = nullptr;
ID3D11PixelShader*        g_pCubemapPixel = nullptr;
ID3D11PixelShader*		  g_pTransparentPixel = nullptr;
ID3D11PixelShader*		  g_pBumpPixel = nullptr;
ID3D11PixelShader*		  g_pDisplacementPixel = nullptr;
ID3D11PixelShader*		  g_pInkPixel = nullptr;
ID3D11InputLayout*        g_pVertexLayout = nullptr;
ID3D11Buffer*             g_pVertexBuffer = nullptr;
ID3D11Buffer*             g_pVertexBuffer2 = nullptr;
ID3D11Buffer*             g_pIndexBuffer = nullptr;
ID3D11Buffer*             g_pIndexBuffer2 = nullptr;
ID3D11Buffer*             g_pConstantBuffer = nullptr;
ID3D11ShaderResourceView* g_pBoxTextureRV = nullptr;
ID3D11SamplerState*       g_pBoxSampler = nullptr;
ID3D11ShaderResourceView* g_pStonesNormalRV = nullptr;
ID3D11SamplerState*       g_pStonesNormalSampler = nullptr;
ID3D11ShaderResourceView* g_pStonesTextureRV = nullptr;
ID3D11SamplerState*       g_pStonesSampler = nullptr;
ID3D11ShaderResourceView* g_pTileTexRV = nullptr;
ID3D11SamplerState*       g_pTileSampler = nullptr;
ID3D11ShaderResourceView* g_pDispMapRV = nullptr;
ID3D11SamplerState*       g_pDispMapSampler = nullptr;
ID3D11DepthStencilState*  g_pDepthStencilStateBox = nullptr;
ID3D11DepthStencilState*  g_pDepthStencilStateObjects = nullptr;
ID3D11RasterizerState*    g_pRasterStateBox = nullptr;
ID3D11RasterizerState*    g_pRasterStateObjects = nullptr;
ID3D11BlendState*	      g_pBlendDesc = nullptr;
ID3D11BlendState*         g_pNoBlendDesc = nullptr;

XMMATRIX				  g_World;
XMMATRIX				  g_View;
XMMATRIX				  g_Projection;
Lighting                  g_light;
XMVECTOR				  g_Eye;
XMVECTOR				  g_Eye2;
XMVECTOR				  g_At;
XMVECTOR				  g_Up;
XMVECTOR				  g_Up2;
size_t nIndices;
#pragma endregion