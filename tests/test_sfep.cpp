#define BOOST_TEST_MODULE __FILE__
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <tuple>

#define RIOT_SERVER_SFEP_NO_MATCHER_PADDING
#include <riot/server/parsers/sfe_parser.hpp>

namespace utf   = boost::unit_test;
namespace utfd  = utf::data;
using namespace std;
using namespace riot::server;

/*******************************************************************************
 *      test_negatives
 ******************************************************************************/

auto load_negatives() {
    vector<string> negatives;
    ifstream ifs{"negatives.txt"};
    string line;
    while (getline(ifs, line)) {
        negatives.push_back(line);
    }
    return negatives;
}

BOOST_DATA_TEST_CASE(test_negatives, utfd::make(load_negatives()), x) {
    BOOST_CHECK_THROW(sfe_parser::parse(x), std::runtime_error);
}


/*******************************************************************************
 *      test_comparisons
 ******************************************************************************/

auto load_comparisons() {
    std::vector<std::tuple<std::string, std::string>> comparisons;
    ifstream ifs{"comparisons.txt"};
    string line;
    int i = 0;
    std::string str0;
    while (getline(ifs, line)) {
        if (i == 0) {
            str0 = line;
            ++i;
        }
        else if (i == 1) {
            comparisons.emplace_back(str0, line);
            ++i;
        }
        else {
            i = 0;
        }
    }
    return comparisons;
}

BOOST_DATA_TEST_CASE(test_comparisons, utfd::make(load_comparisons()), x, y) {
    std::ostringstream oss;
    BOOST_CHECK_NO_THROW(oss << sfe_parser::parse(x));
    BOOST_CHECK_EQUAL(y, oss.str());
}


