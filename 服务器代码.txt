#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <string.h>
#include <time.h>

#define MAX_CLNT 256
#define BUF_SIZE 1024

#pragma comment(lib, "WS2_32.lib")

void error_handling(const char* msg);
DWORD WINAPI ThreadProc(LPVOID lpParam);
DWORD WINAPI DisplayThread(LPVOID lpParam);
void send_msg(char* msg, int len);
void cleanup_client(SOCKET clnt_sock);
void save_chat_log(int length, char* msg);
void show_menu();
void flush_input_buffer();
DWORD WINAPI InputThreadFunc(LPVOID lpParam);
void server_group_chat(const char* msg);

HANDLE g_hEvent;
HANDLE g_hInputThread;
HANDLE g_hInputEvent;
int clnt_cnt = 0;
SOCKET clnt_socks[MAX_CLNT];
int terminateFlag[MAX_CLNT] = { 0 };
int in_group_chat = 0;  // 标记服务器是否进入群聊模式
HANDLE hThread[MAX_CLNT];
int g_InputFlag = 0;
int clientSequence[MAX_CLNT];

int main()
{
    WSADATA wsaData;
    WORD sockVersion = MAKEWORD(2, 2);
    WSAStartup(sockVersion, &wsaData);

    g_hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    g_hInputEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

    SOCKET serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock == INVALID_SOCKET)
        error_handling("Failed socket()");

    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(8888);
    sin.sin_addr.s_addr = INADDR_ANY;

    if (bind(serv_sock, (SOCKADDR*)&sin, sizeof(sin)) == SOCKET_ERROR)
        error_handling("Failed bind()");

    if (listen(serv_sock, 256) == SOCKET_ERROR)
        error_handling("Failed listen()");

    printf("开始监听:\n");

    sockaddr_in remoteAddr;
    int nAddrLen = sizeof(remoteAddr);
    DWORD dwThreadId;
    SOCKET clnt_sock;

    hThread[clnt_cnt] = CreateThread(NULL, NULL, DisplayThread, NULL, 0, &dwThreadId);

    while (TRUE)
    {
        printf("等待新连接\n");
        clnt_sock = accept(serv_sock, (SOCKADDR*)&remoteAddr, &nAddrLen);
        if (clnt_sock == INVALID_SOCKET)
        {
            printf("Failed accept()");
            continue;
        }

        WaitForSingleObject(g_hEvent, INFINITE);
        hThread[clnt_cnt] = CreateThread(NULL, NULL, ThreadProc, (LPVOID)clnt_sock, 0, &dwThreadId);
        clnt_socks[clnt_cnt] = clnt_sock;
        clientSequence[clnt_sock] = clnt_cnt;
        clnt_cnt++;
        SetEvent(g_hEvent);

        printf("接受到一个连接：%s 执行线程ID：%d 他的序号为%d\r\n", inet_ntoa(remoteAddr.sin_addr), dwThreadId, clnt_cnt);
    }

    WaitForMultipleObjects(clnt_cnt, hThread, TRUE, INFINITE);

    for (int i = 0; i < clnt_cnt; i++)
    {
        CloseHandle(hThread[i]);
    }

    closesocket(serv_sock);
    WSACleanup();
    return 0;
}

// 错误处理函数
void error_handling(const char* msg)
{
    printf("%s\n", msg);
    WSACleanup();
    exit(1);
}

// 子线程函数，处理客户端消息
DWORD WINAPI ThreadProc(LPVOID lpParam)
{
    SOCKET clnt_sock = (SOCKET)lpParam;
    int str_len = 0;
    char msg[BUF_SIZE];

    while (!terminateFlag[clnt_sock])
    {
        if (terminateFlag[clnt_sock])
            break;

        str_len = recv(clnt_sock, msg, sizeof(msg), 0);
        if (str_len == -1)
            break;

        send_msg(msg, str_len);
        printf("收到线程(%d)信息：", GetCurrentThreadId());
        for (int j = 0; j < str_len; ++j)
        {
            printf("%c", msg[j]);
        }
        printf("\n");
        save_chat_log(str_len, msg);
    }

    printf("客户端退出：%d\n", GetCurrentThreadId());

    cleanup_client(clnt_sock);

    return 0;
}

