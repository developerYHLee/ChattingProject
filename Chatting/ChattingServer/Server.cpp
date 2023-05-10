#pragma comment(lib, "ws2_32.lib") //��������� ���̺귯���� ȣ���ϴ� ��� �߿� �ϳ�

#include <iostream>
#include <sstream>
#include <string>
#include <WinSock2.h>
#include <thread>
#include <vector>
#include <mysql/jdbc.h>
#include <algorithm>

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
string get_rooms(string user_id); //chatting_user�� room_id ��������
void SaveMessage(int room_id, string user_id, string msg);
//------------------------

//------------------------ Server
struct SOCKET_INFO {
    SOCKET sck; //unsigned int pointer ��
    string user; //��� �̸�

    bool operator<(const SOCKET_INFO& other) const {
        return user < other.user;
    }
};

template<typename T> class SortedVector {
public:
    void insert(const T& value) {
        auto it = std::lower_bound(data_.begin(), data_.end(), value);
        data_.insert(it, value);
    }
    void erase(int idx) {
        data_.erase(data_.begin() + idx);
    }

    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }
    size_t size() const { return data_.size(); }

private:
    std::vector<T> data_;
};

SortedVector<SOCKET_INFO> sck_list_sort; //������ ����� client ������ ���� => SortedVector, �޼��� ���ۿ� ����
std::vector<SOCKET_INFO> sck_list; //������ ����� client ������ ���� => SortedVector, �޼��� ���� �ܿ� ����
SOCKET_INFO server_sock; //���� ������ ������ ������ ����
int client_count = 0; //���� ���ӵ� Ŭ���̾�Ʈ �� ī��Ʈ �뵵
int client_show_room_count = 0;

void server_init(); //������ ������ ����� �Լ�, ~listen()���� ����
void add_client(); //accept �Լ� ����ǰ� ���� ����
void send_msg(const char* msg); //send() ����
void recv_msg(int idx); //recv() ����
void del_client(int idx); //Ŭ���̾�Ʈ���� ������ ���� ��
void del_client(string name);
void login_client(); //Ŭ���̾�Ʈ �α���
void show_rooms(int idx); //Ŭ���̾�Ʈ ���� �� �����ֱ�
int search(string key);
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

string get_rooms(string user_id) {
    sql::ResultSet* result;
    std::vector<string> infos;

    pstmt = con->prepareStatement("select * from chatting_room where id in(select room_id from chatting_user where user_id = ?);");
    pstmt->setString(1, user_id);
    result = pstmt->executeQuery();

    int count = result->rowsCount();
    if (count <= 0) return "�������� ä�ù��� �����ϴ�.";
    
    while (result->next()) {
        string temp = "";
        temp += std::to_string(result->getInt(1)) + " " + result->getString(2) + " " + std::to_string(result->getInt(3));
        infos.push_back(temp);
    }

    delete result;

    string info = "";
    for (int i = 0; i < infos.size() - 1; i++) info += infos[i] + ",";
    info += infos[infos.size() - 1];

    return info;
}

void SaveMessage(int room_id, string user_id, string msg) {
    sql::ResultSet* result;

    pstmt = con->prepareStatement("INSERT INTO chatting_message(room_id, user_id, message, _time) VALUES(?,?,?, current_time())"); // INSERT

    pstmt->setInt(1, room_id);
    pstmt->setString(2, user_id);
    pstmt->setString(3, msg);
    pstmt->execute();
    cout << "SaveMessage Success" << endl;
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
    //Ŭ���̾�Ʈ�� ������ ���ῡ �����ϸ�, �α��� �õ�
    login_client();

    //Ŭ���̾�Ʈ�� ������ �� �����ֱ�
    show_rooms(client_count);

    //��� ������ client�� �����ε� ��� �޽����� ���� �� �ֵ��� recv
    std::thread th(recv_msg, client_count);

    client_count++;
    cout << "[����] ���� ������ �� : " << client_count << "��" << endl;
    //send_msg(msg.c_str()); //���� ���Դ��� ���� �ϱ� ���ؼ�

    th.join();
}

