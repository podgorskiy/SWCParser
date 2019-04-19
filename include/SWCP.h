#pragma once
#include <stdlib.h>
#include <cstdarg>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

#ifdef VS2013
	#include <stdint.h>
#endif

namespace SWCP 
{
	#if __cplusplus >= 201103L
		#define SWCP_CPP11_COMPATIBLE
	#endif
	#if defined _MSC_VER && _MSC_VER >= 1800
		#define SWCP_CPP11_COMPATIBLE
	#endif

	// If not c++11 and if it is not version of MSVC that is c++11 compatible, 
	// then define own int64_t and uint64_t types in SWCP namespace.
	#if !defined SWCP_CPP11_COMPATIBLE && !defined SWIG
		typedef long long int int64_t;
		typedef unsigned long long int uint64_t;
	#endif

	// strtoll and strtoull are not part of standard prior c++11, 
	// but are widely supported, except for MSVC that has own names for them
	#if defined _MSC_VER && (_MSC_VER < 1800)
		inline int64_t strtoll(char const* str, char** endPtr, int radix)
		{
			return _strtoi64(str, endPtr, radix);
		}

		inline uint64_t strtoull(char const* str, char** endPtr, int radix)
		{
			return _strtoui64(str, endPtr, radix);
		}
	#endif

	// Safe version of sprintf
	#if !defined SWCP_CPP11_COMPATIBLE && defined _MSC_VER
		template<class T, size_t N>
		inline int64_t sprintf(T(&dstBuf)[N], const char * format, ...)
		{
			va_list args;
			va_start(args, format);
			int result = vsprintf_s(dstBuf, format, args);
			va_end(args);
			return result;
		}
	#elif defined SWCP_CPP11_COMPATIBLE
		template<class T, size_t N>
		inline int sprintf(T(&dstBuf)[N], const char * format, ...)
		{
			va_list args;
			va_start(args, format);
			int result = vsnprintf(dstBuf, N, format, args);
			va_end(args);
			return result;
		}
	#endif

	struct Vertex
	{
		enum Type
		{
			Undefined = 0,
			Soma,
			Axon,
			Dendrite,
			ApicalDendrite,
			ForkPoint,
			EndPoint,
			Custom,
		};

		Vertex(int64_t id, Type type, double x, double y, double z, float radius) : id(id), type(type), radius(radius), x(x), y(y), z(z), visited(false)
		{};
		
		Vertex() {};

		int64_t id;
		double x;
		double y;
		double z;
		float radius;
		Type type;
		bool visited;
	};
		
	struct Edge
	{
		Edge(int64_t idParent, int64_t idChild) : idParent(idParent), idChild(idChild)
		{};
		
		Edge() {};

		int64_t idParent;
		int64_t idChild;
	};
	
	struct Graph
	{
		std::vector<int64_t> rootIDs;
		std::vector<Vertex> vertices;
		std::vector<Edge> edges;
		std::vector<std::string> meta;
	};

	class Parser
	{
	public:
		// Reads SWC from filespecified by *filename*. 
		// Output is written to *graph*, old content is errased. 
		// If no error have happened returns true
		bool ReadSWCFromFile(const char *filename, Graph& graph);

		// Reads SWC from filestream. 
		// Output is written to graph, old content is errased. 
		// If no error have happened returns true
		bool ReadSWC(std::istream &inStream, Graph& graph);

		// Reads SWC from string. 
		// Output is written to graph, old content is errased. 
		// If no error have happened returns true
		bool ReadSWC(const char *string, Graph& graph);
		
		// Returns error message for the last parsing if error have happened.
		std::string GetErrorMessage();
	private:
		void NextSymbol();

		bool Accept(char symbol);
		bool AcceptWhightSpace();
		bool AcceptLine(Graph& graph);
		bool AcceptEndOfLine();
		bool AcceptInteger(int64_t& integer);
		bool AcceptInteger(uint64_t& integer);
		bool AcceptDouble(double& integer);
		
		const char* m_iterator;
		int m_line;
		std::stringstream m_errorMessage;
	};

	class Generator
	{
	public:
		bool WriteToFile(const char *filename, const Graph& graph);

		bool Write(std::ostream &outStream, const Graph& graph);

		bool Write(std::string& outString, const Graph& graph);

