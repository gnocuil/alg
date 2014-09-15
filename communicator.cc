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
    Parser::run(this);
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
    printf("fa: write data ADDDATA\n");
    int cnt = write(fd_data[1], &c, sizeof(c));
    printf("fa: write cnt=%d\n", cnt);
    do {
        printf("fa: reading ans(addData)\n");
        int cnt = read(fd_ans[0], &c, sizeof(c));
        printf("fa: read ans(addData) %d cnt=%d\n", c, cnt);
        if (!cnt) return;
        switch (c) {
        case MOREDATA://parser has done
            puts("addData::MOREDATA");
            break;
        case QUIT:
            puts("child thread has quit, cancel");
            return;
        default:
            break;
        }
    } while (0);
}
std::istream* StatefulCommunicator::getIStream() {
    COMMAND c = MOREDATA;
        printf("son: write ans MOREDATA\n");
    write(fd_ans[1], &c, sizeof(c));
        printf("son: reading data\n");
    read(fd_data[0], &c, sizeof(c));
        printf("son: read data %d\n", c);
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

void StatefulCommunicator::init() {
    running_ = true;
    cout << "create thread"<<endl;
    pthread_create(&tid, NULL, _thread_t, this);
    COMMAND c;
    do {
        printf("father: reading ans\n");    
        read(fd_ans[0], &c, sizeof(c));
        printf("father: read ans %d\n", c);
        switch (c) {
        case MOREDATA://parser has done
            puts("init::MOREDATA!");
            break;
        default:
            ;
        }
    } while (0);
}

void StatefulCommunicator::run() {
    try {
        Parser::run(this);
    } catch (const std::exception &e) {
        cerr << "In thread " << tid << " : caught exception [" << e.what() << "], quit." << endl;
    }
    COMMAND c = QUIT;
    write(fd_ans[1], &c, sizeof(c));
    close(fd_ans[1]);
}

void* StatefulCommunicator::_thread_t(void* param) {
    StatefulCommunicator* com = (StatefulCommunicator*)param;
    com->run();
    cerr << "!!! Thread " << com->tid << " QUIT !!!" << endl;
    return NULL;
}

