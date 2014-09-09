#pragma once

#include <sstream>
#include <string>

class Communicator {
public:
    virtual void addData(const std::string& data) = 0;
    virtual std::istream* getIStream() = 0;
    virtual void test(int a) = 0;
protected:
    std::string data_;
};

class StatelessCommunicator : public Communicator {
public:
    virtual void addData(const std::string& data);
    virtual std::istream* getIStream();
    virtual void test(int a) {}
};

class StatefulCommunicator : public Communicator {
public:
    StatefulCommunicator();
    virtual void addData(const std::string& data);
    virtual std::istream* getIStream();
    virtual void test(int a);
private:
    enum COMMAND {
        ADDDATA,
        MOREDATA,
        QUIT
    };
    void init();
    static void* _thread_t(void* param);
    void run();
    
    bool running_;
    int fd_data[2];
    int fd_ans[2];
    pthread_t tid;
};

