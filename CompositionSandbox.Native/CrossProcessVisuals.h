#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <windows.ui.composition.h>
#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>

#include <winrt/Windows.UI.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>

#include <dcomp.h>
#include <dxgi1_3.h>
#include <d3d11_2.h>

#include "dcomp.private.h"

namespace WU = winrt::Windows::UI;
namespace WF = winrt::Windows::Foundation;
namespace WUC = winrt::Windows::UI::Composition;
namespace AWUC = ABI::Windows::UI::Composition;
namespace WUCD = winrt::Windows::UI::Composition::Desktop;
namespace AWUCD = ABI::Windows::UI::Composition::Desktop;

#define CDSSHAREDHANDLE 0x123987

HINSTANCE hInst;
HANDLE handleWaitEvent;
HANDLE receivedHandle;
HANDLE parentProccess;

WUC::ContainerVisual rootVisual{ nullptr };

ATOM RegisterWindowClass(HINSTANCE hInstance, const wchar_t* className);
_Success_(return == TRUE) BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, const wchar_t* className, const wchar_t* title, _Out_ HWND* hwnd);
int RunMessageLoop();
winrt::Windows::System::DispatcherQueueController CreateDispatcherQueueController();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    const bool isSecondary = wcscmp(lpCmdLine, L"SecondaryProcess") == 0;

    const wchar_t* mainClassName = L"CompositionSandbox.Native.MainWindow";
    const wchar_t* secondaryClassName = L"CompositionSandbox.Native.SecondaryWindow";
    const wchar_t* className = isSecondary ? secondaryClassName : mainClassName;
    const wchar_t* windowTitle = isSecondary ? L"Secondary Window" : L"Main Window";

    RegisterWindowClass(hInstance, className);
    
    HWND hwnd;
    if (!InitInstance(hInstance, nCmdShow, className, windowTitle, &hwnd))
        return FALSE;

    if (isSecondary)
        handleWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    winrt::init_apartment(winrt::apartment_type::single_threaded);
    auto controller = CreateDispatcherQueueController();

    WUC::Compositor compositor;
    WUCD::DesktopWindowTarget target{ nullptr };

    auto interop = compositor.try_as<AWUCD::ICompositorDesktopInterop>();
    auto partner = compositor.try_as<ICompositorPartner>();

    if (!interop || !partner)
    {
        MessageBox(hwnd, L"Make sure you are running on 19041+ and if so please open an issue", L"Unsupported Windows build", NULL);
        ExitProcess(0);
    }

    interop->CreateDesktopWindowTarget(hwnd, true, reinterpret_cast<AWUCD::IDesktopWindowTarget**>(winrt::put_abi(target)));

    if (!isSecondary)
    {
        WUC::Visual visual{ nullptr };
        partner->CreateSharedVisual(reinterpret_cast<AWUC::IVisual**>(winrt::put_abi(visual)));

        HANDLE sharedHandle;
        partner->OpenSharedResourceHandle(reinterpret_cast<AWUC::ICompositionObject*>(winrt::get_abi(visual.as<WUC::CompositionObject>())), &sharedHandle);

        STARTUPINFOA cif;
        ZeroMemory(&cif, sizeof(cif));
        cif.cb = sizeof(cif);

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        std::string cmd = GetCommandLineA();
        cmd += " SecondaryProcess";
        CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &cif, &pi);

        target.Root(visual);
        visual.RelativeSizeAdjustment({ 1, 1 });

        HANDLE secondaryHandle;
        DuplicateHandle(GetCurrentProcess(), sharedHandle, pi.hProcess, &secondaryHandle, NULL, FALSE, DUPLICATE_SAME_ACCESS);

        COPYDATASTRUCT CDS;
        CDS.cbData = sizeof(HANDLE);
        CDS.dwData = CDSSHAREDHANDLE;
        CDS.lpData = &secondaryHandle;

        Sleep(500); // Enough time for the secondary process to spawn its window

        HWND secondaryHwnd = FindWindowW(secondaryClassName, NULL);
        SendMessage(secondaryHwnd, WM_COPYDATA, (WPARAM)(HWND)hwnd, (LPARAM)(LPVOID)&CDS);

        auto retVal = RunMessageLoop();

        CloseHandle(pi.hThread);
        TerminateProcess(pi.hProcess, 0);

        CloseHandle(sharedHandle);
        CloseHandle(secondaryHandle);

        return retVal;
    }
    else
    {
        winrt::com_ptr<IVisualTargetPartner> visualTarget;

        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (handleWaitEvent && WaitForSingleObjectEx(handleWaitEvent, 0, true) != WAIT_TIMEOUT)
            {
                CloseHandle(handleWaitEvent);
                handleWaitEvent = NULL;
                
                partner->OpenSharedTargetFromHandle(receivedHandle, visualTarget.put());

                rootVisual = compositor.CreateContainerVisual();

                WUC::SpriteVisual backgroundVisual = compositor.CreateSpriteVisual();
                backgroundVisual.Brush(compositor.CreateColorBrush(WU::Colors::MediumPurple()));
                backgroundVisual.RelativeSizeAdjustment({ 1, 1 });
                rootVisual.Children().InsertAtBottom(backgroundVisual);

                WUC::SpriteVisual boxVisual = compositor.CreateSpriteVisual();
                boxVisual.Brush(compositor.CreateColorBrush(WU::Colors::White()));
                boxVisual.Size({ 50, 50 });
                boxVisual.AnchorPoint({ 0.5f, 0.5f });
                boxVisual.RelativeOffsetAdjustment({ 0.1f, 0.5f, 0 });
                rootVisual.Children().InsertAtTop(boxVisual);

                WUC::Vector3KeyFrameAnimation anim = compositor.CreateVector3KeyFrameAnimation();
                anim.InsertKeyFrame(0, { 0.1f, 0.5f, 0 });
                anim.InsertKeyFrame(0.5f, { 0.9f, 0.5f, 0 });
                anim.InsertKeyFrame(1, { 0.1f, 0.5f, 0 });
                anim.IterationBehavior(WUC::AnimationIterationBehavior::Forever);
                anim.Duration(std::chrono::seconds(2));
                boxVisual.StartAnimation(L"RelativeOffsetAdjustment", anim);

                target.Root(rootVisual);

                WUC::SpriteVisual redirectVisual = compositor.CreateSpriteVisual();
                WUC::CompositionVisualSurface visualSurface = compositor.CreateVisualSurface();
                WUC::CompositionSurfaceBrush surfaceBrush = compositor.CreateSurfaceBrush(visualSurface);

                visualSurface.SourceVisual(rootVisual);
                surfaceBrush.Stretch(WUC::CompositionStretch::None);
                redirectVisual.Brush(surfaceBrush);

                RECT wrct;
                GetClientRect(hwnd, &wrct);
                rootVisual.Size({ (float)(wrct.right - wrct.left), (float)(wrct.bottom - wrct.top) });

                WUC::ExpressionAnimation sizeAnim = compositor.CreateExpressionAnimation(L"rootVisual.Size");
                sizeAnim.SetReferenceParameter(L"rootVisual", rootVisual);

                redirectVisual.StartAnimation(L"Size", sizeAnim);
                visualSurface.StartAnimation(L"SourceSize", sizeAnim);

                visualTarget->SetRoot(reinterpret_cast<AWUC::IVisual*>(winrt::get_abi(redirectVisual)));
            }
        }

        CloseHandle(receivedHandle);

        if (parentProccess)
            TerminateProcess(parentProccess, 0);

        return (int)msg.wParam;
    }
}

