#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>
#include <json/json.h>


#if defined(__GNUC__)
    #define forceinline __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define forceinline __forceinline
    #define strcasecmp _stricmp
#endif


/*==========================================================================
T& assign(T* target, const T& source)

Safely delegates copy assignment to existing destructor/copy constructor.
--------------------------------------------------------------------------*/
template <typename T>
T& assign(T* target, const T& source) {
    if (target != &source) {
        target->~T();
        new(target)T(source);
    }
    return *target;
}


/*==========================================================================
T& assign(T* target, T&& source)

Safely delegates move assignment to existing destructor/move constructor.
--------------------------------------------------------------------------*/
template <typename T>
T& assign(T* target, T&& source) {
    if (target != &source) {
        target->~T();
        new(target)T(std::move(source));
    }
    return *target;
}


// json::path ------------------------------------------------------------------


std::vector<std::string>
json::path::split(const std::string& path) {
    if (path.empty()) return {};
    if (path == "/") return {};

    std::vector<std::string> subpaths;

    const size_t n = path.length();
    size_t subpath = (path.front() == '/') ? 1 : 0;
    for (size_t i = subpath; i < n; ++i) {
        if (path[i] == '/') {
            subpaths.emplace_back(path.substr(subpath, i - subpath));
            subpath = i + 1;
        }
    }
    return subpaths;
}



// json structors --------------------------------------------------------------

template <typename T, typename... Args>
void construct(T& t, Args&&... args) {
    new(&t)T(std::forward<Args&&>(args)...);
}

template <typename T>
void destruct(T& t) { t.~T(); }

json::json()
: _type(NULLJSON) {
    construct(_number, 0.0);
}

json::json(null)
: _type(NULLJSON) {
    construct(_number, 0.0);
}

json::json(boolean value)
: _type(BOOLEAN) {
    construct(_number, value ? 1.0 : 0.0);
}

json::json(number value)
: _type(NUMBER) {
    construct(_number, value);
}

json::json(integer value)
: _type(NUMBER) {
    construct(_number, value);
}

json::json(string value)
: _type(STRING) {
    construct(_string, value);
}

json::json(const char* value)
: _type(value ? STRING : NULLJSON) {
    if (value) {
        construct(_string, value);
    } else {
        construct(_number, 0.0);
    }
}

json::json(array elements)
: _type(ARRAY) {
    construct(_array, std::move(elements));
}

json::json(object properties)
: _type(OBJECT) {
    construct(_object, std::move(properties));
}

json::~json() {
    switch (_type) {
        case STRING: {
            destruct(_string);
        } break;
        case ARRAY: {
            destruct(_array);
        } break;
        case OBJECT: {
            destruct(_object);
        } break;
        default: break;
    }
    _type = NULLJSON;
}

// json copy -------------------------------------------------------------------

json::json(const json& src)
: _type(src._type) {
    switch (src._type) {
        case NULLJSON: {
            construct(_number, 0.0);
            break;
        }
        case BOOLEAN:
        case NUMBER: {
            construct(_number, src._number);
            break;
        }
        case STRING: {
            construct(_string, src._string);
            break;
        }
        case ARRAY: {
            construct(_array, src._array);
            break;
        }
        case OBJECT: {
            construct(_object, src._object);
            break;
        }
        default: assert(false); break;
    }
}

json&
json::operator=(const json& src) {
    return assign(this, src);
}

// json move -------------------------------------------------------------------

json::json(json&& src)
: _type(src._type) {
    switch (src._type) {
        case NULLJSON: {
            construct(_number, 0.0);
            break;
        }
        case BOOLEAN:
        case NUMBER: {
            construct(_number, src._number);
            break;
        }
        case STRING: {
            construct(_string, std::move(src._string));
            break;
        }
        case ARRAY: {
            construct(_array, std::move(src._array));
            break;
        }
        case OBJECT: {
            construct(_object, std::move(src._object));
            break;
        }
        default: assert(false); break;
    }
    new(&src)json();
}

json&
json::operator=(json&& src) {
    return assign(this, std::move(src));
}

// json assignment -------------------------------------------------------------

json&
json::operator=(null) {
    this->~json(); return *new(this)json();
}

json&
json::operator=(boolean boolean) {
    this->~json(); return *new(this)json(boolean);
}

json&
json::operator=(number number) {
    this->~json(); return *new(this)json(number);
}

json&
json::operator=(integer number) {
    this->~json(); return *new(this)json(number);
}

json&
json::operator=(string string) {
    this->~json(); return *new(this)json(string);
}

json&
json::operator=(const char* string) {
    this->~json(); return *new(this)json(string);
}

json&
json::operator=(array elements) {
    this->~json(); return *new(this)json(std::move(elements));
}

json&
json::operator=(object properties) {
    this->~json(); return *new(this)json(std::move(properties));
}

// json operators --------------------------------------------------------------

