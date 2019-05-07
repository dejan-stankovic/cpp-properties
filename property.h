#include <functional>
#include <vector>
#include <cstring>

// Silly little stand-in for a variant type, don't read into it
struct Value
{
    union {
        int intValue;
        const char *charValue;
    };
    bool isInt;

    Value(int v) : intValue(v), isInt(true) { }
    Value(const char *v) : charValue(v), isInt(false) { }

    template<typename T> T convert();

    void print() {
        if (isInt)
            printf("value int: %d\n", intValue);
        else
            printf("value string: %s\n", charValue);
    }
};

template<> int Value::convert<int>()
{
    if (isInt)
        return intValue;
    else
        return 0;
};

template<> const char *Value::convert<const char*>()
{
    if (isInt)
        return nullptr;
    else
        return charValue;
};

class Object;

using PropertyGetter = std::function<Value(Object*)>;
using PropertySetter = std::function<void(Object*,Value)>;
template<typename O, typename T> PropertyGetter wrapPropertyGetter(const std::function<T(O*)> &getter)
{
    return [getter](Object *o) {
        auto obj = dynamic_cast<O*>(o);
        if (!obj)
            abort();
        return getter(obj);
    };
}

template<typename O, typename T> PropertySetter wrapPropertySetter(const std::function<void(O*,T)> &setter)
{
    return [setter](Object *o, Value value) {
        auto obj = dynamic_cast<O*>(o);
        if (!obj)
            abort();
        setter(obj, value.convert<T>());
    };
}

class Property
{
public:
    const char * const name;
    const PropertyGetter get;
    const PropertySetter set;

    Property()
        : name(nullptr)
        , get(nullptr)
        , set(nullptr)
    {
    }

    Property(const char *name, const PropertyGetter &&getter, const PropertySetter &&setter)
        : name(name)
        , get(std::move(getter))
        , set(std::move(setter))
    {
    }

    // For convenience, wrap the functions transparently in an overload
    template<typename O, typename T> Property(const char *name, std::function<T(O*)> &getter, std::function<void(O*,T)> &setter)
        : Property(name, wrapPropertyGetter(getter), wrapPropertySetter(setter))
    {
    }

    bool isWritable() const { return (bool)set; }
};

// Properties holds a map of properties for a type. Usually declared as a static member and
// defined like:
//
// Properties<Thing> Thing::properties = {
//     MakeProperty("message", &Thing::getMessage, &Thing::setMessage),
//     MakeProperty("readonly", &Thing::getMessage)
// };
//
class Properties
{
public:
    Properties(std::initializer_list<Property> props)
    {
        for (auto prop : props) {
            properties.push_back(prop);
        }
    }
    Properties(const Properties &) = delete;
    Properties &operator=(const Properties &) = delete;

    Property find(const char *prop) const
    {
        for (const auto& iprop : properties) {
            if (strcmp(iprop.name, prop) == 0) {
                return iprop;
            }
        }

        return Property();
    }

    Value get(Object *object, const char *prop) const
    {
        return find(prop).get(object);
    }

    void set(Object *object, const char *prop, Value value)
    {
        return find(prop).set(object, value);
    }

    Property operator[](const char *prop)
    {
        return find(prop);
    }

private:
    std::vector<Property> properties;
};

// Object is a base class, provided for easy casting. It isn't otherwise meaningful.
// Properties is statically allocated in the subclass, so it's awkwardly passed in here.
class Object
{
public:
    Object(Properties &props)
        : m_properties(props)
    {
    }
    virtual ~Object() { }

    Property property(const char *prop) { return m_properties[prop]; }
    Value getProperty(const char *prop) { return m_properties[prop].get(this); }
    void setProperty(const char *prop, Value value) { m_properties[prop].set(this, value); }

private:
    Properties &m_properties;
};

// XXX These could probably be made into constructors and make a prettier syntax

// An empty set function makes a read-only property
template<typename T, typename O> Property MakeProperty(const char name[], std::function<T(O*)> get, std::function<void(O*,T)> set = nullptr)
{
    // XXX This is leaked, if you care
    return Property(name, get, set);
}

// Overload for template deduction; allows the use of: MakeProperty(&Object::get, &Object::set)
template<typename T, typename O> Property MakeProperty(const char name[], T(O::*get)(), void(O::*set)(T) = nullptr)
{
    return MakeProperty<T,O>(name, std::function<T(O*)>(get), std::function<void(O*,T)>(set));
}

