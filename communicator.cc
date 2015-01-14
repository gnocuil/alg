#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <stdexcept>
#include "communicator.h"
#include "parser.h"

using namespace std;

std::istream& Communicator::getAddrIStream() {
    Communicator* c = this;
    std::string str((char*)(&c), sizeof(Communicator*));
    sin.str(str);
    return sin;
}

int StatelessCommunicator::addData(const std::string& data) {//cout<<"stateless addData size="<<data.size()<<endl;
    data_ = data;
    Parser::run(this);
    return 0;
}
std::istream* StatelessCommunicator::getIStream() {
    std::istream* ret = NULL;
    if (data_.size() > 0) {
        ret = new istringstream(data_);
        data_ = "";
    }
    return ret;
}

StatefulCommunicator::StatefulCommunicator()
    : running_(false),
      total_len(0) {
    int r;
    r = pipe (fd_data);
    r = pipe (fd_ans);//TODO: check pipe() failure
}

int StatefulCommunicator::addData(const std::string& data) {cout<<"addData size="<<data.size()<<endl;
    data_ = data;
    if (!running_) {
        init();
    }
    COMMAND c = ADDDATA;
//    printf("fa: write data ADDDATA\n");
    int cnt = write(fd_data[1], &c, sizeof(c));
//    printf("fa: write cnt=%d\n", cnt);
    do {
//        printf("fa: reading ans(addData)\n");
        int cnt = read(fd_ans[0], &c, sizeof(c));
//        printf("fa: read ans(addData) %d cnt=%d\n", c, cnt);
        if (!cnt) return -1;
        switch (c) {
        case MOREDATA://parser has done
//            puts("addData::MOREDATA");
            break;
        case QUIT:
            //puts("addData(): child thread has quit, cancel");
            return -1;
        default:
            break;
        }
    } while (0);
    return 0;
}
std::istream* StatefulCommunicator::getIStream() {puts("try to getIStream()");
    //check if the packet should end
    Parser *parser = Parser::getParser(this);
    if (parser && parser->endPos > 0) {
//        printf("getIStream check: endPos=%d total=%d\n", parser->endPos, total_len);
        if (parser->endPos <= total_len) {//end now
            COMMAND c = QUIT;
//            printf("getIStream: endPos has reached, quit!\n");
            int result = write(fd_ans[1], &c, sizeof(c));
            throw runtime_error("endPos reached, QUIT!");
        }
    }

    COMMAND c = MOREDATA;
//        printf("son: write ans MOREDATA\n");
    int result = write(fd_ans[1], &c, sizeof(c));
//        printf("son: reading data\n");
    result = read(fd_data[0], &c, sizeof(c));
//        printf("son: read data %d\n", c);
    switch (c) {
    case ADDDATA:
        break;
    case QUIT:
        throw runtime_error("QUIT!");
    default:
        throw runtime_error("Unknown cmd :" + c);
    }
    total_len += data_.size();
    return new istringstream(data_);
}

void StatefulCommunicator::init() {
    running_ = true;
    //cout << "create thread"<<endl;
    pthread_create(&tid, NULL, _thread_t, this);
    COMMAND c;
    do {
//        printf("father: reading ans\n");    
        int result = read(fd_ans[0], &c, sizeof(c));
//        printf("father: read ans %d\n", c);
        switch (c) {
        case MOREDATA://parser has done
//            puts("init::MOREDATA!");
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
        //cerr << "In thread " << tid << " : caught exception [" << e.what() << "], quit." << endl;
    }
    COMMAND c = QUIT;
    int result = write(fd_ans[1], &c, sizeof(c));
    close(fd_ans[1]);
}

void* StatefulCommunicator::_thread_t(void* param) {
    StatefulCommunicator* com = (StatefulCommunicator*)param;
    com->run();
    //cerr << "!!! Thread " << com->tid << " QUIT !!!" << endl;
    return NULL;
}

