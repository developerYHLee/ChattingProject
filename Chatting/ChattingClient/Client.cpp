#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h> //Winsock 헤더파일 include. WSADATA 들어있음.
#include <WS2tcpip.h>
#include <sstream>
#include <thread>
#include <conio.h>
#include <vector>
#include <iostream>
#include <string>
#include <ctime>

#define MAX_SIZE 1024
using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;

struct ROOMS_INFO {
	int id;
	string room_name;
	int count_client;
};

SOCKET client_sock;
string my_id, my_rooms, past_date = "";
vector<ROOMS_INFO> rooms_info;
int current_room, count_room;

bool set_my_rooms(); //나의 채팅방 목록 my_rooms에 저장
bool choose_room(); //채팅방 선택하기
void exit_room(); //채팅방 화면 나가기
bool get_message(); //채팅방의 이전 메시지 출력
int sign_in();
bool menu(); //반환값이 false면 클라이언트 종료

int chat_recv() {
	char buf[MAX_SIZE] = { };
	string msg;
	while (1) {
		ZeroMemory(&buf, MAX_SIZE);
		if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
			string user, room_id, temp;

			msg = buf;

			int pos = msg.find(",");  // 첫 번째 ','의 위치를 찾습니다.
		
			room_id = msg.substr(0, pos++);
			msg = msg.substr(pos);
			
			string cur_date, time;
			std::stringstream ss(msg);  // 문자열을 스트림화
			ss >> cur_date;
			ss >> time;
			ss >> user; // 스트림을 통해, 문자열을 공백 분리해 변수에 할당

			pos = msg.find(user);
			msg = msg.substr(pos);

			if (user != my_id && std::stoi(room_id) == current_room) {
				if (cur_date.compare(past_date) != 0) {
					cout << "(" << cur_date << ")\n";
					past_date = cur_date;
				}
				cout << msg << endl; // 내가 보낸 게 아니고 내 방이 아닐 경우에만 출력하도록.
			}
		}
		else {
			cout << "Server Off" << endl;
			return -1;
		}
	}
}

