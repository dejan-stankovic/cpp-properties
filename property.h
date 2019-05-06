#include <functional>
#include <unordered_map>

// Generic base class of TypedProperty and all properties
class Property
{
public:
    const char * const name;
    const bool writable;

    Property(const char *name, bool writable)
        : name(name)
        , writable(writable)
    {
    }
    virtual ~Property() { }

    template<typename T, typename Object> T get(Object *object);
    template<typename T, typename Object> void set(Object *object, T value);
};

template<typename T, typename O> class TypedProperty : public Property
{
public:
    const std::function<T(O*)> get;
    const std::function<void(O*,T)> set;

    TypedProperty(const char name[], std::function<T(O*)> get, std::function<void(O*,T)> set)
        : Property(name, (bool)set)
        , get(get)
        , set(set)
    {
    }
};

// XXX abort() is not a great reaction to mismatched types
template<typename T, typename Object> T Property::get(Object *object)
{
    auto val = dynamic_cast<TypedProperty<T,Object>*>(this);
    if (!val)
        abort();
    return val->get(object);
}

template<typename T, typename Object> void Property::set(Object *object, T value)
{
    auto val = dynamic_cast<TypedProperty<T,Object>*>(this);
    if (!val)
        abort();
    if (!writable)
        abort();
    val->set(object, value);
}

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
    Properties(std::initializer_list<Property*> props)
    {
        for (auto prop : props)
            properties.insert({prop->name, prop});
    }
    Properties(const Properties &) = delete;
    Properties &operator=(const Properties &) = delete;

    template<typename T, typename Object> T get(Object *object, const char *prop) const
    {
        return properties.at(prop)->template get<T,Object>(object);
    }

    template<typename T, typename Object> void set(Object *object, const char *prop, T value)
    {
        return properties.at(prop)->template set<T,Object>(object, value);
    }

    Property *operator[](const char *prop)
    {
        return properties.at(prop);
    }

private:
    std::unordered_map<const char*, Property*> properties;
};

// An empty set function makes a read-only property
template<typename T, typename O> Property *MakeProperty(const char name[], std::function<T(O*)> get, std::function<void(O*,T)> set = nullptr)
{
    // XXX leaked.. would be nice if this was allocated differently :shrug:
    return new TypedProperty<T,O>{name, get, set};
}

// Overload for template deduction; allows the use of: MakeProperty(&Object::get, &Object::set)
template<typename T, typename O> Property *MakeProperty(const char name[], T(O::*get)(), void(O::*set)(T) = nullptr)
{
    return MakeProperty<T,O>(name, std::function<T(O*)>(get), std::function<void(O*,T)>(set));
}


