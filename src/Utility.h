
#include <string>
#include <algorithm>
#include <exception>
#include <fstream>

inline const std::string toLower(const std::string& str)
{
	std::string res{str};
	std::transform(res.begin(), res.end(), res.begin(), ::tolower);

	return res;
} 

inline const void createFileWithData(const std::string& file, const std::string& data)
{
	std::ofstream outfile (file);
	outfile << data;
	outfile.close();
}


