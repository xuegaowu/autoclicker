#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

using namespace std::chrono;

// 控件ID
#define ID_CHECK_ENABLE     1001    // 保险开关
#define ID_RADIO_LEFT       1002    // 左键模式
#define ID_RADIO_RIGHT      1003    // 右键模式
#define ID_EDIT_DELAY       1004    // 间隔输入框
#define ID_BTN_HOTKEY       1005    // 启动键按钮
#define ID_STATUS_TEXT      1006    // 状态文字
#define ID_BTN_TEST         1007    // 测试按钮
#define ID_TEXT_TEST_COUNT  1008    // 测试计数显示
#define ID_BTN_WORKKEY      1009    // 配合键按钮
#define ID_CHECK_WORKKEY    1010    // 启用配合键复选框

// 全局变量
HINSTANCE hInst;
HWND hMainWnd;
HWND hCheckEnable;              // 保险开关
HWND hRadioLeft, hRadioRight;   // 左键/右键单选
HWND hEditDelay;                // 间隔输入框
HWND hBtnHotkey;                // 启动键按钮
HWND hStatusText;               // 状态文字
HWND hBtnTest;                  // 测试按钮
HWND hTextTestCount;            // 测试计数显示
HWND hBtnWorkkey;               // 配合键按钮
HWND hCheckWorkkey;             // 启用配合键复选框

// 连点器状态
std::atomic<bool> g_enabled{false};      // 总开关（默认关闭）
std::atomic<int> g_clickDelay{100};      // 点击间隔（毫秒）
std::atomic<int> g_clickButton{0};       // 0=左键, 1=右键
std::atomic<int> g_hotkey{VK_F2};        // 启动键（默认F2）
std::atomic<int> g_workkey{VK_LBUTTON};  // 配合键（默认左键）
std::atomic<bool> g_useWorkkey{false};   // 是否使用配合键
std::thread g_clickThread;
std::atomic<bool> g_threadRunning{true};

// 测试计数器（主线程使用，不需要原子）
int g_testCount = 0;

// 获取按键名称
std::string GetKeyName(int vk) {
    if (vk >= 'A' && vk <= 'Z') {
        return std::string(1, (char)vk);
    }
    switch(vk) {
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        case VK_RETURN: return "Enter";
        case VK_SPACE: return "Space";
        case VK_LBUTTON: return "左键";
        case VK_RBUTTON: return "右键";
        default: return "Key";
    }
}

// 模拟鼠标点击
void SimulateClick() {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    
    if (g_clickButton == 0) {
        // 左键
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        std::this_thread::sleep_for(milliseconds(5));
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    } else {
        // 右键
        input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        std::this_thread::sleep_for(milliseconds(5));
        input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        SendInput(1, &input, sizeof(INPUT));
    }
}

// 连点线程函数
void ClickThreadFunc() {
    while (g_threadRunning) {
        // 检测启动键是否按下
        bool hotkeyPressed = (GetAsyncKeyState(g_hotkey.load()) & 0x8000) != 0;
        
        bool triggerCondition = false;
        if (g_useWorkkey) {
            // 使用配合键：启动键 + 配合键
            bool workPressed = (GetAsyncKeyState(g_workkey.load()) & 0x8000) != 0;
            triggerCondition = hotkeyPressed && workPressed;
        } else {
            // 不使用配合键：只按启动键
            triggerCondition = hotkeyPressed;
        }
        
        if (g_enabled && triggerCondition) {
            SimulateClick();
            std::this_thread::sleep_for(milliseconds(g_clickDelay.load()));
        } else {
            std::this_thread::sleep_for(milliseconds(10));
        }
    }
}

// 更新状态显示
void UpdateStatusText() {
    std::string status;
    if (!g_enabled) {
        status = "状态: ● 已禁用（保险关闭）";
    } else {
        std::string keyName = GetKeyName(g_hotkey.load());
        if (g_useWorkkey) {
            std::string workName = GetKeyName(g_workkey.load());
            status = "状态: ○ 待机（按住 " + keyName + " + " + workName + " 启动连点）";
        } else {
            status = "状态: ○ 待机（按住 " + keyName + " 启动连点）";
        }
    }
    SetWindowTextA(hStatusText, status.c_str());
}