bool
json::operator==(const json& b) const {
    if (_type != b._type)
        return false;

    switch (_type) {
        case NULLJSON: {
            return true;
        }
        case BOOLEAN:
        case NUMBER: {
            return _number == b._number;
        }
        case STRING: {
            return _string == b._string;
        }
        case ARRAY: {
            const size_t a_size =   _array.size();
            const size_t b_size = b._array.size();
            if (a_size != b_size) {
                return false;
            }
            for (size_t i = 0; i < a_size; ++i) {
                auto& a_element =   _array[i];
                auto& b_element = b._array[i];
                if (a_element != b_element)
                    return false;
            }
            return true;
        }
        case OBJECT: {
            const size_t a_size =   _object.size();
            const size_t b_size = b._object.size();
            if (a_size != b_size)
                return false;

            for (auto& a_prop : _object) {
                const json& a_value = a_prop;
                const json& b_value = b[a_prop.name()];
                if (a_value != b_value)
                    return false;
            }
            return true;
        }
    }
    return false;
}

bool
json::operator !=(const json& b) const {
    return not operator==(b);
}

json::operator boolean() const {
    switch (_type) {
        case BOOLEAN:
        case NUMBER: {
            return _number != 0.0;
        }
        case STRING: {
            if (strcasecmp(_string.data(), "true")  == 0) return true;
            if (strcasecmp(_string.data(), "false") == 0) return false;
            return operator number() != 0.0;
        }
        default: return false;
    }
}

json::operator number() const {
    switch (_type) {
        case BOOLEAN:
        case NUMBER: {
            return _number;
        }
        case STRING: {
            return number(strtod(_string.data(), nullptr));
        }
        default: return 0;
    }
}

json::operator integer() const {
    switch (_type) {
        case BOOLEAN:
        case NUMBER: {
            return integer(_number);
        }
        case STRING: {
            return integer(strtol(_string.data(), nullptr, 0));
        }
        default: return 0;
    }
}

json::operator string() const {
    switch (_type) {
        case BOOLEAN: {
            return (_number == 0.0) ? "false" : "true";
        }
        case NUMBER: {
            std::stringstream s; s << _number;
            return s.str();
        }
        case STRING: {
            return _string;
        }
        default: return {};
    }
}

static const json _json_null;

const json&
json::operator [](const size_t index) const {
    auto* e = child(index);
    return e ? *e : _json_null;
}

json&
json::operator [](const size_t index) {
    if (_type != ARRAY) operator=(json::array());
    if (index >= _array.size()) _array.resize(index + 1);
    return _array[index];
}

const json&
json::operator [](const string& name) const {
    auto* p = child(name);
    return p ? *p : _json_null;
}

json&
json::operator [](const string& name) {
    if (_type != OBJECT) operator=(json::object());
    for (auto& p : _object) if (name == p.name()) return p;
    _object.emplace_back(name);
    return _object.back();
}

json::boolean
operator or(const json& j, json::boolean fallback) {
    if (j.type() == json::BOOLEAN) return json::boolean(j);
    return fallback;
}

json::number
operator or(const json& j, json::number fallback) {
    if (j.type() == json::NUMBER) return json::number(j);
    return fallback;
}

json::integer
operator or(const json& j, json::integer fallback) {
    if (j.type() == json::NUMBER) return json::integer(j);
    return fallback;
}

json::string
operator or(const json& j, json::string fallback) {
    if (j.type() == json::STRING) return json::string(j);
    return fallback;
}

json::string
operator or(const json& j, const char* fallback) {
    if (j.type() == json::STRING) return json::string(j);
    return fallback;
}

// json type -------------------------------------------------------------------

json::TYPE
json::type() const { return _type; }

// json iteration --------------------------------------------------------------

json::iterators<const json::element>
json::elements() const {
    if (_type == ARRAY) {
        return { &*_array.begin(), &*_array.end() };
    }
    return {};
}

json::iterators<json::element>
json::elements() {
    if (_type == ARRAY) {
        return { &*_array.begin(), &*_array.end() };
    }
    return {};
}

json::iterators<const json::property>
json::properties() const {
    if (_type == OBJECT) {
        return { &*_object.begin(), &*_object.end() };
    }
    return {};
}

json::iterators<json::property>
json::properties() {
    if (_type == OBJECT) {
        return { &*_object.begin(), &*_object.end() };
    }
    return {};
}

// json search -----------------------------------------------------------------

static
int
toindex(const std::string& name) {
    const char* itr = &*name.begin();
    char* end = nullptr;
    const int index(strtol(itr, &end, 10));
    if (size_t(end) == size_t(&*name.end())) {
        // entire name was converted to index
        return index >= 0 ? index : -1;
    }
    return -1;
}

const json*
json::child(const size_t index) const {
    if (_type == ARRAY) {
        if (index < _array.size()) {
            return &_array[index];
        }
    }
    return nullptr;
}

json*
json::child(const size_t index) {
    if (_type == ARRAY) {
        if (index < _array.size()) {
            return &_array[index];
        }
    }
    return nullptr;
}