		std::string GetErrorMessage();
	private:
		enum 
		{
			MaxLineSize = 4096
		};
		std::stringstream m_errorMessage;
	};

	inline bool SWCP::Parser::ReadSWCFromFile(const char *filename, Graph& graph)
	{
		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		if (!file.is_open())
		{
			m_errorMessage.clear();
			m_errorMessage << "Error: Can not open file: " << filename << '\n';
			return false;
		}

		char* content = new char[static_cast<size_t>(size) + 1];
		file.read(content, size);

		content[size] = '\0';
		
		bool result = ReadSWC(content, graph);

		delete[] content;

		return result;
	}

	inline bool Parser::ReadSWC(std::istream &inStream, Graph& graph)
	{
		std::string str;
		char buffer[4096];
		while (inStream.read(buffer, sizeof(buffer)))
		{
			str.append(buffer, sizeof(buffer));
		}
		str.append(buffer, static_cast<unsigned int>(inStream.gcount()));
		return ReadSWC(str.c_str(), graph);
	}

	inline bool Parser::ReadSWC(const char *string, Graph& graph)
	{
		m_errorMessage.clear();

		m_iterator = string;

		int countOfLines = 1;
		while (*m_iterator != '\0')
		{
			while (!AcceptEndOfLine() && *m_iterator != '\0')
			{
				NextSymbol();
			}
			countOfLines++;
		}

		graph.edges.clear();
		graph.vertices.clear();
		graph.meta.clear();
		graph.edges.reserve(countOfLines);
		graph.vertices.reserve(countOfLines);

		m_line = 1;
		m_iterator = string;

		while (AcceptLine(graph))
		{
			++m_line;
		}

		if (*m_iterator == '\0')
		{
			return true;
		}
		else
		{
			m_errorMessage << "Error at line: " << m_line << ", unexpected symbol:" << *m_iterator << '\n';
			return false;
		}
	}

	inline void Parser::NextSymbol()
	{
		if (*m_iterator != '\0')
		{
			++m_iterator;
		}
	}

	inline bool Parser::AcceptLine(Graph& graph)
	{
		while (AcceptWhightSpace())
		{}

		if (AcceptEndOfLine())
		{
			return true;
		}

		if (Accept('#'))
		{
			const char* commentStart = m_iterator;
			const char* commentEnd = m_iterator;
			while (!AcceptEndOfLine() && *m_iterator != '\0')
			{
				NextSymbol();
				commentEnd = m_iterator;
			}
			graph.meta.push_back(std::string(commentStart, commentEnd));
			return true;
		}

		int64_t id = 0;
		if (AcceptInteger(id))
		{
			int64_t type = 0;
			if (AcceptInteger(type))
			{
				double x, y, z;
				if (AcceptDouble(x))
				{
					if (AcceptDouble(y))
					{
						if (AcceptDouble(z))
						{
							double r;
							if (AcceptDouble(r))
							{
								int64_t parent;
								if (AcceptInteger(parent))
								{
									graph.vertices.push_back(Vertex(id, static_cast<Vertex::Type>(type), x, y, z, static_cast<float>(r)));

									if (parent != -1)
									{
										graph.edges.push_back(Edge(parent, id));
									}
									else
									{
										graph.rootIDs.push_back(id);
									}
									return true;
								}
								else
								{
									m_errorMessage << "Error at line: " << m_line 
										<< ", wrong parent. You need to specify an id of a perent, or -1 if there is no parent.\n";
									return false;
								}
							}
							m_errorMessage << "Error at line: " << m_line
								<< ", wrong radius. You need to specify a radius as a float value.\n";
							return false;
						}
					}
				}
				m_errorMessage << "Error at line: " << m_line
					<< ", wrong coordinates. You need to specify type as an integer value.\n";
				return false;
			}
			m_errorMessage << "Error at line: " << m_line
				<< ", wrong coordinates. You need to specify coordinates as three double values.\n";
			return false;
		}

		return false;
	}

	inline bool Parser::AcceptEndOfLine()
	{
		if (Accept('\n'))
		{
			return true;
		}
		else if (Accept('\r'))
		{
			Accept('\n');
			return true;
		}

		return false;
	}

	inline bool Parser::Accept(char symbol)
	{
		if (symbol == *m_iterator)
		{
			NextSymbol();
			return true;
		}
		return false;
	}

