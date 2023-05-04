#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h> //Winsock ������� include. WSADATA �������.
#include <WS2tcpip.h>
#include <sstream>
#include <thread>
#include <conio.h>
#include <vector>
#include <iostream>
#include <string>

#define MAX_SIZE 1024
using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;

SOCKET client_sock;
string my_id;

int chat_recv() {
    char buf[MAX_SIZE] = { };
    string msg;
    while (1) {
        ZeroMemory(&buf, MAX_SIZE);
        if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
            msg = buf;
            std::stringstream ss(msg);  // ���ڿ��� ��Ʈ��ȭ
            string user;
            ss >> user; // ��Ʈ���� ����, ���ڿ��� ���� �и��� ������ �Ҵ�
            if (user != my_id) cout << buf << endl; // ���� ���� �� �ƴ� ��쿡�� ����ϵ���.
        }
        else {
            cout << "Server Off" << endl;
            return -1;
        }
    }
}

//Ű���� �Է�
string _input(boolean isPassword) {
    string res = "";

    while (true) {
        char ch = _getch();

        if (ch == 13) { //\n
            if (res.length() <= 0) continue;
            break;
        }

        // ����� �������̽�
        if (ch == 8) {
            if (res.length() == 0) {
                continue;
            }
            res.pop_back();

            // Ŀ�� �ڷΰ���
            cout << "\b \b";
        }
        else {
            if (res.length() > 50) {
                cout << "\b \b";
                res.pop_back();
                
                if (isPassword) std::cout << '*'; //��ȣ�� *�� ǥ��
                else cout << ch;
            }
            else {
                if (isPassword) std::cout << '*';
                else cout << ch;
            }

            res.push_back(ch);
        }
    }

    cout << endl;
    return res;
}

//���̵� Ȯ�� �� ��й�ȣ �Է�, ��ȯ ���� false�� ���α׷� ����
bool login() {
    string input_user_id;
    cout << "\n50�� ���Ϸ� �Է��ϼ���.\n";
    cout << "���̵� �Է����ּ��� : ";

    input_user_id = _input(false);

    //server�� id�� ������ recv()�� id�� password �ޱ�
    send(client_sock, input_user_id.c_str(), input_user_id.length(), 0);

    string input_user_pw;

    cout << "��й�ȣ�� �Է����ּ��� : ";
    input_user_pw = _input(true);

    char buf[MAX_SIZE] = { };
    string msg;
    //while (1) {
    //    zeromemory(&buf, max_size);
    //    if (recv(client_sock, buf, max_size, 0) > 0) {
    //        msg = buf;
    //        std::stringstream ss(msg);  // ���ڿ��� ��Ʈ��ȭ
    //        string user;
    //        ss >> user; // ��Ʈ���� ����, ���ڿ��� ���� �и��� ������ �Ҵ�

    //    }
    //    else {
    //        cout << "server off" << endl;
    //        return false;
    //    }
    //}
}

int main()
{
    WSADATA wsa;

    // Winsock�� �ʱ�ȭ�ϴ� �Լ�. MAKEWORD(2, 2)�� Winsock�� 2.2 ������ ����ϰڴٴ� �ǹ�.
    // ���࿡ �����ϸ� 0��, �����ϸ� �� �̿��� ���� ��ȯ.
    // 0�� ��ȯ�ߴٴ� ���� Winsock�� ����� �غ� �Ǿ��ٴ� �ǹ�.
    int code = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (!code) {
        client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        SOCKADDR_IN client_addr = {};
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(7777);
        InetPton(AF_INET, TEXT("127.0.0.1"), &client_addr.sin_addr);

        while (1) {
            if (!connect(client_sock, (SOCKADDR*)&client_addr, sizeof(client_addr))) {
                cout << "Server Connect" << endl;
                cout << "Chatting world�� ���Ű��� ȯ���մϴ�!!" << endl;
                break;
            }
            cout << "Connecting..." << endl;
        }

        while (true) {
            cout << "              1. �� �� ��               " << endl;
            cout << "              2. ȸ������               " << endl;

            int isLogin;
            cin >> isLogin;

            if (isLogin == 1) {
                if (!login()) return -1;
            }
        }

        std::thread th2(chat_recv);
        while (1) {
            string text;
            std::getline(cin, text);
            const char* buffer = text.c_str(); // string���� char* Ÿ������ ��ȯ
            send(client_sock, buffer, strlen(buffer), 0);
        }
        th2.join();
        closesocket(client_sock);
    }
    WSACleanup();
    return 0;
}