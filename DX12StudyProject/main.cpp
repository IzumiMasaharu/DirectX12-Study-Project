#include "MyApp.h"

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR IpCmdLine, _In_ int nShowCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    try
    {
        MyApp MyDXProject(hInstance);
        if (!MyDXProject.Init())
            return 0;

        return MyDXProject.Run();
    }
    catch (DxException& error)
    {
        MessageBox(nullptr, error.ErrorMessageString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}