const json*
json::child(const string& name) const {
    if (_type == ARRAY) {
        const int index = toindex(name);
        return
            (index > 0)
            ? child(size_t(index))
            : nullptr;
    }
    if (_type == OBJECT) {
        auto ritr = _object.rbegin();
        auto rend = _object.rend();
        while (ritr != rend) {
            if (ritr->name() == name) {
                return &*ritr;
            }
            ++ritr;
        }
    }
    return nullptr;
}

json*
json::child(const string& name) {
    if (_type == ARRAY) {
        const int index = toindex(name);
        return
            (index > 0)
            ? child(size_t(index))
            : nullptr;
    }
    if (_type == OBJECT) {
        auto ritr = _object.rbegin();
        auto rend = _object.rend();
        while (ritr != rend) {
            if (ritr->name() == name) {
                return &*ritr;
            }
            ++ritr;
        }
    }
    return nullptr;
}

const json*
json::descendant(const string& path) const {
    const json* leaf = this;

    const auto subpaths = json::path::split(path);
    for (const string& subpath : subpaths) {
        const json* child = leaf->child(subpath);
        if (not child) return nullptr;
        leaf = child;
    }

    return leaf;
}

json*
json::descendant(const string& path) {
    json* leaf = this;

    const auto subpaths = json::path::split(path);
    for (const string& subpath : subpaths) {
        json* child = leaf->child(subpath);
        if (not child) return nullptr;
        leaf = child;
    }

    return leaf;
}

// json modifiers --------------------------------------------------------------

void
json::clear() {
    switch(_type) {
        case NULLJSON:
        case BOOLEAN:
        case NUMBER: {
            _number = 0.0;
            break;
        }
        case STRING: {
            _string.clear();
            break;
        }
        case ARRAY: {
            _array.clear();
            break;
        }
        case OBJECT: {
            _object.clear();
            break;
        }
    }
}

bool
json::erase(const size_t index) {
    if (_type == ARRAY) {
        if (index < _array.size()) {
            _array.erase(_array.begin() + index);
            return true;
        }
    }
    return false;
}

bool
json::erase(const string& name) {
    if (_type == ARRAY) {
        const int index = toindex(name);
        return (index > 0) and erase(size_t(index));
    }
    if (_type == OBJECT) {
        bool erased = false;
        auto itr = _object.begin();
        while (itr != _object.end()) {
            if (itr->name() == name) {
                itr = _object.erase(itr);
                erased = true;
            } else {
                ++itr;
            }
        }
        return erased;
    }
    return false;
}

void
json::insert(const size_t index, const json& element) {
    if (_type != ARRAY) operator=(json::array());
    _array.insert(_array.begin() + index, element);
}

void
json::insert(const size_t index, json&& element) {
    if (_type != ARRAY) operator=(json::array());
    _array.emplace(_array.begin() + index, std::move(element));
}

void
json::push_back(const json& element) {
    if (_type != ARRAY) operator=(json::array());
    _array.push_back(element);
}

void
json::push_back(json&& element) {
    if (_type != ARRAY) operator=(json::array());
    _array.emplace_back(std::move(element));
}

void
json::pop_back() {
    if (_type == ARRAY) _array.pop_back();
}

void
json::swap(json& b) {
    std::swap(_type, b._type);
    json& a = *this;
    json c(std::move(a));
    new(&a)json(std::move(b));
    new(&b)json(std::move(c));
}


// json::patch =================================================================


bool
json::patch::apply(json& target, const json& patch) {
    if (not patch.child("op")) return false;
    if (not patch.child("path")) return false;

    const auto path = string(patch["path"]);
    const auto op   = string(patch["op"]);

    if (op == "remove") return remove(target, path);

    const json* value = patch.child("value");
    if (not value) return false;

    if (op == "insert") return insert(target, path, json(*value));
    if (op ==    "set") return    set(target, path, json(*value));

    return false;
}

bool
json::patch::remove(json& target, const string& path) {
    auto subpaths = json::path::split(path);
    if (subpaths.empty()) return false;

    const auto leafname = subpaths.back();
    subpaths.pop_back();

    json* parent = &target;

    for (const string& subpath : subpaths) {
        json* child = parent->child(subpath);
        if (not child) return false;
        parent = child;
    }

    return parent->erase(leafname);
}

bool
json::patch::insert(json& target, const string& path, json value) {
    auto subpaths = json::path::split(path);
    if (subpaths.empty()) {
        target = std::move(value);
        return true;
    }

    const auto leafname = subpaths.back();
    subpaths.pop_back();

    json* parent = &target;

    for (const string& subpath : subpaths) {
        json* child = parent->child(subpath);
        if (not child) return false;
        parent = child;
    }

    if (parent->_type == ARRAY) {
        const int index = toindex(leafname);
        if (index < 0) return false;
        parent->insert(size_t(index), std::move(value));
        return true;
    }
    if (parent->_type == OBJECT) {
        parent->operator[](leafname) = std::move(value);
        return true;
    }
    return false;
}

