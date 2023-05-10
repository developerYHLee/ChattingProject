#pragma comment(lib, "ws2_32.lib") //명시적으로 라이브러리를 호출하는 방법 중에 하나

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
const string server = "tcp://127.0.0.1:3306"; // 데이터베이스 주소
const string username = "root"; // 데이터베이스 사용자
const string password = "sql123456789!@#"; // 데이터베이스 접속 비밀번호

sql::Connection* con;
sql::Statement* stmt;
sql::PreparedStatement* pstmt;

string ID_Check(string id);
void Select(string table);
string get_rooms(string user_id); //chatting_user의 room_id 가져오기
void SaveMessage(int room_id, string user_id, string msg);
//------------------------

//------------------------ Server
struct SOCKET_INFO {
    SOCKET sck; //unsigned int pointer 형
    string user; //사람 이름

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

SortedVector<SOCKET_INFO> sck_list_sort; //서버에 연결된 client 저장할 변수 => SortedVector, 메세지 전송에 쓰임
std::vector<SOCKET_INFO> sck_list; //서버에 연결된 client 저장할 변수 => SortedVector, 메세지 전송 외에 쓰임
SOCKET_INFO server_sock; //서버 소켓의 정보를 저장할 변수
int client_count = 0; //현재 접속된 클라이언트 수 카운트 용도
int client_show_room_count = 0;

void server_init(); //서버용 소켓을 만드는 함수, ~listen()까지 실행
void add_client(); //accept 함수 실행되고 있을 예정
void send_msg(const char* msg); //send() 실행
void recv_msg(int idx); //recv() 실행
void del_client(int idx); //클라이언트와의 연결을 끊을 때
void del_client(string name);
void login_client(); //클라이언트 로그인
void show_rooms(int idx); //클라이언트 참여 방 보여주기
int search(string key);
//------------------------

void Insert_user_info(string id, string password, string nickname) {
    if (ID_Check(id).compare("X") != 0) {
        Select("user_info");
        cout << "중복된 id 입니다.\n";
        return;
    }
    pstmt = con->prepareStatement("INSERT INTO user_info(id, password, nickname) VALUES(?,?,?)"); // INSERT

    pstmt->setString(1, id);
    pstmt->setString(2, password);
    pstmt->setString(3, nickname);
    pstmt->execute();
    cout << "Insert_user_info Success" << endl;
}

//user_info id 중복 체크, 반환값이 X가 아니면 중복
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
    if (count <= 0) return "참여중인 채팅방이 없습니다.";
    
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
    // MySQL Connector/C++ 초기화
    sql::mysql::MySQL_Driver* driver; // 추후 해제하지 않아도 Connector/C++가 자동으로 해제해 줌

    try {
        driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect(server, username, password);
    }
    catch (sql::SQLException& e) {
        cout << "Could not connect to server. Error message: " << e.what() << endl;
        exit(1);
    }

    // 데이터베이스 선택
    con->setSchema("chatting");

    // db 한글 저장을 위한 셋팅 
    stmt = con->createStatement();
    stmt->execute("set names euckr");
    if (stmt) { delete stmt; stmt = nullptr; }

    //Insert_user_info("abc", "defg", "alpha"); //중복 확인하는 코드 추가

    WSADATA wsa;

    // Winsock를 초기화하는 함수. MAKEWORD(2, 2)는 Winsock의 2.2 버전을 사용하겠다는 의미.
    // 실행에 성공하면 0을, 실패하면 그 이외의 값을 반환.
    // 0을 반환했다는 것은 Winsock을 사용할 준비가 되었다는 의미.
    int code = WSAStartup(MAKEWORD(2, 2), &wsa); //성공하면 0
    if (!code) {
        server_init(); //서버측 소켓 활성화
        
        //크기가 MAX_CLIENT(3)인 배열 생성, 배열에 담길 자료형은 std::thread
        std::thread th1[MAX_CLIENT];
        for (int i = 0; i < MAX_CLIENT; i++) {
            //클라이언트를 받을 수 있는 상태를 만들어 줌, accept 한번당 소켓 하나
            th1[i] = std::thread(add_client); //main 함수가 종료되지 않도록 해줌, return 0을 호출하지 않는다. add_client는 메소드
        }

        while (1) {
            string text, msg = "";

            std::getline(cin, text);
            const char* buf = text.c_str();
            msg = server_sock.user + " : " + buf;
            send_msg(msg.c_str());
        }

        for (int i = 0; i < MAX_CLIENT; i++) {
            //join()은 main 함수가 종료되지 않기 위한 함수
            //add_client가 끝나면 main 함수가 종료
            th1[i].join();
        }

        closesocket(server_sock.sck);
    }
    else {
        cout << "프로그램 종료. (Error code : " << code << ")";
    }

