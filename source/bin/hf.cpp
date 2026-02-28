#include <HCore/HCore.h>
#include <string>
using namespace std;

int main(int argc, char* argv[])
{
	if( argc > 1 )
	{
		string filename = argv[1];
		HCore hf(filename);
		hf.run();
	}
}