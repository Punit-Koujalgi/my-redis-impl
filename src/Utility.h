
#include <string>
#include <algorithm>

inline const std::string toLower(const std::string& str)
{
	std::string res{str};
	std::transform(res.begin(), res.end(), res.begin(), ::tolower);

	return res;
} 

