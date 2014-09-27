//#include "HTTPScannerbase.h"
void HTTPScannerBase::Input::resolve() {
    d_in->read((char*)(&buf_), sizeof(Communicator*));
}

std::istream* HTTPScannerBase::Input::getNewIstream() {
    Communicator *c = (Communicator*)buf_;
    return c->getIStream();
}
