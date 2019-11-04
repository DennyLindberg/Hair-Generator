#pragma once
#include <string>
#include <map>
#include <functional>

class LSystemString
{
public:
	std::string axiom = "";
	std::map<char, std::string> productionRules; // any symbol assigned here becomes a variable

	LSystemString() = default;
	~LSystemString() = default;

	std::string RunProduction(int iterations = 1);
};

class LSystemStringFunctional
{
public:
	std::string axiom = "";
	std::map<char, std::function<std::string()>> productionRules; // any symbol assigned here becomes a variable

	LSystemStringFunctional() = default;
	~LSystemStringFunctional() = default;

	std::string RunProduction(int iterations = 1);
};