inline int RunMessageLoop()
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

winrt::Windows::System::DispatcherQueueController CreateDispatcherQueueController()
{
    DispatcherQueueOptions options
    {
        sizeof(DispatcherQueueOptions),
        DQTYPE_THREAD_CURRENT,
        DQTAT_COM_STA
    };

    winrt::Windows::System::DispatcherQueueController controller{ nullptr };
    winrt::check_hresult(CreateDispatcherQueueController(options, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(winrt::put_abi(controller))));
    return controller;
}

ATOM RegisterWindowClass(HINSTANCE hInstance, const wchar_t* className)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName  = className;
    wcex.lpszMenuName   = NULL;
    wcex.hIconSm        = LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);

    return RegisterClassExW(&wcex);
}

_Success_(return == TRUE)
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, const wchar_t* className, const wchar_t* title, _Out_ HWND* hwnd)
{
   hInst = hInstance;
   HWND hWnd = CreateWindowEx(WS_EX_NOREDIRECTIONBITMAP, className, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
       return FALSE;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   *hwnd = hWnd;

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COPYDATA:
    {
        auto CDS = (PCOPYDATASTRUCT)lParam;
        if (CDS->dwData == CDSSHAREDHANDLE)
        {
            receivedHandle = *(HANDLE*)CDS->lpData;
            SetEvent(handleWaitEvent);

            DWORD pid;
            GetWindowThreadProcessId((HWND)wParam, &pid);
            parentProccess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        }
        break;
    }
    case WM_SIZE:
    {
        if (rootVisual && wParam != SIZE_MINIMIZED)
        {
            float width = (float)LOWORD(lParam);
            float height = (float)HIWORD(lParam);

            rootVisual.Size({ width, height });
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}