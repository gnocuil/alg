#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <sys/time.h>
#include <fstream>
#include <cmath>
using namespace std;
const int LEN = 10000000;
const int TESTS = 10;
char s[LEN];
char d[LEN];
int len;

const int T = 40;

int size[TESTS] = {
    100, 200, 300,400,500,600,700,800,900,1000
};

class Operation {
public:
    Operation()
        : cnt(false)
    {}
    enum OP {
        REPLACE
    };
    int start_pos;
    int end_pos;//replace data[start_pos,end_pos) with new data
    OP op;
    std::string newdata;
    bool cnt;
};
bool operator<(const Operation& a, const Operation& b) {
    return a.start_pos < b.start_pos;
}

std::vector<Operation> ops;
std::vector<Operation> ops2;

static long long gettime(struct timeval t1, struct timeval t2) {
    return (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec) ;
}

void generate(int n)
{
    string buf;
    buf += "<!DOCTYPE HTML>\r\n";
    buf += "<html>\r\n";
    buf += "<body>\r\n";
    buf += "<h1>My First Heading</h1>\r\n";
    buf += "<p>My first paragraph.</p>\r\n";
    for (int i = 0; i < size[TESTS-1]; ++i) {
        Operation op;
        op.start_pos = buf.size() + 19;
        op.end_pos = buf.size() + 19 + 8;
        buf += "<p><a href=\"http://10.0.0.2/test/test.html\">Test Page</a></p>\r\n";
        op.newdata = "10,0,0,2";
        if (i * 3 > n && rand() % 10 == 0)
            op.newdata = "10.0.0.2.tld";
//        string test(buf.begin() + op.start_pos, buf.begin() + op.end_pos);
//        cout<<test<<endl;
//        printf("op: %s\n", string(buf.begin() + op.start_pos, buf.begin() + op.end_pos).c_str());
        if (i < n)
            ops.push_back(op);
    }
    buf += "/<html>\r\n";
    buf += "/<body>\r\n";
    len = buf.size();
    printf("len_before=%d\n", len);
    memcpy(s, buf.c_str(), len);
}

int modify_slow() {
    sort(ops.begin(), ops.end());
    memcpy(d, s, len);
    for (int i = ops.size() - 1; i >= 0; --i) {
        int delta = ops[i].newdata.size() - (ops[i].end_pos - ops[i].start_pos);
        if (delta > 0) {
            for (int j = len + delta - 1; j - delta >= ops[i].start_pos; --j)
                d[j] = d[j - delta];
        }
        memcpy(d + ops[i].start_pos, ops[i].newdata.c_str(), ops[i].newdata.size());
    }
}

int modify_nosort()
{
    if (ops.size() == 0)
        return 0;
	int len_old = len;
	
	for (int i = 0; i < ops.size(); ++i) {
	    //printf("replace : %d %d [", ops[i].start_pos, ops[i].end_pos);
	    //printf("] with <%s>\n", ops[i].newdata.c_str());
	    int delta = ops[i].newdata.size() - (ops[i].end_pos - ops[i].start_pos);
	    len += delta;
	}
	//printf("newlen=%d\n", len);
	int cnt = 0, pd, ps;
	bool inside = false;
	for (pd = ps = 0; pd < len; ++pd) {
	    if (!inside && ps == ops[cnt].start_pos) {
	        if (ops[cnt].newdata.size() == 0) {
	            ps = ops[cnt++].end_pos;
	        } else {
    	        inside = true;
	            ps = 0;
	        }
	    }
	    if (inside) {
	        d[pd] = ops[cnt].newdata[ps++];
	        if (ps == ops[cnt].newdata.size()) {
	            inside = false;
	            ps = ops[cnt++].end_pos;
	        }
	    } else {
	        d[pd] = s[ps++];
	    }
	}
	
}

int modify()
{
    if (ops.size() == 0)
        return 0;
    sort(ops.begin(), ops.end());//sort ?

	int len_old = len;
	
	for (int i = 0; i < ops.size(); ++i) {
	    //printf("replace : %d %d [", ops[i].start_pos, ops[i].end_pos);
	    //printf("] with <%s>\n", ops[i].newdata.c_str());
	    int delta = ops[i].newdata.size() - (ops[i].end_pos - ops[i].start_pos);
	    len += delta;
	}
	//printf("newlen=%d\n", len);
	int cnt = 0, pd, ps;
	bool inside = false;
	for (pd = ps = 0; pd < len; ++pd) {
	    if (!inside && ps == ops[cnt].start_pos) {
	        if (ops[cnt].newdata.size() == 0) {
	            ps = ops[cnt++].end_pos;
	        } else {
    	        inside = true;
	            ps = 0;
	        }
	    }
	    if (inside) {
	        d[pd] = ops[cnt].newdata[ps++];
	        if (ps == ops[cnt].newdata.size()) {
	            inside = false;
	            ps = ops[cnt++].end_pos;
	        }
	    } else {
	        d[pd] = s[ps++];
	    }
	}
	
}

void randswap() {
    for (int i = 1; i < ops.size()/10; ++i)
        swap(ops[i], ops[rand() % i]);
    ops2 = ops;
}

void print() {
    for (int i = 0; i < len; ++i) cout << d[i];
}


int main()
{
    ofstream fout("result.csv");
        fout << "#opt"<<","<<"Optimized"<<","<<"Optimized-No sort"<<","<<"original"<<endl;
    for (int i = 0; i < TESTS; ++i) {
        generate(size[i]);
        
        //opt_sort
        randswap();
        struct timeval t1;
        gettimeofday(&t1, NULL);  
        for (int j = 0; j < T; ++j) {
            ops = ops2;
            modify();
        }
        struct timeval t2;
        gettimeofday(&t2, NULL);  
        double time1 = 1.0*gettime(t1, t2) / T;
        
        //opt-nosort
        gettimeofday(&t1, NULL);  
        for (int j = 0; j < T; ++j) {
            ops = ops2;
            modify_nosort();
        }
        gettimeofday(&t2, NULL);  
        double time2 = 1.0*gettime(t1, t2) / T;        
        
        //slow
        randswap();
        gettimeofday(&t1, NULL);  
        for (int j = 0; j < T; ++j) {
            ops = ops2;
            modify_slow();
        }
        gettimeofday(&t2, NULL);  
        double time3 = 1.0*gettime(t1, t2) / T;
        
        cout << "#" << i+1 << "  len=" << len << "  size=" << size[i] << "  time1="<<time1<<"   time2="<<time2<<"   time3="<<time3<<endl;
//        fout << size[i]<<","<<log(time1)<<","<<log(time2)<<","<<log(time3)<<endl;
        fout << size[i]<<","<<1000000/(time1)<<","<<1000000/(time2)<<","<<1000000/(time3)<<endl;
    }
//    print();
}
