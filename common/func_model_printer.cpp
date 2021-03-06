#include "func_model_printer.hpp"

#include <cmath>

using namespace std;

namespace stream {
namespace functional {

printer::printer()
{

}

template<typename L>
void print_list(const L & list, const string & separator = ", ")
{
    int size = list.size();
    int count = 0;
    for(const auto & elem : list)
    {
        ++count;
    }
}

void printer::print(id_ptr id, ostream & out)
{
    out << id->name << " = ";
    print(id->expr, out);
}

void printer::print(expr_ptr expr, ostream & out)
{
    if (!expr)
    {
        out << "{}";
    }
    else if (auto const_int = dynamic_pointer_cast<int_const>(expr))
    {
        out << const_int->value;
    }
    else if (auto const_double = dynamic_pointer_cast<real_const>(expr))
    {
        out << const_double->value;
    }
    else if (auto const_bool = dynamic_pointer_cast<bool_const>(expr))
    {
        out << (const_bool->value ? "true" : "false");
    }
    else if (auto inf = dynamic_pointer_cast<infinity>(expr))
    {
        out << "~";
    }
    else if (auto const_complex = dynamic_pointer_cast<complex_const>(expr))
    {
        auto & v = const_complex->value;
        if (v.real())
            out << v.real() << '+';
        out << v.imag() << 'i';
    }
    else if (auto ref = dynamic_pointer_cast<reference>(expr))
    {
        if (m_print_var_address)
            out << ref->var;
        else
            out << name(ref->var);
    }
    else if (auto aref = dynamic_pointer_cast<array_self_ref>(expr))
    {
        out << "this";
    }
    else if (auto prim = dynamic_pointer_cast<const primitive>(expr))
    {
        out << prim->kind;

        out << "(";
        int count = 0;
        for(auto & operand : prim->operands)
        {
            print(operand, out);
            if (++count < prim->operands.size())
                out << ",";
        }
        out << ")";
    }
    else if (auto op = dynamic_pointer_cast<const operation>(expr))
    {
        switch(op->kind)
        {
        case operation::array_concatenate:
            out << "++"; break;
        case operation::array_enumerate:
            out << ".."; break;
        default:
            out << "?";
        }

        out << "(";
        int count = 0;
        for(auto & operand : op->operands)
        {
            print(operand, out);
            if (++count < op->operands.size())
                out << ",";
        }
        out << ")";
    }
    else if (auto ae = dynamic_pointer_cast<const affine_expr>(expr))
    {
        print(ae->expr, out);
    }
    else if (auto c = dynamic_pointer_cast<const case_expr>(expr))
    {
        int ci = 0;
        for (auto & a_case : c->cases)
        {
            if (ci++ > 0)
                out << ' ';
            out << "| ";
            print(a_case.first, out);
            out << " -> ";
            print(a_case.second, out);
        }
    }
    else if (auto ar = dynamic_pointer_cast<const array>(expr))
    {
        out << "[";
        {
            int count = ar->vars.size();
            for (auto var : ar->vars)
            {
                out << name(var);
                if (var->range)
                {
                    out << ":";
                    print(var->range, out);
                }
                if (--count)
                    out << ",";
            }
        }
        out << ": ";
        print(ar->expr, out);
        if (m_print_scopes && ar->scope.ids.size())
        {
            out << " where ";
            int i = 0;
            for(const auto & id : ar->scope.ids)
            {
                if (i++ > 0)
                    out << ", ";
                print(id, out);
            }
        }
        out << "]";
    }
    else if (auto p = dynamic_pointer_cast<array_patterns>(expr))
    {
        auto pi = 0;
        for (auto & pattern : p->patterns)
        {
            if (pi++ > 0)
                out << ' ';
            auto dim = 0;
            for (auto & index : pattern.indexes)
            {
                if (dim++ > 0)
                    out << ',';
                if (index.var)
                {
                    out << index.var->name;
                    if (index.var->range)
                    {
                        out << ":";
                        print(index.var->range, out);
                    }
                }
                else
                    out << index.value;
            }
            if (pattern.domains)
            {
                out << " ";
                print(pattern.domains, out);
                out << " | ";
                print(pattern.expr, out);
            }
            else
            {
                out << " -> ";
                print(pattern.expr, out);
            }
            out << ';';
        }
    }
    else if (auto app = dynamic_pointer_cast<array_app>(expr))
    {
        print(app->object, out);
        out << "[";
        int i = 0;
        for (const auto & arg : app->args)
        {
            print(arg, out);
            if(++i < app->args.size())
                out << ",";
        }
        out << "]";
    }
    else if (auto as = dynamic_pointer_cast<array_size>(expr))
    {
        out << '#';
        print(as->object, out);
        if (as->dimension)
        {
            out << '@';
            print(as->dimension, out);
        }
    }
    else if (auto app = dynamic_pointer_cast<func_app>(expr))
    {
        print(app->object, out);
        out << "(";
        int i = 0;
        for (const auto & arg : app->args)
        {
            print(arg, out);
            if(++i < app->args.size())
                out << ", ";
        }
        out << ")";
    }
    else if (auto func = dynamic_pointer_cast<function>(expr))
    {
        out << "\\";
        int i = 0;
        for (const auto & var : func->vars)
        {
            out << name(var);
            if(++i < func->vars.size())
                out << ", ";
        };
        out << " -> ";
        print(func->expr, out);
        if (m_print_scopes && func->scope.ids.size())
        {
            out << " where ";
            int i = 0;
            for(const auto & id : func->scope.ids)
            {
                if (i++ > 0)
                    out << ", ";
                print(id, out);
            }
        }
        out << "\\";
    }
    else
    {
        out << "?";
    }
}

void printer::print(const linexpr & expr, ostream & out)
{
    out << "{";
    int t = 0;
    for(const auto & term : expr)
    {
        const auto var = term.first;
        int coef = term.second;
        int abs_coef = std::abs(coef);
        char sign = (coef > 0 ? '+' : '-');
        if (t > 0 || coef < 0)
            out << sign;
        if (!var || abs_coef != 1)
            out << abs_coef;
        if (var)
            out << var->name;
        ++t;
    }
    out << "}";
}

}
}
