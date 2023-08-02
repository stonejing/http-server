#include <windows.h>
#include <shellapi.h>

#define ID_TRAY_APP_ICON    5000
#define ID_TRAY_EXIT        32771

// Window Proc for handling system tray messages
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Handle tray messages
    switch (message)
    {
        case WM_CREATE:
        {
            // Create system tray icon
            NOTIFYICONDATA tray_icon_data;
            tray_icon_data.cbSize = sizeof(NOTIFYICONDATA);
            tray_icon_data.hWnd = hWnd;
            tray_icon_data.uID = ID_TRAY_APP_ICON;
            tray_icon_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            tray_icon_data.uCallbackMessage = WM_USER;
            tray_icon_data.hIcon = (HICON)LoadImage(NULL, "icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
            tray_icon_data.szTip[0] = '\0';
            Shell_NotifyIcon(NIM_ADD, &tray_icon_data);

            // Create system tray menu
            HMENU tray_menu = CreatePopupMenu();
            AppendMenu(tray_menu, MF_STRING, ID_TRAY_EXIT, "Exit");
            SetForegroundWindow(hWnd);
            TrackPopupMenu(tray_menu, TPM_BOTTOMALIGN, 0, 0, 0, hWnd, NULL);
            DestroyMenu(tray_menu);

            break;
        }

        case WM_USER:
        {
            // Handle tray icon messages
            switch (lParam)
            {
                case WM_RBUTTONUP:
                {
                    // Show system tray menu on right-click
                    HMENU tray_menu = CreatePopupMenu();
                    AppendMenu(tray_menu, MF_STRING, ID_TRAY_EXIT, "Exit");
                    SetForegroundWindow(hWnd);
                    TrackPopupMenu(tray_menu, TPM_BOTTOMALIGN, 0, 0, 0, hWnd, NULL);
                    DestroyMenu(tray_menu);

                    break;
                }

                case WM_LBUTTONDBLCLK:
                {
                    // Show a message on double-click
                    MessageBox(NULL, "Hello World!", "System Tray", MB_OK);
                    break;
                }
            }

            break;
        }

        case WM_COMMAND:
        {
            // Handle system tray menu commands
            switch (wParam)
            {
                case ID_TRAY_EXIT:
                    // Exit the program
                    PostQuitMessage(0);
                    break;
            }

            break;
        }

        case WM_DESTROY:
        {
            // Remove system tray icon
            NOTIFYICONDATA tray_icon_data;
            tray_icon_data.cbSize = sizeof(NOTIFYICONDATA);
            tray_icon_data.hWnd = hWnd;
            tray_icon_data.uID = ID_TRAY_APP_ICON;
            Shell_NotifyIcon(NIM_DELETE, &tray_icon_data);

            break;
        }

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

// Win32 main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX window_class;
    ZeroMemory(&window_class, sizeof(WNDCLASSEX));
    window_class.cbSize = sizeof(WNDCLASSEX);
    window_class.hbrBackground = (HBRUSH)COLOR_WINDOW;
    window_class.hInstance = hInstance;
    window_class.lpfnWndProc = WindowProc;
    window_class.lpszClassName = "WindowClass";
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassEx(&window_class);

    // Create hidden window
    CreateWindowEx(0, "WindowClass", "", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    // Message loop
    MSG message;
    while (GetMessage(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return 0;
}
