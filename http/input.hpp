//#include "HTTPScannerbase.h"
void HTTPScannerBase::Input::resolve() {
    d_in->read((char*)(&buf_), sizeof(Communicator*));
    puts("Input::resolve!");
}

std::istream* HTTPScannerBase::Input::getNewIstream() {puts("getNewIstream!!!");
    Communicator *c = (Communicator*)buf_;
    return c->getIStream();
}
