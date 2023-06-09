#include <winsock2.h>	
#include <stdio.h>
#include <windows.h>
#include <time.h>
// 告诉连接器与WS2_32库连接
#pragma comment(lib,"WS2_32.lib")
#define BUF_SIZE 1024
#define NAME_SIZE 30
DWORD WINAPI send_msg(LPVOID lpParam);
DWORD WINAPI recv_msg(LPVOID lpParam);
void error_handling(const char* msg);
char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];
int main()
{
	HANDLE hThread[2];
	DWORD dwThreadId;
	// 初始化WS2_32.dll
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	WSAStartup(sockVersion, &wsaData);
	/*设置登录用户名*/
	printf("你的聊天名字:");
	scanf("%s", name);
	getchar();	//接收换行符
	// 创建套节字
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
		error_handling("socket()出错");

	// 填写远程地址信息
	sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(8888);
	// 如果你的计算机没有联网，直接使用本地地址127.0.0.1
	servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

	if (connect(sock, (sockaddr*)&servAddr, sizeof(servAddr)) == -1)
		error_handling("connect()出错");
	printf("创建连接成功\n");
	hThread[0] = CreateThread(
		NULL,		// 默认安全属性
		NULL,		// 默认堆栈大小
		send_msg,	// 线程入口地址（执行线程的函数）
		&sock,		// 传给函数的参数
		0,		// 指定线程立即运行
		&dwThreadId);	// 返回线程的ID号
	hThread[1] = CreateThread(
		NULL,		// 默认安全属性
		NULL,		// 默认堆栈大小
		recv_msg,	// 线程入口地址（执行线程的函数）
		&sock,		// 传给函数的参数
		0,		// 指定线程立即运行
		&dwThreadId);	// 返回线程的ID号

	// 等待线程运行结束
	WaitForMultipleObjects(2, hThread, true, INFINITE);
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);
	printf(" 结束进程\n");
	getchar();
	// 关闭套节字
	closesocket(sock);
	// 释放WS2_32库
	WSACleanup();
	return 0;
}

DWORD WINAPI send_msg(LPVOID lpParam)    //接收信息进程
{

	char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    time_t timep;
    struct tm *t;
    time(&timep);
	t = localtime(&timep);
	int sock = *((int*)lpParam);  //将参数转为int类型
	char name_msg[NAME_SIZE + BUF_SIZE];
	while (1)
	{
		fgets(msg, BUF_SIZE, stdin);  //stdin表示标准输入流
		if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
		{
			closesocket(sock);  //关闭sock
			exit(0);
		}
		sprintf(name_msg, "%d/%d/%d 星期%d %d:%d:%d\n [%s]: %s", (1900+t->tm_year), (1+t->tm_mon),( t->tm_mday),t->tm_wday, t->tm_hour, t->tm_min, t->tm_sec,name, msg);  //将时间，名字与数据放入name_msg中
		int nRecv = send(sock, name_msg, strlen(name_msg), 0);  //接收信息
	}
	return NULL;
}

DWORD WINAPI recv_msg(LPVOID lpParam)    //发送信息进程
{
	int sock = *((int*)lpParam);  //将参数转为int类型
	char name_msg[NAME_SIZE + BUF_SIZE];
	int str_len;
	while (1)
	{
		
		str_len = recv(sock, name_msg, NAME_SIZE + BUF_SIZE - 1, 0);  //发送数据
		
		if (str_len == -1)  //健康性检查
			return -1;
		name_msg[str_len] = 0;
		fputs(name_msg, stdout);  //name_msg表示存放数组首地址；stdout表示默认文本指针可直接使用，具体不管
	}
	return NULL;
}


void error_handling(const char* msg)  //函数出错时执行此函数
{
	printf("%s\n", msg);
	WSACleanup();
	exit(1);
}
  /*实现显示当前时间函数
{
	char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    time_t timep;
    struct tm *t;
    time(&timep);
    t = localtime(&timep); //取得当地时间
    printf ("%d/%d/%d ", (1900+t->tm_year), (1+t->tm_mon), t->tm_mday);
    printf("星期%d %d:%d:%d\n", t->tm_wday, t->tm_hour, t->tm_min, t->tm_sec);
}
*/
