#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>
#include <vector>
#include <sstream>

char* get_buf(size_t sz);

/////
/////
/////
std::string numberToString (double v);
std::string numberToString (long long v);

///
///
///
template<class InputIt>
std::string merge(InputIt first, InputIt last,
                  const std::string& theDelimiter = ",")
{
    std::ostringstream s;
    for(int n = 0; first != last; ++first, ++n)
    {
        if(n) s << theDelimiter;
        s << *first;
    }
    return s.str();
}
///
/// \brief escapedString
/// \param s
/// \return
///
std::string escapedString (const std::string& s);
///
/// \brief ssplit
/// \param theStringVector
/// \param theString
/// \param theDelimiter
/// \param theIncludeEmptyStrings
///
void ssplit (std::vector<std::string>& theStringVector,
             const std::string& theString, const std::string& theDelimiter,
             bool theIncludeEmptyStrings = false);
///
/// \brief ssplit
/// \param theString
/// \param theDelimiter
/// \param theIncludeEmptyStrings
/// \return
///
std::vector<std::string>
ssplit ( const std::string  & theString,
         const std::string  & theDelimiter,
         bool             theIncludeEmptyStrings = false );
///
/// \brief replace
/// \param str
/// \param oldStr
/// \param newStr
///
void replace(std::string& str, const std::string& oldStr,
             const std::string& newStr);


#endif // STRINGUTILS_H
