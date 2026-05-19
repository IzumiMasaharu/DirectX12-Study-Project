#include "Mayohoshi.h"
#include "DemoScene.h"

#include <crtdbg.h>
#include <exception>

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR IpCmdLine, _In_ int nShowCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		Mayohoshi::Renderer renderer(hInstance);
		MayohoshiExamples::BuildDemoScene(renderer);

		if (!renderer.CreateRenderWindow(1280, 720, L"Mayohoshi Render"))
		{
			const std::wstring& message = renderer.GetLastErrorMessage();
			if (!message.empty())
				MessageBox(nullptr, message.c_str(), L"Mayohoshi Render", MB_OK | MB_ICONERROR);
			return 114;
		}

		return renderer.RunMessageLoop();
	}
	catch (const std::exception& error)
	{
		MessageBoxA(nullptr, error.what(), "Mayohoshi Render", MB_OK | MB_ICONERROR);
		return -1;
	}
}
