#include <time.h>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <future>
#include <iomanip>
#include <mutex>
#include <wiringPi.h>
#include <softPwm.h>

using namespace std;
// using std::atomic;
using std::mutex;

using system_clock = chrono::system_clock;

std::mutex g_mutex;

#include "IAgoraService.h"
#include "IAgoraRtmService.h"

#define RESET "\033[0m"
#define BLACK "\033[30m"              /* Black */
#define RED "\033[31m"                /* Red */
#define GREEN "\033[32m"              /* Green */
#define YELLOW "\033[33m"             /* Yellow */
#define BLUE "\033[34m"               /* Blue */
#define MAGENTA "\033[35m"            /* Magenta */
#define CYAN "\033[36m"               /* Cyan */
#define WHITE "\033[37m"              /* White */
#define BOLDBLACK "\033[1m\033[30m"   /* Bold Black */
#define BOLDRED "\033[1m\033[31m"     /* Bold Red */
#define BOLDGREEN "\033[1m\033[32m"   /* Bold Green */
#define BOLDYELLOW "\033[1m\033[33m"  /* Bold Yellow */
#define BOLDBLUE "\033[1m\033[34m"    /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define BOLDCYAN "\033[1m\033[36m"    /* Bold Cyan */
#define BOLDWHITE "\033[1m\033[37m"   /* Bold White */

int PWMA = 1;
int AIN2 = 2;
int AIN1 = 3;

int PWMB = 4;
int BIN2 = 5;
int BIN1 = 6;

using namespace agora::rtm;

void t_up(unsigned int speed, unsigned int t_time)
{
    digitalWrite(AIN2, 0);
    digitalWrite(AIN1, 1);
    softPwmWrite(PWMA, speed);

    digitalWrite(BIN2, 0);
    digitalWrite(BIN1, 1);
    softPwmWrite(PWMB, speed);
    delay(t_time);
}

void t_stop(unsigned int t_time)
{
    digitalWrite(AIN2, 0);
    digitalWrite(AIN1, 0);
    softPwmWrite(PWMA, 0);

    digitalWrite(BIN2, 0);
    digitalWrite(BIN1, 0);
    softPwmWrite(PWMB, 0);
    delay(t_time);
}

void t_down(unsigned int speed, unsigned int t_time)
{
    digitalWrite(AIN2, 1);
    digitalWrite(AIN1, 0);
    softPwmWrite(PWMA, speed);

    digitalWrite(BIN2, 1);
    digitalWrite(BIN1, 0);
    softPwmWrite(PWMB, speed);
    delay(t_time);
}

void t_left(unsigned int speed, unsigned int t_time)
{
    digitalWrite(AIN2, 1);
    digitalWrite(AIN1, 0);
    softPwmWrite(PWMA, speed);

    digitalWrite(BIN2, 0);
    digitalWrite(BIN1, 1);
    softPwmWrite(PWMB, speed);
    delay(t_time);
}

void t_right(unsigned int speed, unsigned int t_time)
{
    digitalWrite(AIN2, 0);
    digitalWrite(AIN1, 1);
    softPwmWrite(PWMA, speed);

    digitalWrite(BIN2, 1);
    digitalWrite(BIN1, 0);
    softPwmWrite(PWMB, speed);
    delay(t_time);
}

class DemoChannel : IChannelEventHandler
{
private:
    bool joined_;
    DemoChannel() : joined_(false){};
    ~DemoChannel(){};

public:
    virtual void onJoinSuccess() override
    {
        joined_ = true;
        cout << "join success" << endl;
    }
    virtual void onJoinFailure(JOIN_CHANNEL_ERR errorCode) override
    {
        cout << "join failure, error: " << errorCode << endl;
    }
    virtual void onLeave(LEAVE_CHANNEL_ERR errorCode) override
    {
        cout << "on leave, error: " << errorCode << endl;
    }
    virtual void onMessageReceived(const char *userId, const IMessage *message) override
    {
        // cout << "on message received, error: " << errorCode << endl;
    }
    virtual void onSendMessageResult(long long messageId, CHANNEL_MESSAGE_ERR_CODE state) override
    {
    }
    virtual void onMemberJoined(IChannelMember *member) override
    {
    }
    virtual void onMemberLeft(IChannelMember *member) override
    {
    }
    virtual void onGetMembers(IChannelMember **members, int userCount, GET_MEMBERS_ERR errorCode) override
    {
    }
};


class DemoMessaging : IRtmServiceEventHandler, IChannelEventHandler
{
private:
    string APP_ID;
    string userAccount;
    bool online_;
    int code_ = 0;
    bool logouted_;
    bool assigned_start_time;
    bool assigned_end_time;
    std::chrono::time_point<system_clock> _received_ts;

public:
    DemoMessaging(string appId, string account)
        : APP_ID(appId),
          userAccount(account),
          online_(false),
          logouted_(false),
          channel_("agora.pi-server2"),
          agoraService_(createAgoraService()),
          stop_(false),
          direction(""),
          duration(0.4),
          assigned_start_time(false),
          assigned_end_time(false),
          _received_ts(system_clock::now()),
          rtmService_(nullptr) { Init(); }

