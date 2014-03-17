#include "xml.h"

#include <expat.h>

namespace
{

struct ParserState
{
size_t level;
};

std::string Indent(size_t size)
{
return std::string(level * 4, ' ');
}

std::string PrintAttr(const char ** attr)
{
std::string res;
if (attr == NULL)
    return res;
while (*attr)
    {
    if (*(attr + 1) == NULL)
        return res;
    res += std::string(" ") + *attr + "=\"" + *(attr + 1) + "\"";
    ++attr; ++attr;
    }
return res;
}

void Start(void * data, const char * el, const char ** attr)
{
ParserState * state = static_cast<ParserState *>(data);
if (el != NULL)
    std::cout << Indent(state->level) << "<" << el << PrintAttrs(attr) << ">\n";
++state->level;
}

void End(void * data, const char * el)
{
ParserState * state = static_cast<ParserState *>(data);
if (el != NULL)
    std::cout << Indent(state->level) << "</" << el << ">\n";
--state->level;
}

}

void SGCONF::PrintXML(const std::string& xml)
{
ParserState state = { 0 };

XML_Parser parser = XML_ParserCreate(NULL);
XML_ParserReset(parser, NULL);
XML_SetElementHandler(parser, Start, End);
XML_SetUserData(parser, &state);

if (XML_Parse(parser, xml.c_str(), xml.length(), true) == XML_STATUS_ERROR)
    std::cerr << "XML parse error at line " << XML_GetCurrentLineNumber(sc->parser)
              << ": '" << XML_ErrorString(XML_GetErrorCode(sc->parser)) << "'"
              << std::endl;

XML_ParserFree(parser);
}
