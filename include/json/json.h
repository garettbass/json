#pragma once
#include <stddef.h>
#include <initializer_list>
#include <iosfwd>
#include <vector>
#include <string>
#include <sstream>


#define json_literal(...) ::json::read(#__VA_ARGS__)


class json;
class json_property;


//==========================================================================


struct json_format {
    const char* colon;
    const char* comma;
    const char* indent;
    const char* newline;
    const char* numbers;

    static json_format compact() {
        return {":",",","","","%g"};
    }
    static json_format indented(const char* indent = "    ") {
        return {": ",",",indent,"\n","%g"};
    }
};


//==========================================================================


template<typename T>
class json_iterators {
    T* head = nullptr;
    T* tail = nullptr;

public: // structors

    json_iterators() = default;

    json_iterators(T* head, T* tail)
    : head(head), tail(tail) {}

    json_iterators(T* head, size_t length)
    : head(head), tail(head + length) {}

    template<size_t SIZE>
    json_iterators(T (&array)[SIZE])
    : head(array), tail(array + SIZE) {}

public: // operators

    T& operator[] (size_t index) const {
        assert(head);
        assert(head + index < tail);
        return head[index];
    }

public: // properties

    bool empty() const { return head == tail; }

    size_t length() const { return std::distance(head, tail); }

    size_t size() const { return std::distance(head, tail); }

public: // accessors

    T* data() const { return head; }

    T& front() const { assert(not empty()); return head[ 0]; }
    T& back()  const { assert(not empty()); return tail[-1]; }

public: // iterators

    T*  begin() const { return head; }
    T*    end() const { return tail; }

    T* rbegin() const { return tail-1; }
    T*   rend() const { return head-1; }

public: // methods

    T*  find(const T& t) const;
    T* rfind(const T& t) const;
};


//==========================================================================


class json {
public: // types
    using format    = json_format;
    using element   = json;
    using property  = json_property;

    template<typename T>
    using iterators = json_iterators<T>;

    using null    = decltype(nullptr);
    using boolean = bool;
    using number  = double;
    using integer = int;
    using string  = std::string;
    using array   = std::vector<element>;
    using object  = std::vector<property>;

    enum TYPE {
        NULLJSON,
        BOOLEAN,
        NUMBER,
        STRING,
        ARRAY,
        OBJECT,
    };

    struct patch {
        enum OP {
            INSERT,
            REMOVE,
            SET,
        };
        static bool  apply(json& target, const json& patch);
        static bool remove(json& target, const string& path);
        static bool insert(json& target, const string& path, json value);
        static bool    set(json& target, const string& path, json value);
    };

    struct path {
        template<typename... Args>
        static std::string eval(Args&&...) {}
        template<typename... Args>
        static std::string join(Args&&...);
        static std::vector<std::string> split(const std::string& path);
    };

private: // fields
    union {
        json::number _number;
        json::string _string;
        json::array  _array;
        json::object _object;
    };
    TYPE _type;

public: // structors
    json();
    json(null);
    json(boolean);
    json(number);
    json(integer);
    json(string);
    json(const char*);
    json(array);
    json(object);
   ~json();

public: // serialization
    static json read(const string&);
    static json read(std::istream&);

    void write(std::ostream&, const format& = format::indented()) const;
    std::string write(const format& = format::indented()) const;

public: // copyable
    explicit
    json(const json&);
    json& operator=(const json&);

public: // movable
    json(json&&);
    json& operator=(json&&);

public: // assignable
    json& operator=(null);
    json& operator=(boolean);
    json& operator=(number);
    json& operator=(integer);
    json& operator=(string);
    json& operator=(const char*);
    json& operator=(array);
    json& operator=(object);

public: // operators
    bool operator==(const json&) const;
    bool operator!=(const json&) const;

    explicit operator boolean() const;
    explicit operator  number() const;
    explicit operator integer() const;
    explicit operator  string() const;

    const json& operator[](size_t index) const;
          json& operator[](size_t index);

    const json& operator[](const string& name) const;
          json& operator[](const string& name);

public: // type
    TYPE type() const;

public: // iteration
    iterators<const element> elements() const;
    iterators<      element> elements();

    iterators<const property> properties() const;
    iterators<      property> properties();

public: // search
    const json* child(size_t index) const;
          json* child(size_t index);

    const json* child(const string& name) const;
          json* child(const string& name);

    const json* descendant(const string& path) const;
          json* descendant(const string& path);

public: // modifiers
    void clear();
    bool erase(size_t index);
    bool erase(const string& name);

    void insert(size_t index, const json&);
    void insert(size_t index, json&&);

    void push_back(const json&);
    void push_back(json&&);
    void pop_back();

    void swap(json&);

};


json::boolean operator or(const json&, json::boolean fallback);
json::number  operator or(const json&, json::number  fallback);
json::integer operator or(const json&, json::integer fallback);
json::string  operator or(const json&, json::string  fallback);
json::string  operator or(const json&, const char*   fallback);


std::ostream& operator<<(std::ostream&, const json&);
std::ostream& operator<<(std::ostream&, const json::array&);
std::ostream& operator<<(std::ostream&, const json::object&);


//==========================================================================


class json_property : public json {
    string _name;

public: // structors
    json_property();
    json_property(const string& name);
    json_property(const string& name, json&&);
    json_property(const string& name, null);
    json_property(const string& name, boolean);
    json_property(const string& name, number);
    json_property(const string& name, integer);
    json_property(const string& name, string);
    json_property(const string& name, const char*);

public: // copyable
    json_property(const json_property&);
    json_property& operator=(const json_property&);
    json_property& operator=(const json&);

public: // movable
    json_property(json_property&&);
    json_property& operator=(json_property&&);
    json_property& operator=(json&&);

public: // assignable
    json_property& operator=(null);
    json_property& operator=(boolean);
    json_property& operator=(number);
    json_property& operator=(integer);
    json_property& operator=(string);
    json_property& operator=(const char*);
    json_property& operator=(array);
    json_property& operator=(object);

public: // properties
    string name() const;
};


//==========================================================================


template<typename... Args> inline
std::string
json::path::join(Args&&... args) {
    std::stringstream s;
    json::path::eval((s << '/' << args)...);
    return s.str();
}



//==========================================================================


namespace std {

    inline
    void swap(::json& a, ::json& b) { a.swap(b); }

} // namespace std
