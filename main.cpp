#include <fstream>

#include "busmanager.h"
#include "profile.h"


using namespace std;

int main(){
	BusManager bm;

	string inputFilePath = "/home/sergey/Books/coursera-c++brown-4/Экзамен - граф/transport-input1.json";
	ifstream input(inputFilePath, ios::binary);
	if(!input){
		std::cout << "Не найден файл шаблона " << inputFilePath <<'\n';
		return 1;
	}

	stringstream in_ss;
	std::copy(
	        std::istreambuf_iterator<char>(input),
	        std::istreambuf_iterator<char>( ),
	        std::ostreambuf_iterator<char>(in_ss));

	{
		LOG_DURATION("bm.Read")
		bm.Read(in_ss);
	}

	stringstream out_ss;
	{
		LOG_DURATION("bm.WriteResponse")
		bm.WriteResponse(out_ss);
	}

	string outputFilePath = "/home/sergey/Books/coursera-c++brown-4/result1.json";
	ofstream output(outputFilePath, ios::binary);

	std::copy(
		        std::istreambuf_iterator<char>(out_ss),
		        std::istreambuf_iterator<char>( ),
		        std::ostreambuf_iterator<char>(output));

	return 0;
}

