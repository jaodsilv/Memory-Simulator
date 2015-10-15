#include <bits/stdc++.h>
using namespace std;

#define MAX_VIR_PAGES 1024
#define MAX_TOT_PAGES 256
#define MIN_PROCESS 10
#define MAX_PROCESS_VARIATION 21
#define MAX_INIT_TIME 30
#define MAX_DUR_TIME 90

int main(int argc, char const *argv[])
{
	srand (time(NULL));
	int pages_tot = rand() % MAX_TOT_PAGES + 1;
	int pages_vir = rand() % (MAX_VIR_PAGES - pages_tot) + pages_tot + 1;
	cout << pages_tot*16 << " " << pages_vir*16 << endl;
	int process = rand() % MAX_PROCESS_VARIATION + MIN_PROCESS;
	long long int total_b = 0;
	for (int i = 0; i < process; ++i) {
		int t0 = rand() % MAX_INIT_TIME;
		int tf = rand() % MAX_DUR_TIME + t0 + 1;
		int b = rand() % (pages_tot/5);
		total_b += b + 1;
		b = b * 16 + rand() % 16 + 1;
		cout << t0 << " process" << i << " " << tf << " " << b;
		int t = t0;
		t = rand() % 5 + t;
		while (t < tf) {
			// cout << " " << t << " " << t0 << " " << tf << endl;
			cout << " " << rand() % b << " " << t;
			t = rand() % 5 + t;
		}
		cout << endl;
	}

	cerr << "Total pages " << pages_tot << endl;
	cerr << "Virtual pages " << pages_vir << endl;
	cerr << "total needed " << total_b << endl;
	return 0;
}
