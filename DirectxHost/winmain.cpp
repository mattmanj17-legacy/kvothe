#include <windows.h>    // include the basic windows header file

#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>

#include "d3dx12.h"

#include <wrl.h>

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
	UINT dxgiFactoryFlags = 0;

	ComPtr<IDXGIFactory4> factory;
	CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
	
	// create a "Hello World" message box using MessageBox()
	MessageBox(NULL,
		L"Hello World!",
		L"Just another Hello World program!",
		MB_ICONEXCLAMATION | MB_OK);

	// return 0 to Windows
	return 0;
}