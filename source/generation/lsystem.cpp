#include "lsystem.h"

std::string LSystemString::RunProduction(int iterations)
{
	std::string production = axiom;

	while (--iterations >= 0)
	{
		std::string newString;
		for (char c : production)
		{
			if (productionRules.count(c))
			{
				newString.append(productionRules[c]);
			}
			else
			{
				newString.append(1, c);
			}
		}
		production = newString;
	}

	return production;
}

std::string LSystemStringFunctional::RunProduction(int iterations)
{
	std::string production = axiom;

	while (--iterations >= 0)
	{
		std::string newString;
		for (char c : production)
		{
			if (productionRules.count(c))
			{
				newString.append(productionRules[c]());
			}
			else
			{
				newString.append(1, c);
			}
		}
		production = newString;
	}

	return production;
}
