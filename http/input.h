class Input {
    std::deque<unsigned char> d_deque;  // pending input chars
    std::istream *d_in;                 // ptr for easy streamswitching
    size_t d_lineNr;                    // line count

public:
    Input()
    : d_in(0),
      d_lineNr(1) {
    }
    
    Input(std::istream *iStream, size_t lineNr = 1)
    : d_in(iStream),
      d_lineNr(lineNr) {
    }
    
    
    size_t get() {                   // the next range
        switch (size_t ch = next())         // get the next input char
        {
            case '\n':
                ++d_lineNr;
            // FALLING THROUGH

            default:
            return ch;
        }
    }
    void reRead(size_t ch) {         // push back 'ch' (if < 0x100)
        if (ch < 0x100)              // push back str from idx 'fmIdx'
        {
            if (ch == '\n')
                --d_lineNr;
            d_deque.push_front(ch);
        } 
    }
    
    void reRead(std::string const &str, size_t fmIdx) {
        for (size_t idx = str.size(); idx-- > fmIdx; )
            reRead(str[idx]);
    }
    
    size_t lineNr() const { return d_lineNr; }
    
    void close() {                  // force closing the stream
        delete d_in;
        d_in = 0;                   // switchStreams also closes
    }

private:
    size_t next() {                  // obtain the next character
        size_t ch;

        if (d_deque.empty())                    // deque empty: next char fm d_in
        {
            if (d_in == 0)
                return AT_EOF;
            ch = d_in->get();
/*            if (!(*d_in)) {
                static int cnt = 0;
                if (cnt == 0) {
                    ++cnt;
                    delete d_in;
                    d_in = new std::istringstream("89");
                    ch = d_in->get();
                }
            }*/
            return *d_in ? ch : static_cast<size_t>(AT_EOF);
        }

        ch = d_deque.front();
        d_deque.pop_front();
        return ch;
    }
};