// 发送消息给所有客户端
void send_msg(char* msg, int len)
{
    WaitForSingleObject(g_hEvent, INFINITE);
    for (int i = 0; i < clnt_cnt; i++)
    {
        send(clnt_socks[i], msg, len, 0);
    }
    SetEvent(g_hEvent);
}

// 清理客户端连接
void cleanup_client(SOCKET clnt_sock)
{
    closesocket(clnt_sock);

    WaitForSingleObject(hThread[clientSequence[clnt_sock]], INFINITE);
    CloseHandle(hThread[clientSequence[clnt_sock]]);

    // 将关闭的客户端从数组中删除
    for (int i = clientSequence[clnt_sock]; i < clnt_cnt - 1; i++)
    {
        clnt_socks[i] = clnt_socks[i + 1];
        hThread[i] = hThread[i + 1];
        clientSequence[clnt_socks[i]] = i; // 更新客户端序号
    }
    clnt_cnt--;
}

// 保存聊天记录到文件
void save_chat_log(int length, char* msg)
{
    FILE* fp;
    if ((fp = fopen("D:\\聊天记录.txt", "a")) == NULL)
    {
        printf("聊天记录文件打不开，请在D盘根创建名为“聊天记录”的文本");
        exit(0);
    }
    fwrite(msg, 1, length, fp);
    fclose(fp);
}

// 显示菜单选项
DWORD WINAPI DisplayThread(LPVOID lpParam)
{
    while (TRUE)
    {
        show_menu();
    }

    return 0;
}

// 显示菜单选项并处理用户输入
void show_menu()
{
    if (in_group_chat)
    {
        printf("输入要发送的消息：");
        char msg[BUF_SIZE];
        fgets(msg, BUF_SIZE, stdin);
        int len = strlen(msg);
        msg[len - 1] = '\0';

        if (strcmp(msg, "q") == 0)
        {
            in_group_chat = 0;
            printf("退出群聊模式\n");
        }
        else
        {
            char server_msg[BUF_SIZE + 20];
            sprintf(server_msg, "来自服务器：%s\n", msg);  // 在消息前加上"来自服务器"
            send_msg(server_msg, len + 19);
        }
    }
    else
    {
        printf("\n");
        printf("选择一个操作：\n");
        printf("1. 列出所有客户端\n");
        printf("2. 踢出客户端\n");
        printf("3. 进入群聊模式\n");
        printf("q. 退出服务器\n");
        printf("\n");

        char choice;
        scanf("%c", &choice);
        flush_input_buffer();

        switch (choice)
        {
        case '1':
            printf("当前客户端列表：\n");
            for (int i = 0; i < clnt_cnt; i++)
            {
                printf("%d. 客户端线程ID：%d\n", i + 1, GetThreadId(hThread[i]));
            }
            break;
        case '2':
            printf("输入要踢出的客户端序号：");
            int index;
            scanf("%d", &index);
            flush_input_buffer();

            if (index < 1 || index > clnt_cnt)
            {
                printf("无效的客户端序号\n");
            }
            else
            {
                index--;  // 调整为数组索引
                terminateFlag[clnt_socks[index]] = 1;
                printf("成功踢出客户端：%d\n", GetThreadId(hThread[index]));
            }
            break;
        case '3':
            in_group_chat = 1;
            printf("进入群聊模式\n");
            break;
        case 'q':
            for (int i = 0; i < clnt_cnt; i++)
            {
                terminateFlag[clnt_socks[i]] = 1;
            }
            printf("正在退出服务器...\n");
            Sleep(1000);  // 给其他线程足够时间处理退出请求
            exit(0);
        default:
            printf("无效的选择\n");
            break;
        }
    }
}

// 清空输入缓冲区
void flush_input_buffer()
{
    while (getchar() != '\n')
        continue;
}

// 输入线程函数
DWORD WINAPI InputThreadFunc(LPVOID lpParam)
{
    while (TRUE)
    {
        WaitForSingleObject(g_hInputEvent, INFINITE);
        g_InputFlag = 0;
    }

    return 0;
}

// 服务器群聊
void server_group_chat(const char* msg)
{
    send_msg((char*)msg, strlen(msg));
}

