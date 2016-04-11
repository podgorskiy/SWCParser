#include <SWCP.h>

#include <cstring>

int main()
{
	bool pass = true;

	int vcount[] = {5, 32434, 3130, 2285, 10024};

	for (int i = 1; i < 6; ++i)
	{
		SWCP::Parser parser;
		SWCP::Graph graph;
		std::stringstream path;
		path << "../tests/test" << i << ".swc";
		bool result = parser.ReadSWCFromFile(path.str().c_str(), graph);
		pass &= result;
		pass &= vcount[i - 1] == static_cast<int>(graph.vertices.size());
		if (!result)
		{
			printf("Failed on %d\n", i);
		}
		else
		{
			printf("Passed: %s\n", path.str().c_str());
		}
	}

	for (int i = 1; i < 7; ++i)
	{
		SWCP::Parser parser;
		SWCP::Graph graph;
		std::stringstream path;
		path << "../tests/illegal" << i << ".swc";
		bool result = parser.ReadSWCFromFile(path.str().c_str(), graph);
		pass &= !result;
		if (result)
		{
			printf("Failed on %d\n", i);
		}
		else
		{
			printf("Passed: %s\n", path.str().c_str());
		}
	}

	SWCP::Parser parser;
	SWCP::Generator generator;
	SWCP::Graph graph;
	bool result = parser.ReadSWCFromFile("../tests/test2.swc", graph);

	std::ifstream file("../tests/test2.swc", std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	char* content = new char[static_cast<size_t>(size) + 1];
	file.read(content, size);

	content[size] = '\0';

	result = parser.ReadSWC(content, graph);
	pass &= result;

	std::string generated;
	result = generator.Write(generated, graph);
	pass &= result;

	result = strcmp(generated.c_str(), content) == 0;
	pass &= result;
	
	if (!result)
	{
		printf("Failed on comparing read and generated file\n");
	}
	else
	{
		printf("Passed on comparing read and generated file\n");
	}
		
	if (!pass)
	{
		return -1;
	}
	printf("Passed all tests\n");
	return 0;
}