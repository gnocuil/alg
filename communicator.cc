#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <stdexcept>
#include "communicator.h"
#include "parser.h"

using namespace std;

void StatelessCommunicator::addData(const std::string& data) {
    data_ = data;
}
std::istream* StatelessCommunicator::getIStream() {
    return new istringstream(data_);
}

StatefulCommunicator::StatefulCommunicator() : running_(false) {
    int r;
    r = pipe (fd_data);
    r = pipe (fd_ans);//TODO: check pipe() failure
}

void StatefulCommunicator::addData(const std::string& data) {
    data_ = data;
    if (!running_) {
        init();
    }
    COMMAND c = ADDDATA;
    write(fd_data[1], &c, sizeof(c));
    do {
        read(fd_ans[0], &c, sizeof(c));
        switch (c) {
        case MOREDATA://parser has done
            break;
        default:
            break;
        }
    } while (1);
}
std::istream* StatefulCommunicator::getIStream() {cout<<"getIStream succ!"<<endl;
    test(3);
    COMMAND c;
    read(fd_data[0], &c, sizeof(c));
    cout << "received cmd " << c << endl;
    switch (c) {
    case ADDDATA:
//        cout << "add '" << data_ << "'" << endl;
        cout << "add \n";
        break;
    case QUIT:
        throw runtime_error("QUIT!");
    default:
        throw runtime_error("Unknown cmd :" + c);
    }
    return new istringstream(data_);
}

void StatefulCommunicator::test(int a) {
    cout << "\t" << a << " :fd_data[0,1]=" << fd_data[0] << " " << fd_data[1];printf("   #%p\n", (void*)this);
}

void StatefulCommunicator::init() {
    running_ = true;
    test(1);
    cout << "create thread"<<endl;
    pthread_create(&tid, NULL, _thread_t, this);
}

void StatefulCommunicator::run() {
    try {test(2);
        Parser::run(this);
    } catch (const std::exception &e) {
        cerr << "In thread TODO: caught exception [" << e.what() << "], quit." << endl;
    }
    
}

void* StatefulCommunicator::_thread_t(void* param) {
    StatefulCommunicator* com = (StatefulCommunicator*)param;
    com->run();
    cerr << "!!! Thread QUIT !!!" << endl;
    return NULL;
}

