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

		if (ch == 32) continue;
		else if (ch == 13) { //\n
			if (res.length() <= 0 || (isPassword && res.length() < 8)) continue;
			break;
		}
		// ����� �������̽�
		else if (ch == 8) {
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
int login() {
	string input_user_id, input_user_pw;
	cout << "\n���̵�� 50�� ���Ϸ� �Է��ϼ���. ����� �Է��� �� �����ϴ�.\n";
	cout << "��й�ȣ�� 8�� �̻� 50�� ���Ϸ� �Է��ϼ���. ����� �Է��� �� �����ϴ�.\n";

	char buf[1] = { };
	ZeroMemory(&buf, 1);

	cout << "���̵� �Է����ּ��� : ";
	input_user_id = _input(false);

	cout << "��й�ȣ�� �Է����ּ��� : ";
	input_user_pw = _input(true);

	string input_user = input_user_id + " " + input_user_pw;
	//server�� id, password�� ������ recv()�� id�� password �ޱ�
	send(client_sock, input_user.c_str(), input_user.length(), 0);

	string msg;
	if (recv(client_sock, buf, 1, 0) > 0) {
		msg = buf;
		if (msg.compare("Y") == 0) return 1;
		else { 
			cout << "Ʋ�� ���̵�/�н����� �Դϴ�.\n\n";
			return 0; 
		}
	}
	else {
		cout << "server off" << endl;
		return -1;
	}
}

bool show_rooms() { //��ȯ���� false�� ��������
	char buf[MAX_SIZE] = { };
	ZeroMemory(&buf, MAX_SIZE);
	if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
		string roomInfo = buf;
		std::istringstream iss(roomInfo);  // ���ڿ��� ��Ʈ��ȭ
		string temp;
		
		int index = 0;
		while (getline(iss, temp, ',')) {
			std::stringstream ss(temp);  // ���ڿ��� ��Ʈ��ȭ
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
			cout << "              1. �� �� ��               \n";
			cout << "              2. ȸ������               \n";

			int isLogin;
			cin >> isLogin;

			if (isLogin == 1) {
				int loginRes = login();
				if (loginRes == -1) return -1;
				else if (loginRes == 0) continue;
				else break;
			}
		}
		cout << "\n�α��� ����!\n";
		cout << "[�������� ä�ù� ���]\n\n";
		if (!show_rooms()) return -1;
		
		//ä�ù� ����
		int room_number;
		cin >> room_number;

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