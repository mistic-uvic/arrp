#ifndef STREAM_LANG_SEMANTIC_INCLUDED
#define STREAM_LANG_SEMANTIC_INCLUDED

#include "ast.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace stream {
namespace semantic {

using std::string;
using std::unordered_map;
using std::vector;
using std::deque;
using std::ostream;
template <typename T> using sp = std::shared_ptr<T>;
template <typename T> using up = std::unique_ptr<T>;

struct integer_num;
struct real_num;
struct stream;
struct range;

struct type
{
    enum tag
    {
        integer_num,
        real_num,
        range,
        stream
    };

    type(tag t): m_tag(t) {}
    virtual ~type() {}

    tag get_tag() const { return m_tag; }

    virtual void print_on( ostream & s ) const
    {
        s << "<tag = " << m_tag << ">";
    }

    bool is(tag t) { return m_tag == t; }

    template<typename T>
    T * as() { return static_cast<T*>(this); }

private:
    tag m_tag;
};

inline ostream & operator<<( ostream & s, const type & t )
{
    t.print_on(s);
    return s;
}

template <type::tag TAG>
class tagged_type : public type
{
public:
    tagged_type(): type(TAG)
    {}
};

template<typename T>
struct scalar
{
    scalar():
        m_is_constant(false)
    {}

    scalar( const T & value ):
        m_is_constant(true),
        m_value(value)
    {}

    bool is_constant() const { return m_is_constant; }

    T constant_value() const
    {
        assert(m_is_constant);
        return m_value;
    }

    void set_constant( const T & value )
    {
        m_value = value;
        m_is_constant = true;
    }

private:
    bool m_is_constant;
    T m_value;
};

struct integer_num : public scalar<int>, public tagged_type<type::integer_num>
{
    integer_num() {}
    integer_num(int v): scalar(v) {}
    virtual void print_on( ostream & s ) const
    {
        if (is_constant())
            s << "<i:" << constant_value() << ">";
        else
            s << "<i>";
    }
};

struct real_num : public scalar<double>, public tagged_type<type::real_num>
{
    real_num() {}
    real_num(double v): scalar(v) {}
    virtual void print_on( ostream & s ) const
    {
        if (is_constant())
            s << "<r:" << constant_value() << ">";
        else
            s << "<r>";
    }
};

struct range : public tagged_type<type::range>
{
    range() {}

    sp<type> start;
    sp<type> end;

    bool start_is_constant()
    {
        return !start || start->as<semantic::integer_num>()->is_constant();
    }

    bool end_is_constant()
    {
        return !end || end->as<semantic::integer_num>()->is_constant();
    }

    bool is_constant()
    {
        return start_is_constant() && end_is_constant();
    }

    int const_start()
    {
        return start->as<semantic::integer_num>()->constant_value();
    }

    int const_end()
    {
        return end->as<semantic::integer_num>()->constant_value();
    }

    int const_size()
    {
        return std::abs(const_end() - const_start()) + 1;
    }

    virtual void print_on( ostream & s ) const
    {
        s << "[";
        if (start)
            s << *start;
        s << "...";
        if (end)
            s << *end;
        s << "]";
    }
};

struct stream : public tagged_type<type::stream>
{
    stream( const vector<int> & s ) : size(s) {}
    int dimensionality() const { return size.size(); }
    vector<int> size;

    virtual void print_on( ostream & s ) const
    {
        s << "[";
        if (size.size())
            s << size.front();
        for (int i = 1; i < size.size(); ++i)
        {
            s << ',';
            s << size[i];
        }
        s << "]";
    }

    void reduce()
    {
        size.erase( std::remove(size.begin(), size.end(), 1), size.end() );
        if (size.empty())
            size.push_back(1);
    }
};

//semantic::integer_num *as_integer_num() { return static_cast<semantic::integer_num*>(this); }
//semantic::integer_num *as_real_num() { return static_cast<semantic::real_num*>(this); }
//semantic::integer_num *as_stream() { return static_cast<semantic::stream*>(this); }
//semantic::integer_num *as_range() { return static_cast<semantic::range*>(this); }

struct environment;

class symbol
{
public:
    symbol( const string & name ): m_name(name) {}
    const string & name() { return m_name; }
    virtual sp<type> evaluate( environment & envir,
                               const vector<sp<type>> & = vector<sp<type>>() ) = 0;
private:
    string m_name;
};

class constant_symbol : public symbol
{
public:
    constant_symbol( const string & name, const sp<ast::node> & code ):
        symbol(name),
        m_code(code)
    {}
    constant_symbol( const string & name, const sp<type> & value ):
        symbol(name),
        m_value(value)
    {}
    sp<type> evaluate( environment &, const vector<sp<type>> & );
private:
    sp<ast::node> m_code;
    sp<type> m_value;
};

class function_symbol : public symbol
{
public:
    function_symbol( const string & name,
                     const vector<string> & params,
                     const sp<ast::node> & code ):
        symbol(name),
        m_parameters(params),
        m_code(code)
    {}
    sp<type> evaluate( environment &, const vector<sp<type>> & );
private:
    vector<string> m_parameters;
    sp<ast::node> m_code;
};

typedef std::unordered_map<string, up<symbol>> symbol_map;

class environment : private deque<symbol_map>
{
public:
    environment()
    {
        enter_scope();
    }

    void bind( const string & name, symbol * sym )
    {
        back().emplace( name, up<symbol>(sym) );
    }

    void bind( const string & name, const sp<type> & val )
    {
        back().emplace( name, up<symbol>(new constant_symbol(name, val)) );
    }

    void enter_scope()
    {
        emplace_back();
    }

    void exit_scope()
    {
        pop_back();
    }

    symbol * operator[]( const string & key )
    {
        reverse_iterator it;
        for (it = rbegin(); it != rend(); ++it)
        {
            auto entry = it->find(key);
            if (entry != it->end())
              return entry->second.get();
        }

        return nullptr;
    }
};



struct iterator
{
    iterator(): hop(1), size(1), count(1) {}
    string id;
    int hop;
    int size;
    int count;
    sp<type> domain;
    sp<type> value;
};

environment top_environment( ast::node * program );

sp<type> evaluate_expr_block( environment & env, const sp<ast::node> & root );
void evaluate_stmt_list( environment & env, const sp<ast::node> & root );
symbol * evaluate_statement( environment & env, const sp<ast::node> & root );
sp<type> evaluate_expression( environment & env, const sp<ast::node> & root );
sp<type> evaluate_binop( environment & env, const sp<ast::node> & root );
sp<type> evaluate_range( environment & env, const sp<ast::node> & root );
sp<type> evaluate_hash( environment & env, const sp<ast::node> & root );
sp<type> evaluate_call( environment & env, const sp<ast::node> & root );
sp<type> evaluate_iteration( environment & env, const sp<ast::node> & root );
sp<type> evaluate_reduction( environment & env, const sp<ast::node> & root );
iterator evaluate_iterator( environment & env, const sp<ast::node> & root );

struct semantic_error : public std::runtime_error
{
    semantic_error(const string & what, int line = 0):
        runtime_error(what),
        m_line(line)
    {}
    int line() const { return m_line; }
    void report();
private:
    int m_line;
};

} // namespace semantic
} // namespace stream

#endif //  STREAM_LANG_SEMANTIC_INCLUDED
