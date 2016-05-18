#include "module.hpp"
#include <fstream>

using namespace std;

namespace stream {

void print_error(const string & path, const code_range & range, const string & msg)
{
    cerr << "** ERROR ["
         << path << ":"
         << range.start.line << ":"
         << range.start.column
         << "]: "
         << msg
         << endl;

    if (range.end.line == range.start.line)
    {
        ifstream file(path);
        if (!file.is_open())
            return;

        string line_text;

        for(int line = 0; line < range.start.line; ++line)
        {
            std::getline(file, line_text);
        }

        cout << "    " << line_text << endl;
        cout << "    " << string(range.start.column-1,' ');
        cout << string(range.end.column - range.start.column, '^');
        cout << endl;
    }
}

}