    WSACleanup();

    // MySQL Connector/C++ 정리
    delete stmt;
    delete pstmt;
    delete con;

    return 0;
}

void server_init() {
    //서버 소켓을 특정할 수 있는 int형 숫자를 server_sock의 sck에 담음
    //SOCKET_INFO sck, user
    server_sock.sck = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //PF_INET == AF_INET

    SOCKADDR_IN server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(7777); //~~~~:7777, port 부분을 정함
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //127.0.0.1:~~~~, ip 부분을 정함

    //server_sock.sck, 주소를 할당하고 싶은 socket
    //server_addr의 자료형 SOCKADDR_IN을 sockaddr* 형으로 변환
    bind(server_sock.sck, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock.sck, SOMAXCONN);

    server_sock.user = "server"; //내가 원하는 이름 지정

    cout << "Server On" << endl;
}

void add_client() {
    //클라이언트와 서버가 연결에 성공하면, 로그인 시도
    login_client();

    //클라이언트가 참여한 방 보여주기
    show_rooms(client_count);

    //방금 생성된 client가 앞으로도 계속 메시지를 받을 수 있도록 recv
    std::thread th(recv_msg, client_count);

    client_count++;
    cout << "[공지] 현재 접속자 수 : " << client_count << "명" << endl;
    //send_msg(msg.c_str()); //누가 들어왔는지 공지 하기 위해서

    th.join();
}

void show_rooms(int idx) {
    string msg = get_rooms(sck_list[idx].user);
    
    send(sck_list[idx].sck, msg.c_str(), msg.length(), 0);
}

void login_client() {
    //클라이언트와 서버가 연결에 성공하면, 새로운 소켓을 하나 생성하게 되는데,
    //그 주소를 담을 변수 => addr
    SOCKADDR_IN addr = {};
    int addrsize = sizeof(addr);
    ZeroMemory(&addr, addrsize); //addr 메모리를 0(0x00)으로 초기화

    //sck, user : 클라이언트의 소켓 정보를 저장
    SOCKET_INFO new_client = {};

    new_client.sck = accept(server_sock.sck, (sockaddr*)&addr, &addrsize);
    //connect()
    
    char buf[MAX_SIZE] = { }; //메시지 최대 길이 설정
    while (true) {
        string input_msg;

        ZeroMemory(&buf, MAX_SIZE);
        //클라이언트 connect(), send()
        if (recv(new_client.sck, buf, MAX_SIZE, 0) > 0) {
            input_msg = buf;
            std::stringstream ss(input_msg);  // 문자열을 스트림화
            string user_id, user_password;
            ss >> user_id; // 스트림을 통해, 문자열을 공백 분리해 변수에 할당
            ss >> user_password;

            new_client.user = string(user_id); //new_client.user에 id 저장
            string password = ID_Check(new_client.user);

            string return_msg = "";
            if (password.compare(user_password) == 0) {
                string msg = "[공지] " + new_client.user + " 님이 입장했습니다.";
                cout << msg << endl; //서버 콘솔에 공지 찍음
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
    sck_list_sort.insert(new_client); //sck list_sort에 추가함
    sck_list.push_back(new_client); //sck list에 추가함
}

//전체 클라이언트(소켓)에게 보내기
void send_msg(const char* msg) {
    for (int i = 0; i < client_count; i++) {
        send(sck_list[i].sck, msg, MAX_SIZE, 0);
    }
}

//선택한 채팅방의 클라이언트에게 보내기
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

            int pos = msg.find(",");  // 첫 번째 ','의 위치를 찾습니다.
            string room_number, temp_msg;

            option = msg.substr(0, pos++);
            msg = msg.substr(pos);

            if (option.compare("MESSAGE") == 0) {
                int pos = msg.find(",");  // 첫 번째 ','의 위치를 찾습니다.

                room_number = msg.substr(0, pos++);
                temp_msg = msg.substr(pos);

                msg = sck_list[idx].user + " : " + temp_msg;
                cout << msg << endl;
                SaveMessage(stoi(room_number), sck_list[idx].user, temp_msg);
                send_msg(msg.c_str(), stoi(room_number));
            }
        }
        else {
            msg = "[공지] " + sck_list[idx].user + " 님이 퇴장했습니다.";
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