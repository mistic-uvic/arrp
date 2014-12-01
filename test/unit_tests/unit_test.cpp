#include "unit_test.hpp"

#include "../../frontend/parser.h"
#include "../../frontend/environment.hpp"
#include "../../frontend/type_checker.hpp"
#include "../../frontend/ir-generator.hpp"
#include "../../polyhedral/translator.hpp"

#include <sstream>

using namespace std;

namespace stream {
namespace unit_test {

ast::node_ptr syntactic_analysis(istream & code)
{
    Parser parser(code);
    if (parser.parse() == 0)
        return parser.ast();
    else
        return ast::node_ptr();
}

bool symbolic_analysis
(istream & code, semantic::environment * env)
{
    ast::node_ptr ast_root = syntactic_analysis(code);
    if (!ast_root)
        return false;

    if(env)
    {
        semantic::environment_builder env_builder(*env);
        return env_builder.process(ast_root);
    }
    else
    {
        semantic::environment local_env;
        semantic::environment_builder env_builder(local_env);
        return env_builder.process(ast_root);
    }
}

semantic::type_ptr semantic_analysis
(istream & code,
 const string & symbol, vector<semantic::type_ptr> arguments)
{
    semantic::environment env;
    if (!symbolic_analysis(code, &env))
        return semantic::type_ptr();

    auto sym_iter = env.find(symbol);
    if (sym_iter == env.end())
    {
        cerr << "ERROR: no symbol '" << symbol << "' available." << endl;
        return semantic::type_ptr();
    }

    semantic::type_checker type_checker(env);
    semantic::type_ptr result_type =
            type_checker.check(sym_iter->second, arguments);

    return result_type;
}

vector<polyhedral::statement*> polyhedral_model
(istream & code,
 const string & symbol,
 vector<semantic::type_ptr> arguments)
{
    vector<polyhedral::statement*> empty_result;

    semantic::environment env;
    if (!symbolic_analysis(code, &env))
        return empty_result;

    auto sym_iter = env.find(symbol);
    if (sym_iter == env.end())
    {
        cerr << "ERROR: no symbol '" << symbol << "' available." << endl;
        return empty_result;
    }

    semantic::type_checker type_checker(env);
    semantic::type_ptr result_type =
            type_checker.check(sym_iter->second, arguments);

    if (!result_type->is(semantic::type::function))
    {
        cerr << "ERROR: symbol '" << symbol << "' is not a function." << endl;
        return empty_result;
    }

    polyhedral::translator poly(env);
    poly.translate(result_type->as<semantic::function>(), arguments);
    return poly.statements();
}


}
}
