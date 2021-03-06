#include <string>

#include <seqan3/core/debug_stream.hpp>
#include <seqan3/range/views/take_line.hpp>

int main()
{
    std::string vec{"foo\nbar"};
    auto v = vec | seqan3::views::take_line;
    seqan3::debug_stream << v << '\n'; // [f,o,o]

    auto v2 = vec | std::views::reverse | seqan3::views::take_line;
    seqan3::debug_stream << v2 << '\n'; // [r,a,b]
    seqan3::debug_stream << v2 << '\n'; // [r,a,b] (parsing it again gives us the same result)
}