void show_rooms(int idx) {
    string msg = get_rooms(sck_list[idx].user);
    
    send(sck_list[idx].sck, msg.c_str(), msg.length(), 0);
}

void login_client() {
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
    while (true) {
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
                send(new_client.sck, return_msg.c_str(), return_msg.length(), 0);
                break;
            }
            else {
                return_msg = "N";
                send(new_client.sck, return_msg.c_str(), return_msg.length(), 0);
            }
        }

        else return;
    }

    cout << new_client.user << " socket : " << new_client.sck << endl;
    sck_list_sort.insert(new_client); //sck list_sort�� �߰���
    sck_list.push_back(new_client); //sck list�� �߰���
}

//��ü Ŭ���̾�Ʈ(����)���� ������
void send_msg(const char* msg) {
    for (int i = 0; i < client_count; i++) {
        send(sck_list[i].sck, msg, MAX_SIZE, 0);
    }
}

//������ ä�ù��� Ŭ���̾�Ʈ���� ������
void send_msg(const char* msg, int room_number) {
    sql::ResultSet* result;
    std::vector<string> users_name;

    pstmt = con->prepareStatement("select user_id from chatting_user where room_id = ?;");
    pstmt->setInt(1, room_number);
    result = pstmt->executeQuery();
    while (result->next())
        users_name.push_back(result->getString(1));
    
    delete result;
    
    string str = std::to_string(room_number) + "," + msg;
    for (int i = 0; i < users_name.size(); i++) {
        int idx = search(users_name[i]);
        if(idx != -1) send(sck_list_sort[idx].sck, str.c_str(), str.length(), 0);
    }
}

void recv_msg(int idx) {
    char buf[MAX_SIZE] = { };
    string msg = "";

    while (1) {
        ZeroMemory(&buf, MAX_SIZE);
        if (recv(sck_list[idx].sck, buf, MAX_SIZE, 0) > 0) {
            string option;

            msg = buf;

            int pos = msg.find(",");  // ù ��° ','�� ��ġ�� ã���ϴ�.
            string room_number, temp_msg;

            option = msg.substr(0, pos++);
            msg = msg.substr(pos);

            if (option.compare("MESSAGE") == 0) {
                int pos = msg.find(",");  // ù ��° ','�� ��ġ�� ã���ϴ�.

                room_number = msg.substr(0, pos++);
                temp_msg = msg.substr(pos);

                msg = sck_list[idx].user + " : " + temp_msg;
                cout << msg << endl;
                SaveMessage(stoi(room_number), sck_list[idx].user, temp_msg);
                send_msg(msg.c_str(), stoi(room_number));
            }
        }
        else {
            msg = "[����] " + sck_list[idx].user + " ���� �����߽��ϴ�.";
            cout << msg << endl;
            send_msg(msg.c_str());

            del_client(sck_list[idx].user);
            del_client(idx);
            client_count--;
            return;
        }
    }
}

void del_client(string name) {
    int idx = search(name);
    
    cout << "delete_client_sort : " << sck_list_sort[idx].user << endl;
    closesocket(sck_list_sort[idx].sck);
    sck_list_sort.erase(idx);
    for (int i = 0; i < sck_list_sort.size(); i++) {
        cout << sck_list_sort[i].user << sck_list_sort[i].sck << endl;
    }
}

void del_client(int idx) {
    cout << "delete_client : " << sck_list[idx].user << endl;
    closesocket(sck_list[idx].sck); 
    sck_list.erase(sck_list.begin() + idx);
    for (int i = 0; i < sck_list.size(); i++) {
        cout << sck_list[i].user << sck_list[i].sck << endl;
    }
}

int search(string key) {
    int l = 0, r = client_count - 1, mid;

    while (l <= r) {
        mid = (l + r) / 2;

        if (sck_list_sort[mid].user.compare(key) == 0) return mid;
        else if (sck_list_sort[mid].user > key) r = mid - 1;
        else l = mid + 1;
    }

    return -1;
}