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

const int T = 10000;

int size[TESTS] = {
//    100, 200, 300,400,500,600,700,800,900,1000
    10, 20, 30,40,50,60,70,80,90,100
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
	ops.clear();
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
//        op.newdata = "10,0,0,2";
//        if (i * 3 > n && rand() % 10 == 0)
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
    //printf("len_before=%d\n", len);
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

	for (pd = ps = 0; ; ) {
		if (cnt == ops.size()) {
			memcpy(d + pd, s + ps, len_old - ps);
			break;
		}
	    if (ps == ops[cnt].start_pos) {
	    	int sz = ops[cnt].newdata.size();
	    	memcpy(d + pd, ops[cnt].newdata.c_str(), sz);
	    	pd += sz;
	    	ps = ops[cnt++].end_pos;
	    }
	    else {
	    	int sz = ops[cnt].start_pos - ps;
	    	memcpy(d + pd, s + ps, sz);
	    	pd += sz;
	    	ps += sz;
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

	for (pd = ps = 0; ; ) {
		if (cnt == ops.size()) {
			memcpy(d + pd, s + ps, len_old - ps);
			break;
		}
	    if (ps == ops[cnt].start_pos) {
	    	int sz = ops[cnt].newdata.size();
	    	memcpy(d + pd, ops[cnt].newdata.c_str(), sz);
	    	pd += sz;
	    	ps = ops[cnt++].end_pos;
	    }
	    else {
	    	int sz = ops[cnt].start_pos - ps;
	    	memcpy(d + pd, s + ps, sz);
	    	pd += sz;
	    	ps += sz;
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
	srand(time(NULL));
    ofstream fout("result.csv");
        fout << "#opt"<<","<<"Optimized"<<","<<"Optimized-No sort"<<","<<"original"<<endl;
    double y[TESTS][3];
    int x[TESTS];
    

    
    for (int i = 0; i < TESTS; ++i) {
        double time1 = 0;
        double time2 = 0;
        double time3 = 0;
        
        //opt_sort

        struct timeval t1;
        struct timeval t2;        
        for (int j = 0; j < T; ++j) {
        	generate(size[i]);
	        randswap();
            //ops = ops2;
            gettimeofday(&t1, NULL);  
            modify();
	        gettimeofday(&t2, NULL);  
	        time1 += 1.0*gettime(t1, t2);
        }
        time1 /= T;
        
        //opt-nosort
        for (int j = 0; j < T; ++j) {
        	generate(size[i]);
            //ops = ops2;
	        gettimeofday(&t1, NULL);  
            modify_nosort();
	        gettimeofday(&t2, NULL);  
	        time2 += 1.0*gettime(t1, t2);
        }

        time2 /= T;        
        
        //slow
//        randswap();
        for (int j = 0; j < T; ++j) {
            //ops = ops2;
        	generate(size[i]);
	        randswap();
	        gettimeofday(&t1, NULL);  
            modify_slow();
	        gettimeofday(&t2, NULL);  
	        time3 += 1.0*gettime(t1, t2);
        }
        time3 /= T;
        
        cout << "#" << i+1 << "  len=" << len << "  size=" << size[i] << "  time1="<<time1<<"   time2="<<time2<<"   time3="<<time3<<endl;
//        fout << size[i]<<","<<log(time1)<<","<<log(time2)<<","<<log(time3)<<endl;
		x[i] = size[i];
        y[i][0] = 1000000/(time1);
        y[i][1] = 1000000/(time2);
        y[i][2] = 1000000/(time3);
        fout << size[i]<<","<<y[i][0]<<","<<y[i][1]<<","<<y[i][2]<<endl;

    }
    cout << "x=[";
    for (int i = 0; i < TESTS; ++i) cout << x[i] << " ";
    cout << "];"<<endl;
    cout << "y=[";
    for (int i = 0; i < TESTS; ++i) cout << y[i][0] << " "; cout << ";"<<endl;
    for (int i = 0; i < TESTS; ++i) cout << y[i][1] << " "; cout << ";"<<endl;
    for (int i = 0; i < TESTS; ++i) cout << y[i][2] << " "; cout << ";"<<endl;
    cout << "];"<<endl;
//    print();
}
