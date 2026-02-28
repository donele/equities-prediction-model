#include <cstdio>
#include <chrono>
#include <vector>
#include <numeric>
#include <array>
#include <deque>

namespace {
	void on_stack()
	{
		int i;
		i = 99;
	}
	void on_heap()
	{
		int* i = new int;
		delete i;
		//std::vector<int> v(1);
	}
}

int main()
{
	const static int N = 1000000000;

	auto begin = std::chrono::system_clock::now();
	{
	}
	auto end = std::chrono::system_clock::now();
	std::printf("took %f seconds\n", std::chrono::duration<double>(end - begin).count());

	begin = std::chrono::system_clock::now();
	for (int i = 0; i < N; ++i)
		on_stack();
	end = std::chrono::system_clock::now();
	std::printf("on_stack took %f seconds\n", std::chrono::duration<double>(end - begin).count());

	begin = std::chrono::system_clock::now();
	//for (int i = 0; i < N; ++i)
		//on_heap();
	end = std::chrono::system_clock::now();
	std::printf("on_heap took %f seconds\n", std::chrono::duration<double>(end - begin).count());

	begin = std::chrono::system_clock::now();
	{
		std::deque<int> dq(N, 0);
		for( int i = 0; i < N; ++i )
			dq[i] = i;
	}
	end = std::chrono::system_clock::now();
	std::printf("deque took %f seconds\n", std::chrono::duration<double>(end - begin).count());

	begin = std::chrono::system_clock::now();
	{
		//int* i = new int[N];
		//for( int i = 0; i < N; ++i )
			//v[i] = i;
		//delete [] i;
	}
	end = std::chrono::system_clock::now();
	std::printf("heap once took %f seconds\n", std::chrono::duration<double>(end - begin).count());

	begin = std::chrono::system_clock::now();
	{
		std::array<int, N> v;
		for( int i = 0; i < N; ++i )
			v[i] = i;
		//std::iota(v.begin(), v.end(), 0);
	}
	end = std::chrono::system_clock::now();
	std::printf("std array took %f seconds\n", std::chrono::duration<double>(end - begin).count());

	begin = std::chrono::system_clock::now();
	{
		//std::vector<int> v(N);
		//std::iota(v.begin(), v.end(), 0);
	}
	end = std::chrono::system_clock::now();
	std::printf("vector took %f seconds\n", std::chrono::duration<double>(end - begin).count());

	begin = std::chrono::system_clock::now();
	{
		int c[N];
		for( int i = 0; i < N; ++i )
			c[i] = i;
	}
	end = std::chrono::system_clock::now();
	std::printf("array took %f seconds\n", std::chrono::duration<double>(end - begin).count());

	return 0;
}
