#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h> //Winsock 헤더파일 include. WSADATA 들어있음.
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
            std::stringstream ss(msg);  // 문자열을 스트림화
            string user;
            ss >> user; // 스트림을 통해, 문자열을 공백 분리해 변수에 할당
            if (user != my_id) cout << buf << endl; // 내가 보낸 게 아닐 경우에만 출력하도록.
        }
        else {
            cout << "Server Off" << endl;
            return -1;
        }
    }
}

//키보드 입력
string _input(boolean isPassword) {
    string res = "";

    while (true) {
        char ch = _getch();

        if (ch == 13) { //\n
            if (res.length() <= 0) continue;
            break;
        }

        // 지우기 벡스페이스
        if (ch == 8) {
            if (res.length() == 0) {
                continue;
            }
            res.pop_back();

            // 커서 뒤로가기
            cout << "\b \b";
        }
        else {
            if (res.length() > 50) {
                cout << "\b \b";
                res.pop_back();
                
                if (isPassword) std::cout << '*'; //암호를 *로 표시
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

//아이디 확인 후 비밀번호 입력, 반환 값이 false면 프로그램 종료
bool login() {
    string input_user_id;
    cout << "\n50자 이하로 입력하세요.\n";
    cout << "아이디를 입력해주세요 : ";

    input_user_id = _input(false);

    //server에 id를 보내서 recv()로 id와 password 받기
    send(client_sock, input_user_id.c_str(), input_user_id.length(), 0);

    string input_user_pw;

    cout << "비밀번호를 입력해주세요 : ";
    input_user_pw = _input(true);

    char buf[MAX_SIZE] = { };
    string msg;
    //while (1) {
    //    zeromemory(&buf, max_size);
    //    if (recv(client_sock, buf, max_size, 0) > 0) {
    //        msg = buf;
    //        std::stringstream ss(msg);  // 문자열을 스트림화
    //        string user;
    //        ss >> user; // 스트림을 통해, 문자열을 공백 분리해 변수에 할당

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

    // Winsock를 초기화하는 함수. MAKEWORD(2, 2)는 Winsock의 2.2 버전을 사용하겠다는 의미.
    // 실행에 성공하면 0을, 실패하면 그 이외의 값을 반환.
    // 0을 반환했다는 것은 Winsock을 사용할 준비가 되었다는 의미.
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
                cout << "Chatting world에 오신것을 환영합니다!!" << endl;
                break;
            }
            cout << "Connecting..." << endl;
        }

        while (true) {
            cout << "              1. 로 그 인               " << endl;
            cout << "              2. 회원가입               " << endl;

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
            const char* buffer = text.c_str(); // string형을 char* 타입으로 변환
            send(client_sock, buffer, strlen(buffer), 0);
        }
        th2.join();
        closesocket(client_sock);
    }
    WSACleanup();
    return 0;
}