bool
json::patch::set(json& target, const string& path, json value) {
    auto subpaths = json::path::split(path);
    if (subpaths.empty()) {
        target = std::move(value);
        return true;
    }

    const auto leafname = subpaths.back();
    subpaths.pop_back();

    json* parent = &target;

    for (const string& subpath : subpaths) {
        json* child = parent->child(subpath);
        if (not child) return false;
        parent = child;
    }

    if (parent->_type == ARRAY) {
        const int index = toindex(leafname);
        if (index < 0) return false;
        parent->operator[](size_t(index)) = std::move(value);
        return true;
    }
    if (parent->_type == OBJECT) {
        parent->operator[](leafname) = std::move(value);
    }
    return false;
}


// json_property ===============================================================


json_property::json_property(const string& name)
: json() {
    new(&_name)string(name);
}

json_property::json_property(const string& name, json&& value)
: json_property(name) {
    json::operator=(std::move(value));
}

json_property::json_property(const string& name, json::null value)
: json_property(name) {
    json::operator=(value);
}

json_property::json_property(const string& name, json::boolean value)
: json_property(name) {
    json::operator=(value);
}

json_property::json_property(const string& name, json::number value)
: json_property(name) {
    json::operator=(value);
}

json_property::json_property(const string& name, json::integer value)
: json_property(name) {
    json::operator=(value);
}

json_property::json_property(const string& name, json::string value)
: json_property(name) {
    json::operator=(value);
}

json_property::json_property(const string& name, const char* value)
: json_property(name) {
    json::operator=(value);
}

// json_property copy ----------------------------------------------------------

json_property::json_property(const json_property& src)
: json(src)
, _name(src._name) {}

json_property&
json_property::operator=(const json_property& src) {
    return assign(this, src);
}

json_property&
json_property::operator=(const json& src) {
    assign((json*)this, src);
    return *this;
}

// json_property move ----------------------------------------------------------

json_property::json_property(json_property&& src)
: json(std::move(src))
, _name(std::move(src._name)) {}

json_property&
json_property::operator=(json_property&& src) {
    return assign(this, std::move(src));
}

json_property&
json_property::operator=(json&& src) {
    assign((json*)this, std::move(src));
    return *this;
}

// json_property assign --------------------------------------------------------

json_property&
json_property::operator=(null) {
    json::operator=(null());
    return *this;
}

json_property&
json_property::operator=(boolean value) {
    json::operator=(value);
    return *this;
}

json_property&
json_property::operator=(number value) {
    json::operator=(value);
    return *this;
}

json_property&
json_property::operator=(integer value) {
    json::operator=(value);
    return *this;
}

json_property&
json_property::operator=(string value) {
    json::operator=(value);
    return *this;
}

json_property&
json_property::operator=(const char* value) {
    json::operator=(value);
    return *this;
}

json_property&
json_property::operator=(array value) {
    json::operator=(value);
    return *this;
}

json_property&
json_property::operator=(object value) {
    json::operator=(value);
    return *this;
}

// json_property properties ----------------------------------------------------

json::string
json_property::name() const {
    return _name;
}


// unicode utilities ===========================================================


inline bool is_surrogate_head(const char16_t c)
{
    return ((c & 0xFC00) == 0xD800);
}


inline bool is_surrogate_tail(const char16_t c)
{
    return ((c & 0xFC00) == 0xDC00);
}

//--------------------------------------------------------------------------


inline size_t to_codepoint(char32_t& u, const char16_t* p)
{
    char16_t c0 = p[0];
    char16_t c1 = p[1];

    const bool is_c0_head = is_surrogate_head(c0);
    const bool is_c1_tail = is_surrogate_tail(c1);

    const bool is_size_2 = (is_c0_head & is_c1_tail);
    const bool is_size_1 = not is_size_2;

    const size_t size =
        (is_size_1 * 1) +
        (is_size_2 * 2);

    const char32_t uuuuuyyyyyy = ((c0 & 0x03FF) + 0x40);
    const char32_t  xxxxxxxxxx = ((c1 & 0x03FF));

    const char32_t u1 = (is_size_1 * c0) & 0xFFFF;
    const char32_t u2 = (is_size_2 * ((uuuuuyyyyyy << 10) + xxxxxxxxxx));

    u = u1 + u2;

    return size;
}


