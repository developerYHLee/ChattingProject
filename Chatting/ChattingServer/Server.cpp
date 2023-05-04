#pragma comment(lib, "ws2_32.lib") //��������� ���̺귯���� ȣ���ϴ� ��� �߿� �ϳ�

#include <iostream>
#include <sstream>
#include <string>
#include <WinSock2.h>
#include <thread>
#include <vector>
#include <mysql/jdbc.h>

#define MAX_SIZE 1024
#define MAX_CLIENT 3

using std::cout;
using std::cin;
using std::endl;
using std::string;

//------------------------ DB
const string server = "tcp://127.0.0.1:3306"; // �����ͺ��̽� �ּ�
const string username = "root"; // �����ͺ��̽� �����
const string password = "sql123456789!@#"; // �����ͺ��̽� ���� ��й�ȣ

sql::Connection* con;
sql::Statement* stmt;
sql::PreparedStatement* pstmt;

string ID_Check(string id);
void Select(string table);
//------------------------

//------------------------ Server
struct SOCKET_INFO {
    SOCKET sck; //unsigned int pointer ��
    string user; //��� �̸�
};

std::vector<SOCKET_INFO> sck_list; //������ ����� client ������ ���� => vector
SOCKET_INFO server_sock; //���� ������ ������ ������ ����
int client_count = 0; //���� ���ӵ� Ŭ���̾�Ʈ �� ī��Ʈ �뵵

void server_init(); //������ ������ ����� �Լ�, ~listen()���� ����
void add_client(); //accept �Լ� ����ǰ� ���� ����
void send_msg(const char* msg); //send() ����
void recv_msg(int idx); //recv() ����
void del_client(int idx); //Ŭ���̾�Ʈ���� ������ ���� ��
//------------------------

void Insert_user_info(string id, string password, string nickname) {
    if (ID_Check(id).compare("X") != 0) {
        Select("user_info");
        cout << "�ߺ��� id �Դϴ�.\n";
        return;
    }
    pstmt = con->prepareStatement("INSERT INTO user_info(id, password, nickname) VALUES(?,?,?)"); // INSERT

    pstmt->setString(1, id);
    pstmt->setString(2, password);
    pstmt->setString(3, nickname);
    pstmt->execute();
    cout << "Insert_user_info Success" << endl;
}

//user_info id �ߺ� üũ, ��ȯ���� X�� �ƴϸ� �ߺ�
string ID_Check(string id) {
    sql::ResultSet* result;

    pstmt = con->prepareStatement("select * from user_info where id = ?");
    pstmt->setString(1, id);

    result = pstmt->executeQuery();

    int count = result->rowsCount();
    string password = "";

    result->next();
    if (count > 0) password = result->getString(2).c_str();
    else password = "X";

    delete result;

    return password;
}

void Select(string table) {
    sql::ResultSet* result;

    string str = "select * from ";
    str += table + ";";

    pstmt = con->prepareStatement(str);
    result = pstmt->executeQuery();
    while (result->next())
        printf("Reading from table=(%s, %s, %s)\n", result->getString(1).c_str(), result->getString(2).c_str(), result->getString(3).c_str());
    
    delete result;
}

void Update() {
    pstmt = con->prepareStatement("UPDATE inventory SET quantity = ? WHERE name = ?");
    pstmt->setInt(1, 200);
    pstmt->setString(2, "banana");
    pstmt->executeQuery();
    printf("Row updated\n");
}

void Delete() {
    sql::ResultSet* result;

    pstmt = con->prepareStatement("delete from inventory where name = ?");
    pstmt->setString(1, "orange");
    result = pstmt->executeQuery();
    printf("Row deleted\n");

    delete result;
}

int main()
{
    // MySQL Connector/C++ �ʱ�ȭ
    sql::mysql::MySQL_Driver* driver; // ���� �������� �ʾƵ� Connector/C++�� �ڵ����� ������ ��

    try {
        driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect(server, username, password);
    }
    catch (sql::SQLException& e) {
        cout << "Could not connect to server. Error message: " << e.what() << endl;
        exit(1);
    }

    // �����ͺ��̽� ����
    con->setSchema("chatting");

    // db �ѱ� ������ ���� ���� 
    stmt = con->createStatement();
    stmt->execute("set names euckr");
    if (stmt) { delete stmt; stmt = nullptr; }

    //Insert_user_info("abc", "defg", "alpha"); //�ߺ� Ȯ���ϴ� �ڵ� �߰�

    WSADATA wsa;

    // Winsock�� �ʱ�ȭ�ϴ� �Լ�. MAKEWORD(2, 2)�� Winsock�� 2.2 ������ ����ϰڴٴ� �ǹ�.
    // ���࿡ �����ϸ� 0��, �����ϸ� �� �̿��� ���� ��ȯ.
    // 0�� ��ȯ�ߴٴ� ���� Winsock�� ����� �غ� �Ǿ��ٴ� �ǹ�.
    int code = WSAStartup(MAKEWORD(2, 2), &wsa); //�����ϸ� 0
    if (!code) {
        server_init(); //������ ���� Ȱ��ȭ
        
        //ũ�Ⱑ MAX_CLIENT(3)�� �迭 ����, �迭�� ��� �ڷ����� std::thread
        std::thread th1[MAX_CLIENT];
        for (int i = 0; i < MAX_CLIENT; i++) {
            //Ŭ���̾�Ʈ�� ���� �� �ִ� ���¸� ����� ��, accept �ѹ��� ���� �ϳ�
            th1[i] = std::thread(add_client); //main �Լ��� ������� �ʵ��� ����, return 0�� ȣ������ �ʴ´�. add_client�� �޼ҵ�
        }

        while (1) {
            string text, msg = "";

            std::getline(cin, text);
            const char* buf = text.c_str();
            msg = server_sock.user + " : " + buf;
            send_msg(msg.c_str());
        }

        for (int i = 0; i < MAX_CLIENT; i++) {
            //join()�� main �Լ��� ������� �ʱ� ���� �Լ�
            //add_client�� ������ main �Լ��� ����
            th1[i].join();
        }

        closesocket(server_sock.sck);
    }
    else {
        cout << "���α׷� ����. (Error code : " << code << ")";
    }

    WSACleanup();

    // MySQL Connector/C++ ����
    delete stmt;
    delete pstmt;
    delete con;

    return 0;
}