    void Init()
    {
        if (!agoraService_)
        {
            cout << RED << "error creating agora service!" << endl;
            exit(0);
        }
        int ret = agoraService_->initialize(context_);
        if (ret)
        {
            cout << RED << "error initializing agora service: "
                 << ret << endl;
            exit(0);
        }
        rtmService_ = agoraService_->createRtmService();
        if (!rtmService_)
        {
            cout << RED << "error creating rtm service!" << endl;
            exit(0);
        }
        ret = rtmService_->initialize(APP_ID.c_str(), this);
        if (ret)
        {
            cout << RED << "APPID " << APP_ID << " user " << userAccount << endl;
            cout << RED << "error initializing rtm service: " << ret << endl;
            exit(0);
        }

        wiringPiSetup();
        pinMode(1, OUTPUT);
        pinMode(2, OUTPUT);
        pinMode(3, OUTPUT);

        pinMode(4, OUTPUT);
        pinMode(5, OUTPUT);
        pinMode(6, OUTPUT);

        softPwmCreate(PWMA, 0, 100);
        softPwmCreate(PWMB, 0, 100);
    }

    void login()
    {
        int ret = rtmService_->login(APP_ID.c_str(), userAccount.c_str());
        cout << "login ret:" << ret << endl;
        if (ret)
        {
            login();
        }
    }

    void logout()
    {
        if (!logouted_)
        {
            int ret = rtmService_->logout();
            cout << "logout ret: " << ret << endl;
            if (ret)
            {
                logout();
            }
            else
            {
                logouted_ = true;
            }
        }
    }

    // MAIN LOOP
    void run()
    {
        login();
        joinChannel();
        cout << "login invoked" << endl;
        while (true)
        {
            // sleep(1);
            // sendMessageToPeer("agora.pi-client", "ping");
            std::lock_guard<std::mutex> lock(g_mutex);
            auto current = system_clock::now();
            auto current_t = std::chrono::system_clock::to_time_t(current);
            std::chrono::duration<double> sec = current - _received_ts;
            double secs = sec.count();
            // cout << "elapsed secs: " << std::setprecision(5) << secs << endl;
            if (secs >= 1.0) {
                _received_ts = system_clock::now();
                auto _t = system_clock::to_time_t(_received_ts);
                cout << "positive stopped elapsed_secs: " << ctime(&_t) << endl;
                t_stop(0);
            }
        }
    }

    int sendMessageToPeer(string peerID, string message)
    {
        IMessage *rtmMessage = rtmService_->createMessage();
        rtmMessage->setText(message.c_str());
        int ret = rtmService_->sendMessageToPeer(peerID.c_str(), rtmMessage);
        cout << "\033[1;31m " << "sendMessageToPeer ret:" << ret << "\033[0m"<< endl;
        return ret;
    }

private:
    unique_ptr<agora::base::IAgoraService> agoraService_;
    agora::base::AgoraServiceContext context_;
    // unique_ptr<IRtmServiceEventHandler> eventHandler_;
    IRtmService *rtmService_;

    std::chrono::time_point<system_clock>
        start_time, end_time;
    string direction;
    double duration;

private:
    void handleCommand(vector<string> &messages)
    {
        std::string::size_type sz;
        auto cmd = messages[0];
        auto speed = stoi(messages[1], &sz);

        if (cmd == direction &&
            assigned_start_time) {
            end_time = system_clock::now();
            assigned_end_time = true;
            chrono::duration<double> _duration = end_time - start_time;
            cout << " set end time: " << direction << " elapsed " << _duration.count() << endl;
            if (duration >= _duration.count()) {
                cout << "duration greater than 1 seconds " << endl;
                time_t a = system_clock::to_time_t(end_time),
                    b = system_clock::to_time_t(start_time);
                cout << "\033[1;31m " << "end_time " << std::ctime(&a) << "start_time " << std::ctime(&b) << "\033[0m"<< endl;
                cout << "\033[1;31m return program\033[0m" << endl;
                return ;
            } else {
                time_t a = system_clock::to_time_t(end_time),
                    b = system_clock::to_time_t(start_time);
                assigned_end_time = assigned_start_time = false;
                cout << "\033[1;31m return program with " << "end_time " << std::ctime(&b) << "start_time " << std::ctime(&a) << "\033[0m"<< endl;
                // return;
                t_stop(0);
            }
        }

        if (cmd != direction) {
            assigned_end_time = assigned_start_time = false;
            direction = cmd;
            cout << "refresh end & start time: " << direction << endl;
            t_stop(0);
        }

        cout << "\033[1;31m" <<  "prepare handle direction: " << direction << "\033[0m" <<  endl;
        if (cmd == "up")
        {
            t_up(speed, 0);
            cout << "\033[1;31m " << " move up" << "\033[0m" << endl;
            // sleep(1);
            // t_stop(0);
            cout << "\033[1;31m " << " move up stopped" << "\033[0m" << endl;
            cout << "move up" << endl;
        }
        else if (cmd == "down")
        {
            t_down(speed, 0);
            cout << "\033[1;31m " << " move down" << "\033[0m" << endl;
            // t_up(speed, 0);
            // sleep(1);
            // t_stop(0);
            cout << "\033[1;31m " << " move down stopped" << "\033[0m" << endl;
            cout << "move down" << endl;
        }
        else if (cmd == "left")
        {
            t_left(speed, 0);
            cout << "\033[1;31m " << " move left" << "\033[0m" << endl;
            // sleep(1);
            // t_stop(0);
            cout << "\033[1;31m " << " move left stopped" << "\033[0m" << endl;
            cout << "move left" << endl;
        }
        else if (cmd == "right")
        {
            t_right(speed, 0);
            cout << "\033[1;31m " << " move right" << "\033[0m" << endl;
            // sleep(1);
            // t_stop(0);
            cout << "\033[1;31m " << " move right stopped" << "\033[0m" << endl;
            cout << "move right" << endl;
        }
        else if (cmd == "stop")
        {
            t_stop(0);
            cout << "move stop" << endl;
        }

        if (cmd != "") {
            direction = cmd;
            if (!assigned_start_time) {
                start_time = system_clock::now();
                assigned_start_time = true;
                cout << "set start time: " << cmd << endl;
            }
        }

        cout << "handleCommand invoked" << endl;
    }

public:
    virtual void onLoginSuccess() override
    {
        online_ = true;
        cout << "login success" << endl;
        cout << "change online " << boolalpha << online_ << endl;
    }