inline size_t to_codepoint(char32_t& u, const char* p)
{
    const char c0 = p[0];
    const char c1 = p[1];
    const char c2 = p[2];
    const char c3 = p[3];

    const bool is_size_1 = ((c0 & 0x80) == 0x00); // 0xxx xxxx
    const bool is_size_2 = ((c0 & 0xE0) == 0xC0); // 110x xxxx
    const bool is_size_3 = ((c0 & 0xF0) == 0xE0); // 1110 xxxx
    const bool is_size_4 = ((c0 & 0xF8) == 0xF0); // 1111 0xxx

    const size_t size =
        (is_size_1 * 1) +
        (is_size_2 * 2) +
        (is_size_3 * 3) +
        (is_size_4 * 4);

    const char32_t u1 =
        (is_size_1 * c0);

    const char32_t u2 =
        (is_size_2 * (
            (uint32_t(c0 & 0x1F) << 6) +
            (uint32_t(c1 & 0x3F))));

    const char32_t u3 =
        (is_size_3 * (
            (uint32_t(c0 & 0x0F) << 12) +
            (uint32_t(c1 & 0x3F) <<  6) +
            (uint32_t(c2 & 0x3F))));

    const char32_t u4 =
        (is_size_4 * (
            (uint32_t(c0 & 0x07) << 18) +
            (uint32_t(c1 & 0x3F) << 12) +
            (uint32_t(c2 & 0x3F) <<  6) +
            (uint32_t(c3 & 0x3F))));

    u = u1 + u2 + u3 + u4;

    return size;
}


//--------------------------------------------------------------------------


inline bool is_valid_codepoint(const char32_t u)
{
    return (u <= 0x10FFFF) & ((u < 0xD800) | (u > 0xDFFF));
}


inline bool is_valid_codepoint(const char16_t* p)
{
    char32_t c;

    const size_t size = to_codepoint(c, p);

    const bool _is_legible_utf16 = (size > 0);

    const bool _is_valid_utf32 = is_valid_codepoint(c);

    return _is_legible_utf16 & _is_valid_utf32;
}


inline bool is_valid_codepoint(const char* p)
{
    char32_t c;

    const size_t size = to_codepoint(c, p);

    const bool is_legible_utf8 = (size > 0);

    const bool is_valid_utf32 = is_valid_codepoint(c);

    return is_legible_utf8 & is_valid_utf32;
}


// utf8_codepoint ==============================================================


class utf8_codepoint {
    char   _data[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    size_t _size = 0;

public: // structors

    utf8_codepoint() = default;

public: // operators

    bool operator () (const char32_t);
    bool operator () (const char32_t*&);
    bool operator () (const char16_t*&);
    bool operator () (const char*&);

    const char& operator [] (size_t i) const { return _data[i]; }

    template <typename Stream>
    auto operator << (Stream& s)
    -> decltype(s << 0) const
    { return s.write(_data, _size); }

public: // properties

    const char* data() const { return _data; }

    bool is_valid() const
    {
        return (_size > 0) & is_valid_codepoint(_data);
    }

    size_t size() const { return _size; }

public: // iterators

    const char* begin() const { return _data; }

    const char* end() const { return _data + _size; }
};


//------------------------------------------------------------------------------


inline bool utf8_codepoint::operator()(const char32_t c)
{
    const bool c_le_0x0000007F = (c <= 0x0000007F);
    const bool c_le_0x000007FF = (c <= 0x000007FF);
    const bool c_le_0x0000FFFF = (c <= 0x0000FFFF);
    const bool c_le_0x0010FFFF = (c <= 0x0010FFFF);

    const bool is_size_1 = c_le_0x0000007F;
    const bool is_size_2 = c_le_0x000007FF & not c_le_0x0000007F;
    const bool is_size_3 = c_le_0x0000FFFF & not c_le_0x000007FF;
    const bool is_size_4 = c_le_0x0010FFFF & not c_le_0x0000FFFF;

    const size_t size =
        (is_size_1 * 1) +
        (is_size_2 * 2) +
        (is_size_3 * 3) +
        (is_size_4 * 4);

    const bool is_legible = (size > 0);

    _data[0] =
        (is_size_1 * (c)) +
        (is_size_2 * (0xC0 + (c >>  6))) +
        (is_size_3 * (0xE0 + (c >> 12))) +
        (is_size_4 * (0xF0 + (c >> 18)));

    _data[1] =
        (is_size_2 * (0x80 + (0x3F & (c)))) +
        (is_size_3 * (0x80 + (0x3F & (c >>  6)))) +
        (is_size_4 * (0x80 + (0x3F & (c >> 12))));

    _data[2] =
        (is_size_3 * (0x80 + (0x3F & (c)))) +
        (is_size_4 * (0x80 + (0x3F & (c >> 6))));

    _data[3] =
        (is_size_4 * (0x80 + (0x3F & (c))));

    _size = size;

    return is_legible;
}


inline bool utf8_codepoint::operator()(const char32_t*& p)
{
    const char32_t c = p[0];

    const bool is_legible = operator()(c);

    p += is_legible;

    return is_legible;
}


inline bool utf8_codepoint::operator()(const char16_t*& p)
{
    char32_t c;

    const size_t utf16_size = to_codepoint(c, p);

    const bool is_legible_utf16 = (utf16_size > 0);
    const bool is_legible_utf32 = operator()(c);
    const bool is_legible       = is_legible_utf16 & is_legible_utf32;

    _size *= is_legible;

    p += is_legible * utf16_size;

    return is_legible;
}


inline bool utf8_codepoint::operator()(const char*& p)
{
    const char c0 = p[0];
    const char c1 = p[1];
    const char c2 = p[2];
    const char c3 = p[3];

    const bool is_size_1 = ((c0 & 0x80) == 0x00); // 0xxx xxxx
    const bool is_size_2 = ((c0 & 0xE0) == 0xC0); // 110x xxxx
    const bool is_size_3 = ((c0 & 0xF0) == 0xE0); // 1110 xxxx
    const bool is_size_4 = ((c0 & 0xF8) == 0xF0); // 1111 0xxx

    const size_t size =
        (is_size_1 * 1) +
        (is_size_2 * 2) +
        (is_size_3 * 3) +
        (is_size_4 * 4);

    const bool is_legible = (size > 0);

    _data[0] = (size >= 1) * c0;
    _data[1] = (size >= 2) * c1;
    _data[2] = (size >= 3) * c2;
    _data[3] = (size >= 4) * c3;

    _size = size;

    p += size;

    return is_legible;
}


// json_parser =================================================================


struct json_parser {
    const std::string&    src;
    std::vector<char>     utf8_buffer;
    std::vector<char16_t> utf16_buffer;

