#include <iostream>
#include <vector>

using namespace std;

void test(const vector<float>& demo, float factor)
{
	float idx = 0.;

	cout<<"===========[ factor "<<factor<<" ]==============="<<endl;

	cout<<"  ---- load --- idx ------"<<endl;
	for (float v: demo)
	{
		idx = idx * factor + v;
		cout<<"  -- "<<v<<" ---- "<<idx<<endl;
	}
}

int main()
{
	vector<float> demo{ 0., 0.7, 1, 1, 1, 1, 0.8, 1, 1, 1, 0.8, 0.6, 0.7, 0.9, 1, 1, 1, 0.6, 0.4, 0.2, 0.1, 0.1, 0.1, 0.1, 0.9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.2, 0.1, 0.2, 0.1 };	

	test(demo, 0.3);
	test(demo, 0.5);
	test(demo, 0.7);
	test(demo, 0.85);

	return 0;
}