    virtual void onLoginFailure(LOGIN_ERR_CODE errorCode) override
    {
        online_ = false;
        cout << "change online: " << boolalpha << online_ << endl;
    }

    virtual void onLogout(LOGOUT_ERR_CODE errorCode) override
    {
        online_ = false;
        cout << "change online: " << boolalpha << online_ << endl;
    }

    virtual void onConnectionStateChanged(CONNECTION_STATE state,
                                          CONNECTION_CHANGE_REASON reason)
        override
    {
        cbPrint("onConnectionStateChanged: %d", state);
    }

    virtual void onSendMessageResult(long long messageId,
                                     PEER_MESSAGE_ERR_CODE errorCode) override
    {
        cbPrint("onSendMessageResult messageID: %llderrorcode: %d",
                messageId, errorCode);
    }

    void onMessageReceivedFromPeer(const char *peerId,
                                   const IMessage *message) override
    {
        if (!peerId || !message)
        {
            return;
        }
        std::lock_guard<std::mutex> lock(g_mutex);
        _received_ts = system_clock::now();
        string text = message->getText();
        cout << "receive :" << text << endl;
        const char delimiter = ',';
        std::vector<std::string> messages;
        std::replace(text.begin(), text.end(), delimiter, ' ');
        stringstream ss(text);
        string tmp;
        while (ss >> tmp)
            messages.push_back(tmp);
        if (messages[0].compare("pi") == 0)
        {
            return;
        }
        handleCommand(messages);
    }

    void SetChannel(const std::string &channel)
    {
        channel_ = channel;
    }

private:
    bool stop_;

public:
    virtual void onJoinSuccess() override
    {
        cbPrint(">>>>>>>>>>>>>>>>>>>>>>> join %s channel succeed", channel_);
    }

    virtual void onJoinFailure(agora::rtm::JOIN_CHANNEL_ERR errorCode)
        override
    {
        cbPrint("join channel failed errorcode: ", errorCode);
    }

    virtual void onLeave(agora::rtm::LEAVE_CHANNEL_ERR errorCode) override
    {
        cbPrint("leave channel reason code: ", errorCode);
    }

    virtual void onMessageReceived(const char *userId,
                                   const agora::rtm::IMessage *message) override
    {
        stop_ = true;
        // t_stop(0);
        cbPrint("receve channel message");
    }

    virtual void onMemberJoined(agora::rtm::IChannelMember *member) override
    {
        cbPrint("channel member joined channel:%s member:%s ",
                member->getChannelId(), member->getUserId());
    }

    virtual void onMemberLeft(agora::rtm::IChannelMember *member) override
    {
        cbPrint("channel member left channel:%s member:%s ",
                member->getChannelId(), member->getUserId());
    }

    virtual void onGetMembers(agora::rtm::IChannelMember **members,
                              int userCount,
                              agora::rtm::GET_MEMBERS_ERR errorCode) override
    {
        cbPrint("show channel:%s members num:%d", channel_.c_str(), userCount);
        for (int index = 0; index < userCount; index++)
        {
            cbPrint(" member :%s", members[index]->getUserId());
        }
    }

    void joinChannel()
    {
        agora::rtm::IChannel *channelHandler =
            rtmService_->createChannel(channel_.c_str(), this);
        if (channelHandler == nullptr)
        {
            std::cout << RED << "err create channel failed." << std::endl;
        }
        cout << "JOIN >>>>>>>>>>>: " << channelHandler->join() << endl;
    }

private:
    std::string channel_;

private:
    void cbPrint(const char *fmt, ...)
    {
        printf("\x1b[32m<< RTM async callback: ");
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf(" >>\x1b[0m\n");
    }
};