    using ccitr = const char*;
    using ccend = const char* const;

    json_parser(const std::string& src)
    : src(src) {
        utf8_buffer.reserve(64);
        utf16_buffer.reserve(32);
    }

    json parse() {
        if (src.empty()) return json::null();
        ccitr itr = src.c_str();
        ccend end = itr + src.size();
        json root;
        if (parse_value(root, itr, end)) {
            return root;
        }
        return json::null();
    }

    bool parse_value(json& root, ccitr& itr, ccend end) {
        skip_whitespace(itr, end);
        if (itr < end) {
            switch (const int c = itr[0]) {
                case int('n'): return parse_null(root, itr, end);
                case int('f'): return parse_false(root, itr, end);
                case int('t'): return parse_true(root, itr, end);
                case int('"'): return parse_string(root, itr, end);
                case int('['): return parse_array(root, itr, end);
                case int('{'): return parse_object(root, itr, end);
            }
            return parse_number(root, itr, end);
        }
        return false;
    }

    // JSON null parsing ---------------------------------------------------

    forceinline
    static
    bool parse_null(json& root, ccitr& itr, ccend end) {
        if (skip_literal(itr, end, "null")) {
            root = json::null();
            return true;
        }
        return false;
    }

    // JSON boolean parsing ------------------------------------------------

    forceinline
    static
    bool parse_false(json& root, ccitr& itr, ccend end) {
        if (skip_literal(itr, end, "false")) {
            root = json::boolean(false);
            return true;
        }
        return false;
    }

    forceinline
    static
    bool parse_true(json& root, ccitr& itr, ccend end) {
        if (skip_literal(itr, end, "true")) {
            root = json::boolean(true);
            return true;
        }
        return false;
    }

    // JSON number parsing -------------------------------------------------

    static
    bool parse_number(json& root, ccitr& itr, ccend end) {
        // NOTE: strtod() format is more tolerant than JSON number
        const char* str = itr;
        char* str_end = const_cast<char*>(end);
        double value = strtod(str, &str_end);
        if (str_end > str) {
            root = json(value);
            itr = str_end;
            return true;
        }
        return false;
    }

    // JSON string parsing -------------------------------------------------

    bool parse_string(json& root, ccitr& itr, ccend end) {
        if (not parse_string_into_utf8(itr, end))
            return false;

        root = json::string(utf8_buffer.data(), utf8_buffer.size());
        return true;
    }

    forceinline
    bool parse_string_into_utf8(ccitr& itr, ccend end) {
        if (not skip_char(itr, end, '"'))
            return false;

        utf8_buffer.clear();
        while (itr < end) {
            const uint8_t u = itr[0];
            if (u < uint8_t(' ')) {
                // found control character
                return false;
            }
            if (u == uint8_t('"')) {
                // found end of string
                ++itr;
                return true;
            }
            if (u == uint8_t('\\')) {
                // found escape sequence
                if (parse_escape_sequence_into_utf8(itr, end))
                    continue;
                return false;
            }
            // found UTF8 octet
            utf8_buffer.push_back(u);
            ++itr;
        }
        return false;
    }

    forceinline
    bool parse_escape_sequence_into_utf8(ccitr& itr, ccend end) {
        if (not has_char(itr, end, '\\'))
            return false;

        if (itr + 2 < end) {
            const char c = itr[1];
            switch (c) {
                // NOTE: \0 is not valid JSON, but \u0000 is
                case  '0': utf8_buffer.push_back('\0'); itr+=2; return true;
                case '\\': utf8_buffer.push_back('\\'); itr+=2; return true;
                case  '"': utf8_buffer.push_back('"');  itr+=2; return true;
                case  '/': utf8_buffer.push_back('/');  itr+=2; return true;
                case  'b': utf8_buffer.push_back('\b'); itr+=2; return true;
                case  'f': utf8_buffer.push_back('\f'); itr+=2; return true;
                case  'n': utf8_buffer.push_back('\n'); itr+=2; return true;
                case  'r': utf8_buffer.push_back('\r'); itr+=2; return true;
                case  't': utf8_buffer.push_back('\t'); itr+=2; return true;
                case  'u': return parse_escaped_utf16_into_utf8(itr, end);
            }
        }
        return false;
    }

