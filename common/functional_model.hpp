/*
Compiler for language for stream processing

Copyright (C) 2015  Jakob Leben <jakob.leben@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef STREAM_FUNCTIONAL_MODEL_INCLUDED
#define STREAM_FUNCTIONAL_MODEL_INCLUDED

#include "primitives.hpp"
#include "func_types.hpp"
#include "linear_algebra.hpp"
#include "module.hpp"
#include "../frontend/location.hh"
#include <memory>
#include <vector>
#include <utility>
#include <iostream>
#include <unordered_set>
#include <complex>

namespace stream {
namespace functional {

struct model; // to control verbose output

using std::vector;
using std::pair;
using std::unordered_set;
typedef code_location location_type;

class var
{
public:
    var() {}
    var(const string & name, const location_type & loc):
        name(name), location(loc)
    {}
    virtual ~var() {}
    string name;
    location_type location;
    int ref_count = 0;
};

typedef std::shared_ptr<var> var_ptr;

inline std::ostream & operator<<(std::ostream & s, const var* v)
{
    if (v)
        s << "'" << v->name << "' ";
    s << '(' << (void*)v << ')';
    return s;
}

class expression
{
public:
    expression() {}
    expression(const location_type & loc, const type_ptr & type = nullptr):
        location(loc), type(type) {}
    virtual ~expression() {}
    location_type location;
    type_ptr type;

};
typedef std::shared_ptr<expression> expr_ptr;

class expr_slot
{
public:
    expr_slot() {}
    explicit expr_slot(const expr_ptr & e):
        expr(e)
    {
        if (e) location = e->location;
    }
    explicit expr_slot(const expr_ptr &e, const location_type & l):
        expr(e), location(l) {}
    expression & operator*() { return *expr; }
    expression * operator->() { return expr.get(); }
    operator bool() const { return bool(expr); }
    operator expr_ptr() const { return expr; }
    expr_slot & operator=(const expr_ptr & e) { expr = e; return *this; }

    expr_ptr expr;
    location_type location;
};

class identifier : public var
{
public:
    identifier(const string & name, const location_type & loc):
        var(name,loc) {}
    identifier(const string & name, expr_slot e, const location_type & loc):
        var(name,loc), expr(e) {}
    identifier(const string & name, expr_ptr e, const location_type & loc):
        var(name,loc), expr(e) {}
    expr_slot expr;
    bool is_recursive = false;
};
typedef std::shared_ptr<identifier> id_ptr;

class scope
{
public:
    // must preserve order of dependency
    vector<id_ptr> ids;
};

template <typename T>
class constant : public expression
{
public:
    T value;
protected:
    constant(const T & v): value(v) {}
    constant(const T & v, const location_type & loc, const type_ptr & type = nullptr):
        expression(loc, type), value(v) {}
};

class bool_const : public constant<bool>
{
public:
    bool_const(bool v): constant(v) {}
    bool_const(bool v, const location_type & loc, const type_ptr & type = nullptr):
        constant(v, loc, type) {}
};

class int_const : public constant<int>
{
public:
    int_const(int v): constant(v)
    {
        auto t = make_int_type();
        t->affine_flag = true;
        t->constant_flag = true;
        t->data_flag = true;
        type = t;
    }
    int_const(int v, const location_type & loc, const type_ptr & type = nullptr):
        constant(v, loc, type)
    {
        if (!this->type)
        {
            auto t = make_int_type();
            t->affine_flag = true;
            t->constant_flag = true;
            t->data_flag = true;
            this->type = t;
        }
    }
};

class real_const : public constant<double>
{
public:
    real_const(int v): constant(v) {}
    real_const(double v, const location_type & loc, const type_ptr & type = nullptr):
        constant(v, loc, type)
    {}
};

class complex_const: public constant<std::complex<double>>
{
public:
    typedef std::complex<double> value_type;
    complex_const(int v): constant(v) {}
    complex_const(value_type v, const location_type & loc, const type_ptr & type = nullptr):
        constant<value_type>(v, loc, type)
    {}
};

class infinity : public expression
{
public:
    infinity(const location_type & loc = location_type()):
        expression(loc, type::infinity())
    {}
};

class primitive : public expression
{
public:
    primitive() {}

    primitive(primitive_op t,
              const vector<expr_slot> & operands):
        kind(t), operands(operands) {}

    primitive(primitive_op t,
              const vector<expr_ptr> & operands):
        kind(t)
    {
        for (auto & o : operands)
        {
            this->operands.emplace_back(o);
        }
    }

    template<typename ...Ts>
    primitive(primitive_op t,
              Ts ... ops):
        kind(t), operands({ expr_slot(ops)... })
    {
#if 0
        for (auto & o : operands)
        {
            this->operands.emplace_back(o, o->location);
        }
#endif
    }
    primitive_op kind;
    vector<expr_slot> operands;
};

class case_expr : public expression
{
public:
    // A case is <domain constraints, expr>.
    // Constraints can be null.
    vector<pair<expr_slot,expr_slot>> cases;
};

class array_var : public var
{
public:
    enum { unconstrained = -1 };
    array_var() {}
    array_var(const string & name, expr_ptr range,
              const location_type & loc):
        var(name, loc), range(range) {}
    array_var(const string & name, expr_slot range,
              const location_type & loc):
        var(name, loc), range(range) {}
    expr_slot range;
};
typedef std::shared_ptr<array_var> array_var_ptr;

class array : public expression
{
public:
    functional::scope scope;
    vector<array_var_ptr> vars;
    expr_slot expr;
    bool is_recursive = false;
};
typedef std::shared_ptr<array> array_ptr;

class array_patterns : public expression
{
public:
    struct index
    {
        array_var_ptr var;
        int value;
    };

    struct pattern
    {
        vector<index> indexes;
        expr_slot domains;
        expr_slot expr;
    };

    vector<pattern> patterns;
};

class operation : public expression
{
public:
    enum {
        array_concatenate,
        array_enumerate,
    } kind;

    vector<expr_slot> operands;
};

class array_app : public expression
{
public:
    expr_slot object;
    vector<expr_slot> args;
};

class array_size : public expression
{
public:
    array_size() {}
    array_size(expr_ptr o, expr_ptr d, const location_type & l):
        expression(l), object(o), dimension(d) {}
    array_size(expr_slot o, expr_slot d, const location_type & l):
        expression(l), object(o), dimension(d) {}
    expr_slot object;
    expr_slot dimension;
};

class reference : public expression
{
public:
    reference(var_ptr v, const location_type & loc = location_type(), type_ptr type = nullptr):
        expression(loc, type), var(v) {}
    var_ptr var;
    bool is_recursion = false;
};

class array_self_ref : public expression
{
public:
    array_self_ref(array_ptr arr, const location_type & loc, const type_ptr & type = nullptr):
        expression(loc, type), arr(arr) {}
    array_ptr arr;
};

class func_var : public var
{
public:
    func_var() {}
    func_var(const string & name, const location_type & loc): var(name,loc) {}
    string qualified_name;
};
typedef std::shared_ptr<func_var> func_var_ptr;

class function : public expression
{
public:
    function() {}
    function(const vector<func_var_ptr> & v, expr_slot e, const location_type & loc):
        expression(loc), vars(v), expr(e) {}
    function(const vector<func_var_ptr> & v, expr_ptr e, const location_type & loc):
        expression(loc), vars(v), expr(e) {}
    vector<func_var_ptr> vars;
    functional::scope scope;
    expr_slot expr;
};

class func_app : public expression
{
public:
    expr_slot object;
    vector<expr_slot> args;
};

class affine_expr : public expression
{
public:
    affine_expr() {}
    affine_expr(const linexpr & e, const location_type & loc = location_type()):
        expression(loc), expr(e) {}
    linexpr expr;
};

class affine_set : public expression
{
public:
    affine_set() {}
    affine_set(const linear_set & s): set(s) {}
    linear_set set;
};

}
}

#endif // STREAM_FUNCTIONAL_MODEL_INCLUDED

