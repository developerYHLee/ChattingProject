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
vector<int> rooms_id;
int current_room;

bool show_rooms();

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

		if (ch == 32) continue;
		else if (ch == 13) { //\n
			if (res.length() <= 0 || (isPassword && res.length() < 8)) continue;
			break;
		}
		// 지우기 벡스페이스
		else if (ch == 8) {
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
int login() {
	string input_user_id, input_user_pw;
	cout << "\n아이디는 50자 이하로 입력하세요. 띄어쓰기는 입력할 수 없습니다.\n";
	cout << "비밀번호는 8자 이상 50자 이하로 입력하세요. 띄어쓰기는 입력할 수 없습니다.\n";

	char buf[1] = { };
	ZeroMemory(&buf, 1);

	cout << "아이디를 입력해주세요 : ";
	input_user_id = _input(false);

	cout << "비밀번호를 입력해주세요 : ";
	input_user_pw = _input(true);

	string input_user = input_user_id + " " + input_user_pw;
	//server에 id, password를 보내서 recv()로 id와 password 받기
	send(client_sock, input_user.c_str(), input_user.length(), 0);

	string msg;
	if (recv(client_sock, buf, 1, 0) > 0) {
		msg = buf;
		if (msg.compare("Y") == 0) return 1;
		else { 
			cout << "틀린 아이디/패스워드 입니다.\n\n";
			return 0; 
		}
	}
	else {
		cout << "server off" << endl;
		return -1;
	}
}

bool show_rooms() { //반환값이 false면 서버오류
	char buf[MAX_SIZE] = { };
	ZeroMemory(&buf, MAX_SIZE);
	if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
		string roomInfo = buf;
		std::istringstream iss(roomInfo);  // 문자열을 스트림화
		string temp;
		
		int index = 0;
		while (getline(iss, temp, ',')) {
			std::stringstream ss(temp);  // 문자열을 스트림화
			string msg;

			ss >> msg;
			rooms_id.push_back(stoi(msg));
			ss >> msg;

			cout << index++ << " : " << msg << endl;
		}
	}
	else {
		cout << "server off" << endl;
		return false;
	}

	return true;
}

int main()
{
	current_room = -1;

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
			cout << "              1. 로 그 인               \n";
			cout << "              2. 회원가입               \n";

			int isLogin;
			cin >> isLogin;

			if (isLogin == 1) {
				int loginRes = login();
				if (loginRes == -1) return -1;
				else if (loginRes == 0) continue;
				else break;
			}
		}
		cout << "\n로그인 성공!\n";
		cout << "[참여중인 채팅방 목록]\n\n";
		if (!show_rooms()) return -1;
		
		//채팅방 선택
		int room_number;
		cin >> room_number;

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