// 更新配合键按钮文字
void UpdateWorkkeyButton() {
    std::string workName = GetKeyName(g_workkey.load());
    std::string btnText = "配合键(" + workName + ")";
    SetWindowTextA(hBtnWorkkey, btnText.c_str());
}

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            // 创建保险开关（复选框）
            hCheckEnable = CreateWindowA("BUTTON", "保险开关",
                WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                20, 20, 100, 25, hWnd, (HMENU)ID_CHECK_ENABLE, hInst, NULL);
            SendMessage(hCheckEnable, BM_SETCHECK, BST_UNCHECKED, 0);
            
            // 创建分组框
            CreateWindowA("BUTTON", "点击模式",
                WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                20, 55, 180, 70, hWnd, NULL, hInst, NULL);
            
            // 左键单选
            hRadioLeft = CreateWindowA("BUTTON", "左键",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                30, 75, 60, 25, hWnd, (HMENU)ID_RADIO_LEFT, hInst, NULL);
            CheckRadioButton(hWnd, ID_RADIO_LEFT, ID_RADIO_RIGHT, ID_RADIO_LEFT);
            
            // 右键单选
            hRadioRight = CreateWindowA("BUTTON", "右键",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                110, 75, 60, 25, hWnd, (HMENU)ID_RADIO_RIGHT, hInst, NULL);
            
            // 频率设置
            CreateWindowA("STATIC", "间隔(毫秒):",
                WS_VISIBLE | WS_CHILD,
                20, 140, 70, 20, hWnd, NULL, hInst, NULL);
            
            hEditDelay = CreateWindowA("EDIT", "100",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                95, 138, 50, 22, hWnd, (HMENU)ID_EDIT_DELAY, hInst, NULL);
            
            // 启动键设置按钮
            hBtnHotkey = CreateWindowA("BUTTON", "启动键(F2)",
                WS_VISIBLE | WS_CHILD,
                160, 138, 100, 25, hWnd, (HMENU)ID_BTN_HOTKEY, hInst, NULL);
            
            // 启用配合键复选框
            hCheckWorkkey = CreateWindowA("BUTTON", "启用配合键",
                WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                20, 175, 90, 25, hWnd, (HMENU)ID_CHECK_WORKKEY, hInst, NULL);
            SendMessage(hCheckWorkkey, BM_SETCHECK, BST_UNCHECKED, 0);
            
            // 配合键设置按钮
            hBtnWorkkey = CreateWindowA("BUTTON", "配合键(左键)",
                WS_VISIBLE | WS_CHILD,
                120, 175, 100, 25, hWnd, (HMENU)ID_BTN_WORKKEY, hInst, NULL);
            // 默认禁用配合键按钮
            EnableWindow(hBtnWorkkey, FALSE);
            
            // 状态显示
            hStatusText = CreateWindowA("STATIC", "状态: ● 已禁用（保险关闭）",
                WS_VISIBLE | WS_CHILD,
                20, 215, 350, 20, hWnd, (HMENU)ID_STATUS_TEXT, hInst, NULL);
            
            // 测试按钮
            hBtnTest = CreateWindowA("BUTTON", "测试按钮",
                WS_VISIBLE | WS_CHILD,
                20, 245, 80, 30, hWnd, (HMENU)ID_BTN_TEST, hInst, NULL);
            
            // 测试计数显示
            hTextTestCount = CreateWindowA("STATIC", "点击次数: 0",
                WS_VISIBLE | WS_CHILD,
                110, 252, 100, 20, hWnd, (HMENU)ID_TEXT_TEST_COUNT, hInst, NULL);
            
            // 启动连点线程
            g_threadRunning = true;
            g_clickThread = std::thread(ClickThreadFunc);
            
            // 更新状态显示
            UpdateStatusText();
            break;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_CHECK_ENABLE: {
                    BOOL checked = (SendMessage(hCheckEnable, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    g_enabled = checked;
                    UpdateStatusText();
                    break;
                }
                
                case ID_RADIO_LEFT: {
                    g_clickButton = 0;
                    break;
                }
                
                case ID_RADIO_RIGHT: {
                    g_clickButton = 1;
                    break;
                }
                
                case ID_EDIT_DELAY: {
                    char buffer[32];
                    GetWindowTextA(hEditDelay, buffer, 32);
                    int delay = atoi(buffer);
                    if (delay >= 1 && delay <= 10000) {
                        g_clickDelay = delay;
                    } else {
                        sprintf(buffer, "%d", g_clickDelay.load());
                        SetWindowTextA(hEditDelay, buffer);
                    }
                    break;
                }
                
                case ID_BTN_HOTKEY: {
                    SetWindowTextA(hBtnHotkey, "按任意键...");
                    SetFocus(hWnd);
                    break;
                }
                
                case ID_CHECK_WORKKEY: {
                    BOOL checked = (SendMessage(hCheckWorkkey, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    g_useWorkkey = checked;
                    EnableWindow(hBtnWorkkey, checked ? TRUE : FALSE);
                    UpdateStatusText();
                    break;
                }
                
                case ID_BTN_WORKKEY: {
                    SetWindowTextA(hBtnWorkkey, "按任意键...");
                    SetFocus(hWnd);
                    break;
                }
                
                case ID_BTN_TEST: {
                    g_testCount++;
                    char countText[50];
                    sprintf(countText, "点击次数: %d", g_testCount);
                    SetWindowTextA(hTextTestCount, countText);
                    break;
                }
            }
            break;
        }
        
        case WM_KEYDOWN: {
            // 检查是否在等待设置启动键
            char btnText[50];
            GetWindowTextA(hBtnHotkey, btnText, 50);
            if (strcmp(btnText, "按任意键...") == 0) {
                g_hotkey = (int)wParam;
                std::string keyName = GetKeyName((int)wParam);
                std::string newText = "启动键(" + keyName + ")";
                SetWindowTextA(hBtnHotkey, newText.c_str());
                UpdateStatusText();
                break;
            }
            
            // 检查是否在等待设置配合键
            GetWindowTextA(hBtnWorkkey, btnText, 50);
            if (strcmp(btnText, "按任意键...") == 0) {
                g_workkey = (int)wParam;
                UpdateWorkkeyButton();
                UpdateStatusText();
                break;
            }
            break;
        }
        
        case WM_DESTROY: {
            g_threadRunning = false;
            if (g_clickThread.joinable()) {
                g_clickThread.join();
            }
            PostQuitMessage(0);
            break;
        }
        
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 注册窗口类
bool RegisterWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXA wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEXA);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = "AutoClickerWindow";
    
    return RegisterClassExA(&wcex) != 0;
}

// 程序入口
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    
    if (!RegisterWindowClass(hInstance)) {
        MessageBoxA(NULL, "窗口类注册失败！", "错误", MB_OK);
        return 1;
    }
    
    hMainWnd = CreateWindowA(
        "AutoClickerWindow",
        "自动连点器",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        420, 330,    // 窗口宽度420，高度330
        NULL, NULL, hInstance, NULL
    );
    
    if (!hMainWnd) {
        MessageBoxA(NULL, "窗口创建失败！", "错误", MB_OK);
        return 1;
    }
    
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}