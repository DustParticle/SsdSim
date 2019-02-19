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

using namespace boost::program_options;

bool parser_command_line(int argc, const char*argv[], std::string& nandspecFilename)
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
			nandspecFilename = vm["nandspec"].as<std::string>();
		}
		else
		{
			std::cout << "nandspec is required!";
			return false;
		}
		notify(vm);
		if (vm.count("help"))
			std::cout << generalOptions << '\n';
	}
	catch (const error &ex)
	{
		std::cerr << ex.what() << '\n';
		return false;
	}
	return true;
}


int main(int argc, const char*argv[])
{
	std::string nandspecFilename = "";

	if (parser_command_line(argc, argv, nandspecFilename))
	{
		if (!nandspecFilename.empty())
		{

			try
			{
				Framework framework;
				framework.Init(nandspecFilename);
				//Start framework
				framework.operator()();
			}
			catch (const Framework::Exception &err)
			{
				std::cout << err.what();
				return -1;
			}
		}
	}

	return 0;
}


