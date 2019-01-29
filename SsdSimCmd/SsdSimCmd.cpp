// SsdSimCmd.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include <future>
#include <array>
#include <chrono>

#include "Nand/NandDevice.h"
#include "Nand/NandHal.h"
#include "Framework.h"
#include "Ipc/MessageClient.h"

#include <boost/program_options.hpp>
#include <string>
#include <fstream>
#include <iostream>
#include "JSONParser.h"

using namespace boost::program_options;

int main(int argc, const char*argv[])
{
	try
	{
		options_description generalOptions{ "General" };
		generalOptions.add_options()
			("help,h", "Help screen")
			("nandspec", value<std::string>(), "Nand specifications");

		options_description fileOptions{ "File" };
		fileOptions.add_options()
			("age", value<int>(), "Age");

		variables_map vm;
		store(parse_command_line(argc, argv, generalOptions), vm);
		if (vm.count("nandspec"))
		{
			try 
			{
				Framework framework(vm["nandspec"].as<std::string>());
				//Start framework
				framework.operator()();
			}
			catch (const ParserError &parser)
			{
				std::cerr << "Parser has failed!" << '\n';
				return false;
			}
		}
		notify(vm);
		if (vm.count("help"))
			std::cout << generalOptions << '\n';
	}
	catch (const error &ex)
	{
		std::cerr << ex.what() << '\n';
	}

	return true;
}