//키보드 입력
string _input(bool isPassword, bool isSign) {
	string res = "";

	while (true) {
		char ch = _getch();
		
		if (ch == 32) continue;
		else if (ch == 13) { //\n
			if (res.length() < 1 && isSign && ch == 88) return "X";
			if (res.length() < 3) continue;
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
	cout << "\n아이디와 비밀번호는 3자 이상 50자 이하로 입력하세요. 띄어쓰기는 입력할 수 없습니다.\n";
	
	char buf[2] = { };
	ZeroMemory(&buf, 2);

	cout << "아이디를 입력해주세요 : ";
	input_user_id = _input(false, false);

	cout << "비밀번호를 입력해주세요 : ";
	input_user_pw = _input(true, false);

	string input_user = "LOGIN " + input_user_id + " " + input_user_pw;
	//server에 id, password를 보내서 recv()로 id와 password 받기
	send(client_sock, input_user.c_str(), input_user.length(), 0);

	string msg;
	if (recv(client_sock, buf, 1, 0) > 0) {
		msg = buf;
		if (msg.compare("Y") == 0) {
			my_id = input_user_id;

			return 1;
		}
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

int sign_in() {
	string input_user_id, input_user_pw, input_user_nickname;
	cout << "\n아이디와 비밀번호는 3자 이상 50자 이하로 입력하세요. 띄어쓰기는 입력할 수 없습니다.\n";

	char buf[2] = { };
	ZeroMemory(&buf, 2);

	cout << "아이디를 입력해주세요 : ";
	input_user_id = _input(false, true);
	if (input_user_id.compare("X") == 0) return 0;

	cout << "비밀번호를 입력해주세요 : ";
	input_user_pw = _input(true, true);
	if (input_user_pw.compare("X") == 0) return 0;

	cout << "닉네임을 입력해주세요 : ";
	input_user_nickname = _input(false, true);
	if (input_user_nickname.compare("X") == 0) return 0;

	string input_user = "SIGN_IN " + input_user_id + " " + input_user_pw + " " + input_user_nickname;
	//server에 id, password, nickname 보내기
	send(client_sock, input_user.c_str(), input_user.length(), 0);

	string msg;
	if (recv(client_sock, buf, 1, 0) > 0) {
		msg = buf;
		if (msg.compare("Y") == 0) {
			my_id = input_user_id;

			return 1;
		}
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

bool set_my_rooms() { //반환값이 false면 서버오류
	char buf[MAX_SIZE] = { };
	ZeroMemory(&buf, MAX_SIZE);

	my_rooms = "";
	if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
		string roomInfo = buf;
		if (roomInfo.compare("참여중인 채팅방이 없습니다.") == 0) {
			cout << "참여중인 채팅방이 없습니다.\n";
			return true;
		}
		std::istringstream iss(roomInfo);  // 문자열을 스트림화
		string temp;
		
		count_room = 0;
		while (getline(iss, temp, ',')) {
			std::stringstream ss(temp);  // 문자열을 스트림화

			ROOMS_INFO room = {};
			ss >> room.id;
			ss >> room.room_name;
			ss >> room.count_client;
			rooms_info.push_back(room);

			my_rooms += std::to_string(count_room++) + " : " + room.room_name + "(" + std::to_string(room.count_client) + ")\n";
		}
	}
	else {
		cout << "server off" << endl;
		return false;
	}

	return true;
}

bool choose_room() {
	int room_number = -1;
	while (room_number < 0 || room_number >= count_room) {
		cout << "채팅방을 선택하세요. : ";
		cin >> room_number;
	}

	current_room = rooms_info[room_number].id;
	cout << rooms_info[room_number].room_name << " 방에 입장했습니다.\n\n";

	char buf[MAX_SIZE] = { };
	ZeroMemory(&buf, MAX_SIZE);

	string msg = "GETMESSAGE," + std::to_string(current_room);
	send(client_sock, msg.c_str(), msg.length(), 0);

	if(!get_message()) return false;

	return true;
}

bool get_message() {
	char buf[MAX_SIZE] = { };
	ZeroMemory(&buf, MAX_SIZE);

	string msg = "";
	if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {		
		msg = buf;

		if (msg.compare("X") == 0) {
			time_t timer;
			struct tm* t;
			timer = time(NULL);
			t = localtime(&timer);

			cout << "(" + std::to_string(t->tm_year + 1900) + "-" + std::to_string(t->tm_mon + 1) + "-" + std::to_string(t->tm_mday) + ")\n";

			return true;
		}
		std::istringstream iss(msg);  // 문자열을 스트림화
		string temp_msg;

		while (getline(iss, temp_msg, '\n')) {
			string cur_date, time, message, temp_date;

			int pos = temp_msg.find(",");  // 첫 번째 ','의 위치를 찾습니다.

			temp_date = temp_msg.substr(0, pos++);
			message = temp_msg.substr(pos);

			std::stringstream ss(temp_date);
			ss >> cur_date;
			ss >> time;

			if (cur_date.compare(past_date) != 0) {
				cout << "(" << cur_date << ")\n";
				past_date = cur_date;
			}

			cout << message << " (" << time << ")" << endl;
		}
	}
	else {
		cout << "Server Off" << endl;
		return -1;
	}

	return true;
}

void exit_room() {
	cout << "채팅방을 나갔습니다.\n\n";
	cout << my_rooms;
	choose_room();
}

bool menu() {
	cout << "1. [회원정보 수정]\n";
	cout << "2. [참여중인 채팅방 목록]\n";
	cout << "3. [로그아웃]\n";

	int menu;
	cin >> menu;

	if (menu == 1) {

	}
	else if (menu == 2) {
		cout << my_rooms << endl;
		choose_room();
	}
	else if (menu == 3) return false;
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
			else if (isLogin == 2) {
				int signRes = sign_in();
				if (signRes == -1) return -1;
				else if (signRes == 0) continue;
				else break;
			}
		}
		cout << "\n로그인 성공!\n\n";
		if (!set_my_rooms()) return -1;

		if(!menu()) return 0;
		//exit_room();

		cin.ignore();
		std::thread th2(chat_recv);
		while (1) {
			string text;
			std::getline(cin, text);
			//if() //인원초대USERINVITE,
			//else if() //인원 목록 조회USERLIST,
			
			string buffer = "MESSAGE," + std::to_string(current_room) + "," + text; // string형을 char* 타입으로 변환
			
			send(client_sock, buffer.c_str(), buffer.length(), 0);
		}
		th2.join();

		closesocket(client_sock);
	}
	WSACleanup();
	return 0;
}