    forceinline
    bool parse_escaped_utf16_into_utf8(ccitr& itr, ccend end) {
        utf16_buffer.clear();
        while ((itr + 6) < end and itr[0] == '\\' and itr[1] == 'u') {
            char16_t utf16 = 0;
            utf16 += hex_to_int(itr[2]); utf16 <<= 4;
            utf16 += hex_to_int(itr[3]); utf16 <<= 4;
            utf16 += hex_to_int(itr[4]); utf16 <<= 4;
            utf16 += hex_to_int(itr[5]);
            utf16_buffer.push_back(utf16);
            itr += 6;
        }
        utf8_codepoint utf8;
        const char16_t*       utf16_itr = &*utf16_buffer.begin();
        const char16_t* const utf16_end = &*utf16_buffer.end();
        while (utf16_itr < utf16_end) {
            if (not utf8(utf16_itr))
                return false;
            for (char c : utf8) {
                utf8_buffer.push_back(c);
            }
        }
        return true;
    }

    // JSON array parsing --------------------------------------------------

    bool parse_array(json& root, ccitr& itr, ccend end) {
        if (not skip_char(itr, end, '['))
            return false;

        json::array elements;
        elements.reserve(32);
        while (itr < end) {
            skip_whitespace(itr, end);
            if (skip_char(itr, end, ']'))
                break;
            json element;
            if (not parse_value(element, itr, end))
                return false;
            elements.push_back(element);
            skip_char(itr, end, ',');
        }
        root = json(std::move(elements));
        return true;
    }

    // JSON object parsing -------------------------------------------------

    bool parse_object(json& root, ccitr& itr, ccend end) {
        if (not skip_char(itr, end, '{'))
            return false;

        json::object properties;
        properties.reserve(16);
        while (itr < end) {
            skip_whitespace(itr, end);
            if (skip_char(itr, end, '}'))
                break;
            if (not parse_string_into_utf8(itr, end))
                return false;
            skip_whitespace(itr, end);
            if (not skip_char(itr, end, ':'))
                return false;
            skip_whitespace(itr, end);
            const json::string name(utf8_buffer.data(), utf8_buffer.size());
            json_property property(name);
            if (not parse_value(property, itr, end))
                return false;
            properties.push_back(std::move(property));
            skip_whitespace(itr, end);
            skip_char(itr, end, ',');
        }
        root = json(std::move(properties));
        return true;
    }

    // JSON parsing utilities ----------------------------------------------


    forceinline
    static bool has_digit(ccitr itr, ccend end) {
        return (itr < end) and isdigit(itr[0]);
    }

    forceinline
    static bool has_char(ccitr itr, ccend end, const char c) {
        return (itr < end) and (itr[0] == c);
    }

    forceinline
    static int is_hex(const int c) {
        enum:int{D0='0',D9='9',A='A',F='F',a='a',f='f'};
        return ((c>=D0) & (c<=D9))|((c>=A) & (c<=F))|((c>=a) & (c<=f));
    }

    forceinline
    static char16_t hex_to_int(uint8_t u) {
        assert(is_hex(u));
        return ((u >> 6) * 9) + (u & 0xF);
    }

    static void skip_whitespace(ccitr& itr, ccend end) {
        while(itr < end and isspace(itr[0])) ++itr;
    }

    forceinline
    static bool skip_char(ccitr& itr, ccend end, const char c) {
        if (has_char(itr, end, c)) { ++itr; return true; }
        return false;
    }

    forceinline
    static int is_literal_end_char(const int c) {
        enum:int{SP=' ',HT='\t',LF='\n',CR='\r',CM=',',RB=']',RC='}'};
        return ((c==SP)|(c==HT)|(c==LF)|(c==CR)|(c==CM)|(c==RB)|(c==RC));
    }

    forceinline
    static int is_literal_end(ccitr itr, ccend end) {
        return itr == end or is_literal_end_char(itr[0]);
    }

