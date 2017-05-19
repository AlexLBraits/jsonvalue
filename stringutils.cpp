#include "stringutils.h"
#include <float.h> // DBL_MAX
#include <math.h> // modf

char* get_buf(size_t sz)
{
    static size_t buf_size = 1024;
    static char* buf = (char*) malloc(buf_size);
    
    if(sz > buf_size)
    {
        buf_size = sz * 1.5;
        buf = (char*) realloc(buf, buf_size);
    }
    return buf;
}

std::string numberToString (double d)
{
    char buf[64];
    int buffer_size = 64;
    int printedChars = 0;
    
    double absd = d < 0 ? -d: d;
    double delta = (int64_t)(d) - d;
    double absDelta = delta < 0 ? -delta : delta;
    if ((d * 0) != 0) {
        printedChars = snprintf(buf, buffer_size, "null");
    } else if ((absDelta <= DBL_EPSILON) && (absd < 1.0e60)) {
        printedChars = snprintf(buf, buffer_size, "%.1f", d);
    } else if ((absd < 1.0e-6) || (absd > 1.0e9)) {
        printedChars = snprintf(buf, buffer_size, "%e", d);
    } else {
        printedChars = snprintf(buf, buffer_size, "%f", d);
        if (printedChars < buffer_size) {
            while (buf[printedChars - 1] == '0') {
                printedChars--;
            }
        }
    }
    return std::string (buf, buf + printedChars);
}

std::string numberToString (long long v)
{
    static char buf[64];
    int printedChars = snprintf (buf, 63, "%lld", v);
    return std::string (buf, buf + printedChars);
}

void escape (char *dst, const char* src)
{
    char c;
    while ((c = *src++))
    {
        switch (c)
        {
        case '\"' :
            *dst++ = '\\';
            *dst++ = '\"';
            break;
        case '\\' :
            *dst++ = '\\';
            *dst++ = '\\';
            break;
        case '/'  :
            *dst++ = '\\';
            *dst++ = '/';
            break;
        case '\b' :
            *dst++ = '\\';
            *dst++ = 'b';
            break;
        case '\f' :
            *dst++ = '\\';
            *dst++ = 'f';
            break;
        case '\n' :
            *dst++ = '\\';
            *dst++ = 'n';
            break;
        case '\r' :
            *dst++ = '\\';
            *dst++ = 'r';
            break;
        case '\t' :
            *dst++ = '\\';
            *dst++ = 't';
            break;
        default:
            *dst++ = c;
            break;
        }
    }
    *dst = 0;
}

void
ssplit (std::vector<std::string>& theStringVector,  /* Altered/returned value */
        const  std::string  & theString,
        const  std::string  & theDelimiter,
        bool  theIncludeEmptyStrings)
{
    size_t start = 0, end = 0, length = 0;

    while ( end != std::string::npos )
    {
        end = theString.find ( theDelimiter, start );

        // If at end, use length=maxLength.  Else use length=end-start.
        length = (end == std::string::npos) ? std::string::npos : end - start;

        if (    theIncludeEmptyStrings
                || (   ( length > 0 ) /* At end, end == length == string::npos */
                       && ( start  < theString.size() ) ) )
            theStringVector.push_back( theString.substr( start, length ) );

        // If at end, use start=maxSize.  Else use start=end+delimiter.
        start = (   ( end > (std::string::npos - theDelimiter.size()) )
                    ?  std::string::npos  :  end + theDelimiter.size()     );
    }
}

std::vector<std::string>
ssplit ( const std::string  & theString,
         const std::string  & theDelimiter,
         bool             theIncludeEmptyStrings)
{
    std::vector<std::string> v;
    ssplit ( v, theString, theDelimiter, theIncludeEmptyStrings );
    return v;
}

std::string escapedString (const std::string& s)
{
    std::string es;

    const char* src = s.c_str ();
    size_t sz = strlen (src) * 2;

    char* dst = get_buf(sz + 1);
    if (dst)
    {
        escape (dst, src);
        es = dst;
    }
    return es;
}

void replace(std::string& str, const std::string& oldStr, const std::string& newStr)
{
    std::string::size_type pos = 0u;
    while((pos = str.find(oldStr, pos)) != std::string::npos)
    {
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }
}