	inline bool Parser::AcceptWhightSpace()
	{
		if (Accept(' '))
		{
			return true;
		}
		if (Accept('\t'))
		{
			return true;
		}
		return false;
	}

	inline bool Parser::AcceptInteger(int64_t& value)
	{
		char* endp = NULL;
		int64_t result = strtoll(m_iterator, &endp, 0);
		if (endp > m_iterator)
		{
			value = result;
			m_iterator = endp;

			while (AcceptWhightSpace())
			{
			}

			return true;
		}
		return false;
	}

	inline bool Parser::AcceptInteger(uint64_t& value)
	{
		char* endp = NULL;
		uint64_t result = strtoull(m_iterator, &endp, 0);
		if (endp > m_iterator)
		{
			value = result;
			m_iterator = endp;

			while (AcceptWhightSpace())
			{
			}

			return true;
		}
		return false;
	}

	inline bool Parser::AcceptDouble(double& value)
	{
		char* endp = NULL;
		double result = strtod(m_iterator, &endp);
		if (endp > m_iterator)
		{
			value = result;
			m_iterator = endp;

			while (AcceptWhightSpace())
			{
			}

			return true;
		}
		return false;
	}

	inline std::string Parser::GetErrorMessage()
	{
		return m_errorMessage.str();
	}

	inline bool Generator::Write(std::ostream &outStream, const Graph& graph)
	{
		for (std::vector<std::string>::const_iterator it = graph.meta.begin(); it != graph.meta.end(); ++it)
		{
			outStream << "#" << (*it) << '\n';
		}

		std::map<int64_t, int64_t> edgeMapChildToParent;
		std::map<int64_t, int64_t> inconsistentEdgeMap;

		for (std::vector<Edge>::const_iterator it = graph.edges.begin(); it != graph.edges.end(); ++it)
		{
			std::map<int64_t, int64_t>::const_iterator edgeIt = edgeMapChildToParent.find(it->idChild);
			if (edgeIt == edgeMapChildToParent.end())
			{
				edgeMapChildToParent[it->idChild] = it->idParent;
			}
			else
			{
				// We have some inconsistency here.
				inconsistentEdgeMap[it->idParent] = it->idChild;
			}
		}

		// This may fix some amount of inconistency.
		for (std::map<int64_t, int64_t>::const_iterator it = inconsistentEdgeMap.begin(); it != inconsistentEdgeMap.end(); ++it)
		{
			int64_t parent = it->first;
			int64_t child = it->second;
			int64_t start = child;
			while (true)
			{
				std::map<int64_t, int64_t>::iterator edgeIt = edgeMapChildToParent.find(parent);
				if (edgeIt == edgeMapChildToParent.end())
				{
					edgeMapChildToParent[parent] = child;
					break;
				}
				if (parent == start)
				{
					m_errorMessage << "Loop detected!" << '\n';
					break;
				}
				int64_t tmp = parent;
				parent = edgeIt->second;
				edgeIt->second = child;
				child = tmp;
			}
		}

		for (std::vector<Vertex>::const_iterator it = graph.vertices.begin(); it != graph.vertices.end(); ++it)
		{
			std::map<int64_t, int64_t>::const_iterator parentVertex = edgeMapChildToParent.find(it->id);
			int64_t parent = -1;
			if (parentVertex != edgeMapChildToParent.end())
			{
				parent = parentVertex->second;
			}
			char buff[MaxLineSize];
			sprintf(buff, " %lld %d %.15g %.15g %.15g %.7g %lld\n", it->id, it->type, it->x, it->y, it->z, it->radius, parent);

			outStream << buff;
		}

		return true;
	}

	inline bool Generator::Write(std::string& outString, const Graph& graph)
	{
		std::stringstream sstream;
		bool result = Write(sstream, graph);
		outString = sstream.str();
		return result;
	}

	inline std::string Generator::GetErrorMessage()
	{
		return m_errorMessage.str();
	}

	inline bool Generator::WriteToFile(const char *filename, const Graph& graph)
	{
		std::ofstream file(filename, std::ios::binary);

		if (!file.is_open())
		{
			m_errorMessage.clear();
			m_errorMessage << "Error: Can not open file: " << filename << '\n';
			return false;
		}

		std::stringstream sstream;
		bool result = Write(sstream, graph);
		file << sstream.rdbuf();
		return result;
	}
}