void server_init() {
    //���� ������ Ư���� �� �ִ� int�� ���ڸ� server_sock�� sck�� ����
    //SOCKET_INFO sck, user
    server_sock.sck = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //PF_INET == AF_INET

    SOCKADDR_IN server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(7777); //~~~~:7777, port �κ��� ����
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //127.0.0.1:~~~~, ip �κ��� ����

    //server_sock.sck, �ּҸ� �Ҵ��ϰ� ���� socket
    //server_addr�� �ڷ��� SOCKADDR_IN�� sockaddr* ������ ��ȯ
    bind(server_sock.sck, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock.sck, SOMAXCONN);

    server_sock.user = "server"; //���� ���ϴ� �̸� ����

    cout << "Server On" << endl;
}

void add_client() {
    //Ŭ���̾�Ʈ�� ������ ���ῡ �����ϸ�, ���ο� ������ �ϳ� �����ϰ� �Ǵµ�,
    //�� �ּҸ� ���� ���� => addr
    SOCKADDR_IN addr = {};
    int addrsize = sizeof(addr);
    ZeroMemory(&addr, addrsize); //addr �޸𸮸� 0(0x00)���� �ʱ�ȭ

    //sck, user : Ŭ���̾�Ʈ�� ���� ������ ����
    SOCKET_INFO new_client = {};

    new_client.sck = accept(server_sock.sck, (sockaddr*)&addr, &addrsize);
    //connect()

    char buf[MAX_SIZE] = { }; //�޽��� �ִ� ���� ����
    while(true) {
        string input_msg;

        ZeroMemory(&buf, MAX_SIZE);
        //Ŭ���̾�Ʈ connect(), send()
        if (recv(new_client.sck, buf, MAX_SIZE, 0) > 0) {
            input_msg = buf;
            std::stringstream ss(input_msg);  // ���ڿ��� ��Ʈ��ȭ
            string user_id, user_password;
            ss >> user_id; // ��Ʈ���� ����, ���ڿ��� ���� �и��� ������ �Ҵ�
            ss >> user_password;

            new_client.user = string(user_id); //new_client.user�� id ����
            string password = ID_Check(new_client.user);

            string return_msg = "";
            if (password.compare(user_password) == 0) {
                string msg = "[����] " + new_client.user + " ���� �����߽��ϴ�.";
                cout << msg << endl; //���� �ֿܼ� ���� ����
                return_msg = "Y";
                send(new_client.sck, return_msg.c_str(), 1, 0);
                break;
            }
            else {
                return_msg = "N";
                send(new_client.sck, return_msg.c_str(), 1, 0);
            }
        }

        else return;
    }
    
    sck_list.push_back(new_client); //sck list�� �߰���

    //��� ������ client�� �����ε� ��� �޽����� ���� �� �ֵ��� recv
    std::thread th(recv_msg, client_count);

    client_count++;
    cout << "[����] ���� ������ �� : " << client_count << "��" << endl;
    //send_msg(msg.c_str()); //���� ���Դ��� ���� �ϱ� ���ؼ�

    th.join();
}

//��ü Ŭ���̾�Ʈ(����)���� ������
void send_msg(const char* msg) {
    for (int i = 0; i < client_count; i++) {
        send(sck_list[i].sck, msg, MAX_SIZE, 0);
    }
}

void recv_msg(int idx) {
    char buf[MAX_SIZE] = { };
    string msg = "";

    while (1) {
        ZeroMemory(&buf, MAX_SIZE);
        if (recv(sck_list[idx].sck, buf, MAX_SIZE, 0) > 0) {
            msg = sck_list[idx].user + " : " + buf;
            cout << msg << endl;
            send_msg(msg.c_str());
        }
        else {
            msg = "[����] " + sck_list[idx].user + " ���� �����߽��ϴ�.";
            cout << msg << endl;
            send_msg(msg.c_str());
            del_client(idx);
            return;
        }
    }
}

void del_client(int idx) {
    closesocket(sck_list[idx].sck);
    client_count--;
}