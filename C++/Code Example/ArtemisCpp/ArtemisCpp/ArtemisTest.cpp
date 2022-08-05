// ArtemisTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "external/artemis/CArtemis.h"
#include <string>
int main(int argc, char* argv[])
{
	CArtemis::Initialize();
	CArtemis::Wait(CArtemis::EEventType::Click, "run-btn");
	auto state = CArtemis::QueryWait();
	std::string input1 = state.state["num1"];
	std::string input2 = state.state["num2"];
	float num1 = std::stof(input1);
	float num2 = std::stof(input2);

	float output = std::pow(std::cos(num1), 2) + std::pow(std::sin(num2), 2);
	CArtemis::Update("output", std::to_string(output));
	_getch();
	return 0;
}