    static bool skip_literal(ccitr& itr, ccend end, const char* s) {
        ccitr p = itr;
        while(p < end and *s and *p == *s) ++p, ++s;
        if (*s == 0 and is_literal_end(p, end)) {
            itr = p;
            return true;
        }
        return false;
    }
};


json
json::read(const string& src) {
    json_parser parser(src);
    return parser.parse();
}


json
json::read(std::istream& in) {
    if (in.good()) {
        in.seekg(0, in.end);
        const size_t size = in.tellg();
        in.seekg(0, in.beg);
        std::string src(size, '\0');
        in.read(&src.front(), size);
        return read(src);
    }
    return NULLJSON;
}


// json writing utilities ======================================================


static
const char*
control_char_escape_sequences[] {
    R"(\u0000)",
    R"(\u0001)",
    R"(\u0002)",
    R"(\u0003)",
    R"(\u0004)",
    R"(\u0005)",
    R"(\u0006)",
    R"(\u0007)",
    R"(\b)",
    R"(\t)",
    R"(\n)",
    R"(\u000B)",
    R"(\f)",
    R"(\r)",
    R"(\u000E)",
    R"(\u000F)",
    R"(\u0010)",
    R"(\u0011)",
    R"(\u0012)",
    R"(\u0013)",
    R"(\u0014)",
    R"(\u0015)",
    R"(\u0016)",
    R"(\u0017)",
    R"(\u0018)",
    R"(\u0019)",
    R"(\u001A)",
    R"(\u001B)",
    R"(\u001C)",
    R"(\u001D)",
    R"(\u001E)",
    R"(\u001F)",
};

forceinline
static
void
write_string(std::ostream& s, const json::string& str) {
    s.put('"');
    for (const char c : str) {
        const uint8_t u(c);
        if (u < uint8_t(' ')) {
            s << control_char_escape_sequences[u];
            continue;
        }
        if (u == uint8_t('\"')) {
            s << R"(\")";
            continue;
        }
        if (u == uint8_t('\\')) {
            s << R"(\\)";
            continue;
        }
        if (u == uint8_t(0x7F)) {
            s << R"(\u007F)";
            continue;
        }
        s.put(u);
    }
    s.put('"');
}

forceinline
static
void
write_indent(std::ostream& s, const char* indent, const int depth) {
    assert(depth >= 0);
    if (indent[0]) for (int i = 0; i < depth; ++i) s << indent;
}

static
void
write_json(
    std::ostream&      s,
    const json_format& format,
    const json&        root,
    const int          depth
);

forceinline
static
void
write_array(
    std::ostream& s,
    const json_format& format,
    const json_iterators<const json::element>& elements,
    int depth
) {
    s.put('[');
    auto itr = elements.begin();
    auto end = elements.end();
    depth += 1;
    if (itr < end) {
        s << format.newline;
        write_indent(s, format.indent, depth);
        write_json(s, format, *itr, depth);
        ++itr;
        while (itr < end) {
            s << format.comma;
            s << format.newline;
            write_indent(s, format.indent, depth);
            write_json(s, format, *itr, depth);
            ++itr;
        }
        s << format.newline;
    }
    depth -= 1;
    write_indent(s, format.indent, depth);
    s.put(']');
}

forceinline
static
void
write_object(
    std::ostream& s,
    const json_format& format,
    const json_iterators<const json::property>& properties,
    int depth
) {
    s.put('{');
    auto itr = properties.begin();
    auto end = properties.end();
    depth += 1;
    if (itr < end) {
        s << format.newline;
        write_indent(s, format.indent, depth);
        write_string(s, itr->name());
        s << format.colon;
        write_json(s, format, *itr, depth);
        ++itr;
        while (itr < end) {
            s << format.comma;
            s << format.newline;
            write_indent(s, format.indent, depth);
            write_string(s, itr->name());
            s << format.colon;
            write_json(s, format, *itr, depth);
            ++itr;
        }
        s << format.newline;
    }
    depth -= 1;
    write_indent(s, format.indent, depth);
    s.put('}');
}

static
void write_json(
    std::ostream&      s,
    const json_format& format,
    const json&        root,
    const int          depth
) {
    switch (root.type()) {
        case json::NULLJSON: {
            s << "null";
        } break;
        case json::BOOLEAN: {
            s << (bool(root) ? "true" : "false");
        } break;
        case json::NUMBER: {
            /*
            double number = double(root);
            double integer = 0.0;
            double fraction = modf(number, &integer);
            if (fraction == 0.0) {
                // write without .0000000
                s << std::fixed << std::setprecision(0) << integer;
            }
            else
            {
                s << std::fixed << std::setprecision(7) << number;
            }
            */
            char buffer[32];
            snprintf(buffer, sizeof(buffer), format.numbers, double(root));
            s << buffer;
        } break;
        case json::STRING: {
            write_string(s, json::string(root));
        } break;
        case json::ARRAY: {
            write_array(s, format, root.elements(), depth);
        } break;
        case json::OBJECT: {
            write_object(s, format, root.properties(), depth);
        } break;
        default: break;
    }
}

void json::write(std::ostream& s, const format& format) const {
    write_json(s, format, *this, 0);
}

std::string json::write(const format& format) const {
    std::stringstream s;
    write(s, format);
    return s.str();
}

std::ostream& operator <<(std::ostream& s, const json& j) {
    write_json(s, json::format::indented(), j, 0);
    return s;
}

std::ostream& operator <<(std::ostream& s, const json::array& a) {
    write_array(s, json::format::indented(), {&*a.begin(), &*a.end()}, 0);
    return s;
}

std::ostream& operator <<(std::ostream& s, const json::object& o) {
    write_object(s, json::format::indented(), {&*o.begin(), &*o.end()}, 0);
    return s;
}
